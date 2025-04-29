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

static void ClampCamera(Camera2D* camera, Level* level) {
	if (level == NULL) {
		LogDebug("Invalid parameters");
		return;
	}
	Room* room = &level->rooms[level->currentRoom];
	if (room->tileCount == 0 || room->tilesPerRow == 0) {
		LogDebug("Invalid room tiles");
		return;
	}
	const float minX = 0.0f + camera->offset.x;
	const float minY = 0.0f + camera->offset.y;
	const float maxX = (room->tilesPerRow * TILE_SIZE) - camera->offset.x;
	const float maxY = ((room->tileCount / room->tilesPerRow) * TILE_SIZE) - camera->offset.y;
	camera->target.x = Clamp(camera->target.x, minX, maxX);
	camera->target.y = Clamp(camera->target.y, minY, maxY);
}

static void UpdateLevelCamera(Camera2D* camera, Level* level, Player* player) {
	camera->target = player->entity.position;
	ClampCamera(camera, level);
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
		/*DrawTextureRec(
			*entity->sprite.texture,
			// Hacky flip for going left for now.
			IsBitSet(entity->dir, 2) ?
				(Rectangle){
					entity->sprite.rect.x,
					entity->sprite.rect.y,
					entity->sprite.rect.width * -1,
					entity->sprite.rect.height
				}
				: entity->sprite.rect,
			// For a GameEntity, its Sprite position is relative to the entity position.
			Vector2Subtract(entity->position, entity->sprite.position),
			doDraw ? WHITE : (Color){ 255, 255, 255, 10 }
		);*/
		AddDrawCall(queue, (DrawCall){
			.fun = DRAW_TEXTURE,
			.layer = ENTITY_LAYER + entity->position.y * 100,
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
		//DrawRectangleV(Vector2SubtractValue(entity->position, 16), (Vector2){ 32, 32 }, doDraw ? RED : (Color){ 230, 41, 55, 10 });
		Vector2 pos = Vector2SubtractValue(entity->position, 16);
		AddDrawCall(queue, (DrawCall){
			.fun = DRAW_RECT,
			.layer = ENTITY_LAYER + entity->position.y * 100,
			.args = { .rect = {
				.rec = (Rectangle){ pos.x, pos.y, TILE_SIZE, TILE_SIZE },
				.color = doDraw ? RED : (Color){ 230, 41, 55, 10 }
			}}
		});
	}
	if (withGizmo) {
		//DrawRectangleRec(HitboxWorldPosition(entity), (Color){ 135, 60, 190, 80 });
		AddDrawCall(queue, (DrawCall){
			.fun = DRAW_RECT,
			.layer = GIZMO_LAYER + entity->position.y * 100,
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
		//DrawLine(entity->position.x, entity->position.y, endPosX, endPosY, ORANGE);
		AddDrawCall(queue, (DrawCall){
			.fun = DRAW_LINE,
			.layer = GIZMO_LAYER + 25 + entity->position.y * 100,
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
	if (attack->attack->type == 1) {
		// TODO: Attack animation here.
		//if (cbArgs->showGizmos) {
			Color color = attack->target == T_ENEMY ? (Color){ 0, 208, 8, 210 } : (Color){ 125, 11, 22, 210 };
			//DrawRectangleRec(attack->hitbox, color);
			AddDrawCall(cbArgs->queue, (DrawCall){
				.fun = DRAW_RECT,
				.layer = ENTITY_LAYER + 1 + attack->center.y * 100,
				.args = { .rect = {
					.rec = attack->hitbox,
					.color = color
				}}
			});
		//}
	}
	if (attack->attack->type == 2) {
		// TODO
		//if (cbArgs->showGizmos) {
			//DrawCircleV(attack->center, attack->attack->hitbox.radius, (Color){ 125, 11, 22, 210 });
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
	//DrawText(text->content, text->start.x + xPosDiff, text->start.y + yPosDiff, text->fontSize, text->color);
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

static void RenderWorldCalls(
	GameContext* context,
	RenderTexture2D* worldRender,
	Player* player,
	Level* level,
	DrawQueue* queue
) {
	// Draw level background.
	float camX1 = context->state->camera.target.x - context->state->camera.offset.x;
	float camY1 = context->state->camera.target.y - context->state->camera.offset.y;
	float scale = GetScreenScale();
	Rectangle worldCamera = {
		camX1 - 1,
		camY1 - 1,
		((float)GetScreenWidth() / context->state->camera.zoom / scale) + 2,
		((float)GetScreenHeight() / context->state->camera.zoom / scale) + 2
	};

	// Run bounds check for camera:
	// Check if the room is bigger than the biggest room that can be shown in camera.
	// If the room is not big enough, there's no need to check for camera bound.
	//const float widthLimit = (WORLD_SIZE_WIDTH / tileSize) + 1;
	//const float heightLimit = (WORLD_SIZE_HEIGHT / tileSize) + 1;
	if (level != NULL) {
		Room* room = &level->rooms[level->currentRoom];
		if (room->tileCount > 0 && room->tilesPerRow > 0) {
			Color tileColor;
			Rectangle tileRect;
			const int columns = room->tileCount / room->tilesPerRow;
			for (int x = 0; x < room->tilesPerRow; x++) {
				for (int y = 0; y < columns; y++) {
					int index = y + (columns * x);
					switch (room->tiles[index].type) {
						case WALL: tileColor = (Color){ 43, 3, 0, 255 }; break;
						case GRASS: tileColor = (Color){ 0, 180, 66, 255 }; break;
						case DOOR: tileColor = BEIGE; break;
						default: tileColor = BROWN;
					}
					tileRect = (Rectangle){ x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE };
					if (CheckCollisionRecs(worldCamera, tileRect)) {
						AddDrawCall(queue, (DrawCall){
							.fun = DRAW_RECT,
							.layer = BG_LAYER + y,
							.args = { .rect = {
								.rec = tileRect,
								.color = tileColor
							}}
						});
					}
				}
			}
		}
	}

	// Draw character.
	DrawEntity(&player->entity, context->options->showGizmos, queue);

	// Draw level entities.
	if (level != NULL) {
		Room* room = &level->rooms[level->currentRoom];
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
	Room* room = &level->rooms[level->currentRoom];
	int maxCalls = room->tileCount + (room->entityCount + 1) * 5 + level->attacks.activeItems + level->texts.activeItems + 32;
	DrawQueue* queue = malloc(sizeof(DrawQueue) + sizeof(DrawCall[maxCalls]));
	if (queue == NULL) {
		LogDebug("Failed to allocate DrawQueue!!");
		return;
	}
	queue->max = maxCalls;
	queue->count = 0;

	// Get all the drawing calls for current state.
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
	Player* player = GetPlayer();
	Level* level = GetLevel();
	UpdateLevelCamera(&context->state->camera, level, player);
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
