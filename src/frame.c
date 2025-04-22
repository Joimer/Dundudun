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

const ScreenSize worldSize = { 480, 270 };
const ScreenSize resolutions[5] = {
	{ .width = 800, .height = 450 },
	{ .width = 960, .height = 540 },
	{ .width = 1280, .height = 720 },
	{ .width = 1760, .height = 990 },
	{ .width = 1920, .height = 1080 },
};

void DrawCursor(GameContext* context) {
	context->options->cursor.position = GetWorldMousePos(context);
	DrawTexture(
		*context->options->cursor.texture,
		(int)context->options->cursor.position.x,
		(int)context->options->cursor.position.y,
		WHITE
	);
}

void DrawSprite(Sprite* sprite) {
	DrawTextureRec(*sprite->texture, sprite->rect, sprite->position, WHITE);
}

static void DrawEntity(GameEntity* entity, bool withGizmo) {
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
		DrawTextureRec(
			*entity->sprite.texture,
			entity->sprite.rect,
			// For a GameEntity, its Sprite position is relative to the entity position.
			Vector2Subtract(entity->position, entity->sprite.position),
			doDraw ? WHITE : (Color){ 255, 255, 255, 10 }
		);
	} else {
		// Draw red square if no texture loaded for the entity.
		DrawRectangleV(Vector2SubtractValue(entity->position, 16), (Vector2){ 32, 32 }, doDraw ? RED : (Color){ 230, 41, 55, 10 });
	}
	if (withGizmo) {
		DrawRectangleRec(HitboxWorldPosition(entity), (Color){ 230, 41, 55, 60 });

		// Show current entity direction.
		int endPosX = entity->dir == EAST || entity->dir == NORTHEAST || entity->dir == SOUTHEAST ?
			entity->position.x + 25 : (entity->dir == WEST || entity->dir == NORTHWEST || entity->dir == SOUTHWEST ? entity->position.x - 25 : entity->position.x);
		int endPosY = entity->dir == NORTH || entity->dir == NORTHEAST || entity->dir == NORTHWEST ?
			entity->position.y - 25 : (entity->dir == SOUTH || entity->dir == SOUTHEAST || entity->dir == SOUTHWEST ? entity->position.y + 25 : entity->position.y);
		DrawLine(entity->position.x, entity->position.y, endPosX, endPosY, ORANGE);
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

	bool showGizmos = *((bool*) args);
	if (attack->attack->type == 1) {
		// TODO: Attack animation here.
		if (showGizmos) {
			Color color = attack->target == T_ENEMY ? (Color){ 0, 208, 8, 210 } : (Color){ 125, 11, 22, 210 };
			DrawRectangleRec(attack->hitbox, color);
		}
	}
	if (attack->attack->type == 2) {
		// TODO: Attack animation here.
		if (showGizmos) {
			DrawCircleV(attack->center, attack->attack->hitbox.radius, (Color){ 125, 11, 22, 210 });
		}
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
	float playTime = *((float*) args);
	if (playTime > text->endTime) {
		// Need to free the text string when we are done with it.
		free(text->content);
		goto cleanup;
	}

	// Text movement vector is text->start to text->end in (text->endTime - text->startTime) time.
	// We just need to translate the position through the elapsed time.
	float elapsed = playTime - text->startTime;
	float pct = elapsed * 100.0f / (text->endTime - text->startTime);
	float yPosDiff = (text->end.y - text->start.y) * pct / 100.0f;
	float xPosDiff = (text->end.x - text->start.x) * pct / 100.0f;
	// TODO: If 2 texts are overlapping, move one a bit? how?
	DrawText(text->content, text->start.x + xPosDiff, text->start.y + yPosDiff, text->fontSize, text->color);
	return;

	cleanup: RemoveFromPool(pool, index);
}

static void RenderWorld(
	GameContext *context,
	RenderTexture2D *worldRender,
	Player *player,
	Level* level
) {
	// First, create the frame for the world on its fixed resolution.
	BeginTextureMode(*worldRender);
	BeginMode2D(context->state->camera);
	ClearBackground(RAYWHITE);

	// Draw level background.
	const int tileSize = 32;
	const int offset = 10;
	Color tileColor;
	for (int x = 0; x < 50; x++) {
		for (int y = 0; y < 50; y++) {
			if (x == 0 || y == 0 || x == 49 || y == 49) {
				tileColor = BLACK;
			} else {
				if ((x % 2 == 0 && y % 2 != 0) || (x % 2 != 0 && y % 2 == 0)) {
					tileColor = BEIGE;
				} else {
					tileColor = BROWN;
				}
			}
			DrawRectangle(x * tileSize, y * tileSize, tileSize, tileSize, tileColor);
			DrawText(TextFormat("%d,%d", x, y), (x * tileSize) + offset, (y * tileSize) + offset, 10, DARKBLUE);
		}
	}

	// Draw character.
	DrawEntity(&player->entity, context->options->showGizmos);

	if (level != NULL && level->tiles != NULL && level->tileCount > 0) {
		// TODO: Tiles from generated level.
	}

	// Draw level entities.
	if (level != NULL) {
		// Enemies.
		if (level->entities != NULL && level->entityCount > 0) {
			for (int i = 0; i < level->entityCount; i++) {
				if (!level->entities[i].active) {
					continue;
				}
				DrawEntity(&level->entities[i].entity, context->options->showGizmos);
				if (context->options->showGizmos) {
					DrawCircleV(level->entities[i].entity.position, level->entities[i].activeRadius, (Color){ 255, 109, 194, 60 });
					DrawLineV(level->entities[i].entity.position, player->entity.position, DARKGRAY);
				}
			}
		}

		// Attacks.
		if (level->attacks.activeItems > 0) {
			IteratePool(&level->attacks, &DrawAttackCallback, &context->options->showGizmos);
		}

		// Damage texts.
		if (level->texts.activeItems > 0) {
			IteratePool(&level->texts, &DrawTextCallback, &level->playTime);
		}
	}

	// Draw custom cursor last.
	if (!context->options->systemCursor) {
		DrawCursor(context);
	}

	// We finished creating the frame as a texture.
	EndMode2D();
	EndTextureMode();
}

void ResizeWindow(GameContext* context, ScreenSize newSize) {
	SetWindowSize(newSize.width, newSize.height);
	CalculateScreenSize(context);
}

int gcd(int a, int b) {
	int remainder = a % b;

	if (remainder == 0) {
		return b;
	}

	return gcd(b, remainder);
}

void CalculateScreenSize(GameContext* context) {
	float screenWidth = (float) GetScreenWidth();
	float screenHeight = (float) GetScreenHeight();
	float gameScreenWidth = (float) worldSize.width;
	float gameScreenHeight = (float) worldSize.height;
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

	const Rectangle renderSource = { 0.0f, 0.0f, worldSize.width, -worldSize.height };
	const Vector2 renderOrigin = { 0, 0 };
	DrawTexturePro(worldRender->texture, renderSource, context->options->screenSize, renderOrigin, 0.0f, WHITE);

	// Draw UI elements.
	DrawFPS(GetScreenWidth() - 95, 10);
	DrawText(TextFormat("Seed: %s", context->state->seedStr), 10, 10, 25, PURPLE);
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

	// We are done, show the frame.
	EndDrawing();
}

void Render(
	GameContext* context,
	RenderTexture2D* worldRender,
	Player* player,
	Level* level
) {
	RenderWorld(context, worldRender, player, level);
	RenderScreen(context, worldRender, player);
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
		monitorWidth / worldSize.width,
		monitorHeight / worldSize.height
	), 1, 10);
	ScreenSize newSize = { worldSize.width * scale, worldSize.height * scale };
	ResizeWindow(context, newSize);
	SetWindowPosition((monitorWidth - newSize.width) / 2, (monitorHeight - newSize.height) / 2);
	CalculateScreenSize(context);
}
