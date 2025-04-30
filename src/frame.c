#include <raylib.h>
#include <string.h>
#include <raymath.h>
#include <stdlib.h>
#include "frame.h"
#include "screens.h"
#include "control.h"
#include "character.h"
#include "game.h"
#include "resource.h"
#include "lib.h"
#include "level.h"

const ScreenSize resolutions[5] = {
	{ .width = 480, .height = 270 },
	{ .width = 960, .height = 540 },
	{ .width = 1280, .height = 720 },
	{ .width = 1440, .height = 810 },
	{ .width = 1920, .height = 1080 },
};

static void AddDrawCall(DrawQueue* queue, DrawCall call) {
	if (queue->count + 1 >= queue->max) {
		LogDebug("Attempted to store a call over the limit! Count: %lu Max: %lu", queue->count, queue->max);
		return;
	}
	queue->calls[queue->count++] = call;
}

static float GetScreenScale() {
	float screenWidth = (float) GetScreenWidth();
	float screenHeight = (float) GetScreenHeight();
	float gameScreenWidth = (float) WORLD_SIZE_WIDTH;
	float gameScreenHeight = (float) WORLD_SIZE_HEIGHT;
	return fmaxf(fminf(screenWidth / gameScreenWidth, screenHeight / gameScreenHeight), 1.0f);
}

static Vector2 CameraClampedPos(Camera2D* camera, Room* room, Vector2 target) {
	if (room->tileCount == 0 || room->rows == 0 || room->columns == 0) {
		LogDebug("Invalid room tiles");
		return target;
	}
	Vector2 roomOffset = RoomOffset(room);
	const float minX = 0.0f + camera->offset.x + roomOffset.x;
	const float minY = 0.0f + camera->offset.y + roomOffset.y;
	const float maxX = (room->columns * TILE_SIZE) - camera->offset.x + roomOffset.x;
	const float maxY = (room->rows * TILE_SIZE) - camera->offset.y + roomOffset.y;
	return (Vector2){
		Clamp(target.x, minX, maxX),
		Clamp(target.y, minY, maxY)
	};
}

static void ClampCamera(Camera2D* camera, Room* room) {
	camera->target = CameraClampedPos(camera, room, camera->target);
}

// Pan a camera from the previous room to the next one when hitting a door.
static void CameraPan(GameContext* context, Level* level, Player* player) {
	if (!level->swappingRoom || level->nextRoom == NULL || level->currentRoom == NULL) {
		LogDebug("Invalid state");
		return;
	}
	float pct = level->playTime > ROOM_CHANGE_TIME ? 100.0f : level->playTime * 100.0f / ROOM_CHANGE_TIME;
	if (pct == 0.0f) {
		// Somehow the room swap time did not start yet.
		return;
	}
	Vector2 endPos = CameraClampedPos(&context->state->camera, level->nextRoom, player->entity.position);
	const float xPosDiff = (endPos.x - context->state->lastCamPos.x) * pct / 100.0f;
	const float yPosDiff = (endPos.y - context->state->lastCamPos.y) * pct / 100.0f;
	context->state->camera.target.x = context->state->lastCamPos.x + xPosDiff;
	context->state->camera.target.y = context->state->lastCamPos.y + yPosDiff;
}

static void UpdateLevelCamera(Camera2D* camera, Level* level, Player* player) {
	camera->target = player->entity.position;
	ClampCamera(camera, level->currentRoom);
}

static void DrawEntity(GameEntity* entity, bool withGizmo, DrawQueue* queue) {
	if (entity == NULL) {
		LogDebug("NULL pointer to entity!");
		return;
	}
	bool doDraw = true;
	if (entity->invuln.active) {
		// Check if we should blink the character.
		const int blinksPerSec = 10;
		const int blinkParts = blinksPerSec * 2;
		const float durationPerPart = entity->invuln.duration / (float)blinkParts;
		const int whichPart = (int)(entity->invuln.elapsed / durationPerPart);
		if (whichPart % 2 == 0) {
			doDraw = false;
		}
	}
	if (entity->sprite.texture != NULL) {
		AddDrawCall(queue, (DrawCall){
			.fun = DRAW_TEXTURE,
			.layer = ENTITY_LAYER + entity->position.y * 10,
			.args = { .texture = {
				.texture = *entity->sprite.texture,
				.source = IsBitSet(entity->dir, 2) ?
					(Rectangle){
						entity->sprite.rect.x,
						entity->sprite.rect.y,
						entity->sprite.rect.width * -1,
						entity->sprite.rect.height
					}
					: entity->sprite.rect
				,
				.position = Vector2Subtract(entity->position, entity->sprite.position),
				.tint = doDraw ? WHITE : (Color){ 255, 255, 255, 10 }
			}}
		});
	} else {
		// Draw red square if no texture loaded for the entity.
		Vector2 pos = Vector2SubtractValue(entity->position, 16);
		AddDrawCall(queue, (DrawCall){
			.fun = DRAW_RECT,
			.layer = ENTITY_LAYER + entity->position.y * 10,
			.args = { .rect = {
				.rec = (Rectangle){ pos.x, pos.y, TILE_SIZE, TILE_SIZE },
				.color = doDraw ? RED : (Color){ 230, 41, 55, 10 }
			}}
		});
	}
	if (withGizmo) {
		AddDrawCall(queue, (DrawCall){
			.fun = DRAW_RECT,
			.layer = GIZMO_LAYER + entity->position.y * 10,
			.args = { .rect = {
				.rec = HitboxWorldPosition(entity),
				.color = (Color){ 135, 60, 190, 80 }
			}}
		});

		// Show current entity direction.
		int endPosX = entity->dir == EAST || entity->dir == NORTHEAST || entity->dir == SOUTHEAST ?
			entity->position.x + 25 : (entity->dir == WEST || entity->dir == NORTHWEST || entity->dir == SOUTHWEST ? entity->position.x - 25 : entity->position.x);
		int endPosY = entity->dir == NORTH || entity->dir == NORTHEAST || entity->dir == NORTHWEST ?
			entity->position.y - 25 : (entity->dir == SOUTH || entity->dir == SOUTHEAST || entity->dir == SOUTHWEST ? entity->position.y + 25 : entity->position.y);
		AddDrawCall(queue, (DrawCall){
			.fun = DRAW_LINE,
			.layer = GIZMO_LAYER + 25 + entity->position.y * 10,
			.args = { .line = {
				.startPosX = entity->position.x,
				.startPosY = entity->position.y,
				.endPosX = endPosX,
				.endPosY = endPosY,
				.color = ORANGE
			}}
		});
	}
}

void DrawAttackCallback(ObjectPool* pool, int index, void* args) {
	if (args == NULL) {
		return;
	}
	ActiveAttack* attack = PoolIndexAddress(pool, index);
	if (attack == NULL || attack->attack == NULL) {
		return;
	}
	if (attack->elapsed >= attack->attack->duration) {
		return;
	}

	DrawAttackCbArgs* cbArgs = (DrawAttackCbArgs*) args;
	Color color = attack->target == T_ENEMY ? (Color){ 0, 208, 8, 210 } : (Color){ 125, 11, 22, 210 };
	//int layer = ENTITY_LAYER + 1 + attack->center.y * 100;
	int layer = GIZMO_LAYER + 2 + attack->center.y * 100;
	if (attack->attack->type == 1) {
		// TODO: Attack animation and sprite check here.
		//if (cbArgs->showGizmos) {
			AddDrawCall(cbArgs->queue, (DrawCall){
				.fun = DRAW_RECT,
				.layer = layer,
				.args = { .rect = {
					.rec = attack->hitbox.rect,
					.color = color
				}}
			});
		//}
	}
	if (attack->attack->type == 2) {
		//if (cbArgs->showGizmos) {
			//DrawCircleV(attack->center, attack->attack->hitbox.radius, (Color){ 125, 11, 22, 210 });
			AddDrawCall(cbArgs->queue, (DrawCall){
				.fun = DRAW_CIRCLE,
				.layer = layer,
				.args = { .circle = {
					.center = attack->center,
					.radius = attack->hitbox.radius,
					.color = color
				}}
			});
		//}
	}
}

void DrawTextCallback(ObjectPool* pool, int index, void* args) {
	if (args == NULL) {
		return;
	}
	ActiveText* text = PoolIndexAddress(pool, index);
	if (text == NULL || text->content == NULL) {
		goto cleanup;
	}
	DrawTextCbArgs* cbArgs = (DrawTextCbArgs*) args;
	if (cbArgs->playTime > text->endTime) {
		// Need to free the text string when we are done with it.
		free(text->content);
		goto cleanup;
	}

	// Text movement vector is text->start to text->end in (text->endTime - text->startTime) time.
	// We just need to translate the position through the elapsed time.
	float elapsed = cbArgs->playTime - text->startTime;
	float pct = elapsed * 100.0f / (text->endTime - text->startTime);
	float yPosDiff = (text->end.y - text->start.y) * pct / 100.0f;
	float xPosDiff = (text->end.x - text->start.x) * pct / 100.0f;
	// TODO: If 2 texts are overlapping, move one a bit? how?
	AddDrawCall(cbArgs->queue, (DrawCall){
		.fun = DRAW_TEXT,
		.layer = EFFECTS_LAYER + text->start.y + yPosDiff,
		.args = { .text = {
			.text = text->content,
			.posX = text->start.x + xPosDiff,
			.posY = text->start.y + yPosDiff,
			.fontSize = text->fontSize,
			.color = text->color
		}}
	});
	return;

	cleanup: RemoveFromPool(pool, index);
}

static void RenderRoomCalls(GameContext* context, Room* room, Player* player, DrawQueue* queue) {
	if (room == NULL || room->tileCount <= 0 || room->rows <= 0 || room->columns == 0) {
		LogDebug("Invalid room state");
		return;
	}

	// Draw room background.
	float camX1 = (context->state->camera.target.x - context->state->camera.offset.x) * context->state->camera.zoom;
	float camY1 = (context->state->camera.target.y - context->state->camera.offset.y) * context->state->camera.zoom;
	const float scale = GetScreenScale();
	const float scaledWidth = (float) GetScreenWidth() / scale;
	const float scaledHeight = (float) GetScreenHeight() / scale;
	float zoomedWidth = scaledWidth;
	float zoomedHeight = scaledHeight;
	if (context->state->camera.zoom != 1.0f) {
		zoomedWidth /= context->state->camera.zoom;
		zoomedHeight /= context->state->camera.zoom;
		camX1 += (scaledWidth - zoomedWidth) / 2;
		camY1 += (scaledHeight - zoomedHeight) / 2;
	}
	Rectangle worldCamera = {
		camX1 - 1,
		camY1 - 1,
		zoomedWidth + 2,
		zoomedHeight + 2
	};

	Color tileColor;
	Rectangle tileRect;
	for (int row = 0; row < room->rows; row++) {
		for (int column = 0; column < room->columns; column++) {
			int index = column + (room->columns * row);
			switch (room->tiles[index].type) {
				case WALL: tileColor = (Color){ 43, 3, 0, 255 }; break;
				case GRASS: tileColor = (Color){ 0, 180, 66, 255 }; break;
				case DOOR: tileColor = BEIGE; break;
				default: tileColor = BROWN;
			}
			Vector2 offsetPos = RoomOffsetPos(room, column, row);
			tileRect = (Rectangle){ offsetPos.x, offsetPos.y, TILE_SIZE, TILE_SIZE };
			if (CheckCollisionRecs(worldCamera, tileRect)) {
				AddDrawCall(queue, (DrawCall){
					.fun = DRAW_RECT,
					.layer = BG_LAYER + row,
					.args = { .rect = {
						.rec = tileRect,
						.color = tileColor
					}}
				});
			}
		}
	}

	// Draw room entities.
	// Enemies.
	if (room->entities != NULL && room->entityCount > 0) {
		for (int i = 0; i < room->entityCount; i++) {
			if (!room->entities[i].active) {
				continue;
			}
			DrawEntity(&room->entities[i].entity, context->options->showGizmos, queue);
			if (context->options->showGizmos) {
				AddDrawCall(queue, (DrawCall){
					.fun = DRAW_CIRCLE,
					.layer = GIZMO_LAYER - 1000 + room->entities[i].entity.position.y * 100,
					.args = { .circle = {
						.center = room->entities[i].entity.position,
						.radius = room->entities[i].activeRadius,
						.color = (Color){ 255, 109, 194, 60 }
					}}
				});
				AddDrawCall(queue, (DrawCall){
					.fun = DRAW_LINE,
					.layer = GIZMO_LAYER + 500 + room->entities[i].entity.position.y * 100,
					.args = { .line = {
						.startPosX = room->entities[i].entity.position.x,
						.startPosY = room->entities[i].entity.position.y,
						.endPosX = player->entity.position.x,
						.endPosY = player->entity.position.y,
						.color = DARKGRAY
					}}
				});
			}
		}
	}
}

static void RenderWorldCalls(
	GameContext* context,
	RenderTexture2D* worldRender,
	Player* player,
	Level* level,
	DrawQueue* queue
) {
	// Draw room background.
	RenderRoomCalls(context, level->currentRoom, player, queue);
	if (level->swappingRoom) {
		RenderRoomCalls(context, level->nextRoom, player, queue);
	}
	// TODO: Pick surrounding rooms somehow?
	// For now zoom out is only available for testing anyway.
	if (!level->swappingRoom && context->state->camera.zoom < 1.0f) {
		if (level->currentRoom != &level->rooms[1]) {
			RenderRoomCalls(context, &level->rooms[1], player, queue);
		}
		if (level->currentRoom != &level->rooms[2]) {
			RenderRoomCalls(context, &level->rooms[2], player, queue);
		}
		if (level->currentRoom != &level->rooms[3]) {
			RenderRoomCalls(context, &level->rooms[3], player, queue);
		}
		if (level->currentRoom != &level->rooms[4]) {
			RenderRoomCalls(context, &level->rooms[4], player, queue);
		}
	}

	// Draw character.
	DrawEntity(&player->entity, context->options->showGizmos, queue);

	// Attacks.
	if (level->attacks.activeItems > 0) {
		DrawAttackCbArgs args = {
			.showGizmos = context->options->showGizmos,
			.queue = queue
		};
		IteratePool(&level->attacks, &DrawAttackCallback, &args);
	}

	// Damage texts.
	if (level->texts.activeItems > 0) {
		DrawTextCbArgs args = {
			.playTime = level->playTime,
			.queue = queue
		};
		IteratePool(&level->texts, &DrawTextCallback, &args);
	}
}

static int CompareDrawCall(const void* a, const void* b) {
	return (((DrawCall*)a)->layer - ((DrawCall*)b)->layer);
}

static void SortDrawCalls(DrawQueue* queue) {
	if (queue->count == 0) {
		return;
	}
	qsort(queue->calls, queue->count, sizeof(DrawCall), CompareDrawCall);
}

static void RenderWorld(
	GameContext* context,
	RenderTexture2D* worldRender,
	Player* player,
	Level* level
) {
	// Create the draw queue.
	// Tiles + entities + player entity + attacks + texts + some extra.
	int maxCalls = level->currentRoom->tileCount + (level->currentRoom->entityCount + 1) * 5 + level->attacks.activeItems + level->texts.activeItems + 32;
	if (level->swappingRoom) {
		maxCalls += level->nextRoom->tileCount + (level->nextRoom->entityCount + 1) * 5;
	}
	// NOTICE For now during debug, we'll see later.
	if (context->state->camera.zoom < 1.0f) {
		maxCalls *= 5;
	}

	DrawQueue* queue = malloc(sizeof(DrawQueue) + sizeof(DrawCall[maxCalls]));
	if (queue == NULL) {
		LogDebug("Failed to allocate DrawQueue!!");
		return;
	}
	queue->max = maxCalls;
	queue->count = 0;

	// Get all the drawing calls for current state.
	// TODO: Do we need to store background tile calls? Can probably just print them out,
	// since they'll always be at the lowest layer and they do not overlap.
	RenderWorldCalls(context, worldRender, player, level, queue);

	// Order the queue by layer.
	SortDrawCalls(queue);

	// First, create the frame for the world on its fixed resolution.
	BeginTextureMode(*worldRender);
	BeginMode2D(context->state->camera);
	ClearBackground(RAYWHITE);

	// Run all draw calls in order.
	DrawCall* call;
	for (int i = 0; i < queue->count; i++) {
		call = &queue->calls[i];
		//LogDebug("Running draw call %d on layer %d", i, call.layer);
		// TODO: dict of enum to function pointer?
		switch (call->fun) {
			case DRAW_RECT:
				DrawRectangleRec(call->args.rect.rec, call->args.rect.color);
				break;
			case DRAW_TEXT:
				if (call->args.text.text != NULL) {
					DrawText(
						call->args.text.text,
						call->args.text.posX,
						call->args.text.posY,
						call->args.text.fontSize,
						call->args.text.color
					);
				}
				break;
			case DRAW_TEXTURE:
				DrawTextureRec(
					call->args.texture.texture,
					call->args.texture.source,
					call->args.texture.position,
					call->args.texture.tint
				);
				break;
			case DRAW_LINE:
				DrawLine(call->args.line.startPosX, call->args.line.startPosY, call->args.line.endPosX, call->args.line.endPosY, call->args.line.color);
				break;
			case DRAW_CIRCLE:
				DrawCircleV(call->args.circle.center, call->args.circle.radius, call->args.circle.color);
				break;
		}
	}

	// We finished creating the frame as a texture.
	free(queue);
	EndMode2D();
	EndTextureMode();
}

void ResizeWindow(GameContext* context, ScreenSize newSize) {
	SetWindowSize(newSize.width, newSize.height);
	CalculateScreenSize(context);
}

void CalculateScreenSize(GameContext* context) {
	float screenWidth = (float) GetScreenWidth();
	float screenHeight = (float) GetScreenHeight();
	float gameScreenWidth = (float) WORLD_SIZE_WIDTH;
	float gameScreenHeight = (float) WORLD_SIZE_HEIGHT;
	float scale = fmaxf(fminf(screenWidth / gameScreenWidth, screenHeight / gameScreenHeight), 1.0f);
	context->options->screenSize = (Rectangle){
		(screenWidth - (gameScreenWidth * scale)) * 0.5f,
		(screenHeight - (gameScreenHeight * scale)) * 0.5f,
		gameScreenWidth * scale,
		gameScreenHeight * scale
	};
}

static void RenderScreen(
	GameContext* context,
	RenderTexture2D* worldRender,
	Player* player
) {
	// Draw the texture in the actual screen resolution.
	BeginDrawing();
	ClearBackground(BLACK);

	const Rectangle renderSource = { 0.0f, 0.0f, WORLD_SIZE_WIDTH, -WORLD_SIZE_HEIGHT };
	DrawTexturePro(worldRender->texture, renderSource, context->options->screenSize, (Vector2){ 0, 0 }, 0.0f, WHITE);

	// Draw UI elements.
	DrawFPS(GetScreenWidth() - 95, 10);
	DrawText(TextFormat("Seed: %s", context->state->seedStr), 10, 10, 25, PURPLE);

	// Player status elements in the UI.
	if (player != NULL) {
		// HP Bar.
		int screenHeight = GetScreenHeight();
		DrawRectangle(20, screenHeight - 40, 255, 25, GOLD);
		Color hpColor = DARKGREEN;
		int hpBarWidth = 251;
		int maxHp = player->entity.maxHealth ? player->entity.maxHealth : player->entity.health;
		if (player->entity.health == 0) {
			hpColor = BLACK;
		} else if (player->entity.health < maxHp) {
			DrawRectangle(22, screenHeight - 38, hpBarWidth, 21, BLACK);
			int pct = player->entity.health * 100 / maxHp;
			hpBarWidth = 251 * pct / 100;
			if (pct < 95) {
				hpColor = (pct > 75) ? YELLOW : (pct > 32 ? ORANGE : RED);
			}
		}
		DrawRectangle(22, screenHeight - 38, hpBarWidth, 21, hpColor);
		DrawText(TextFormat("%d HP", player->entity.health), 285, screenHeight - 40, 22, DARKGREEN);
	}

	// Mouse pointer.
	if (!context->options->systemCursor) {
		DrawTextureEx(*context->options->cursor.texture, GetMousePosition(), 0.0f, GetScreenScale(), WHITE);
	}

	// We are done, show the frame.
	EndDrawing();
}

static void RenderLogo(
	GameContext* context,
	RenderTexture2D* worldRender
) {
	// Render logo state in world render size.
	BeginTextureMode(*worldRender);
	ClearBackground(RAYWHITE);
	const char* text = "Mantis Shrimp";
	int fontSize = 42;
	int textPxSize = MeasureText(text, fontSize);
	DrawText(text, worldRender->texture.width / 2 - textPxSize / 2, worldRender->texture.height / 2 - fontSize / 2, fontSize, BLACK);

	// Fade in logo 2s, fade out
	if (context->state->elapsed <= LOGO_FADE_TIME) {
		char t = (char)(255.0f * (LOGO_FADE_TIME - context->state->elapsed));
		DrawRectangle(0, 0, worldRender->texture.width, worldRender->texture.height, (Color){ 0, 0, 0, t});
	}
	float fadeDiff = LOGO_DURATION - LOGO_FADE_TIME;
	if (context->state->elapsed >= fadeDiff) {
		char t = context->state->elapsed < LOGO_DURATION ? (char)(255.0f * -(fadeDiff - context->state->elapsed)) : 255;
		DrawRectangle(0, 0, worldRender->texture.width, worldRender->texture.height, (Color){ 0, 0, 0, t});
	}
	EndTextureMode();

	// Show frame in final size.
	BeginDrawing();
	ClearBackground(BLACK);
	DrawTexturePro(
		worldRender->texture,
		(Rectangle){ 0, 0, WORLD_SIZE_WIDTH, -WORLD_SIZE_HEIGHT },
		context->options->screenSize,
		(Vector2){ 0, 0 }, 0.0f,
		WHITE
	);
	EndDrawing();
}

static void RenderTitle(
	GameContext* context,
	RenderTexture2D* worldRender
) {
	BeginTextureMode(*worldRender);
	ClearBackground(RAYWHITE);
	const char* text = "This is a title";
	int fontSize = 35;
	int textPxSize = MeasureText(text, fontSize);
	DrawText(text, worldRender->texture.width / 2 - textPxSize / 2, worldRender->texture.height / 2 - fontSize / 2, fontSize, BLACK);
	EndTextureMode();

	BeginDrawing();
	ClearBackground(BLACK);
	DrawTexturePro(
		worldRender->texture,
		(Rectangle){ 0, 0, WORLD_SIZE_WIDTH, -WORLD_SIZE_HEIGHT },
		context->options->screenSize,
		(Vector2){ 0, 0 }, 0.0f,
		WHITE
	);
	EndDrawing();
}

static void RenderLevel(
	GameContext* context,
	RenderTexture2D* worldRender
) {
	Level* level = GetLevel();
	Player* player = GetPlayer();
	if (level->swappingRoom) {
		CameraPan(context, level, player);
	} else {
		UpdateLevelCamera(&context->state->camera, level, player);
	}
	RenderWorld(context, worldRender, player, level);
	RenderScreen(context, worldRender, player);
}

void Render(
	GameContext* context,
	RenderTexture2D* worldRender
) {
	// TODO? If all renders are the same args, can simply use a function pointer.
	switch (context->state->screen) {
		case GAMEPLAY: return RenderLevel(context, worldRender);
		case LOGO: return RenderLogo(context, worldRender);
		case TITLE: return RenderTitle(context, worldRender);
		default: return;
	}
}

void LoadCustomCursor(GameOptions* options) {
	Texture2D* cursorTexture = GetTexture(CURSOR_TEXTURE);
	options->cursor = (Sprite){
		.texture = cursorTexture,
		.position = { 0.0f, 0.0f },
		.rect = { 0.0f, 0.0f, cursorTexture->width, cursorTexture->height },
	};
}

void SetupGameWindow(GameContext* context) {
	int monitor = GetCurrentMonitor();
	int monitorWidth = GetMonitorWidth(monitor);
	int monitorHeight = GetMonitorHeight(monitor);
	int scale = (int) Clamp(fmin(
		monitorWidth / WORLD_SIZE_WIDTH,
		monitorHeight / WORLD_SIZE_HEIGHT
	), 1, 10);
	ScreenSize newSize = { WORLD_SIZE_WIDTH * scale, WORLD_SIZE_HEIGHT * scale };
	ResizeWindow(context, newSize);
	SetWindowPosition((monitorWidth - newSize.width) / 2, (monitorHeight - newSize.height) / 2);
	CalculateScreenSize(context);
}
