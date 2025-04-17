#include <raylib.h>
#include <string.h>
#include <raymath.h>
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

Vector2 GetWorldMousePos(GameContext* context) {
	Vector2 mousePos = GetMousePosition();
	float worldHeight = (float) worldSize.height;
	float worldWidth = (float) worldSize.width;
	if (
		context->options->screenSize.width != worldWidth
		|| context->options->screenSize.height != worldHeight
	) {
		mousePos.x *= worldWidth / context->options->screenSize.width;
		mousePos.y *= worldHeight / context->options->screenSize.height;
	}
	Vector2 realWorld = GetScreenToWorld2D(mousePos, context->state->camera);

	return realWorld;
}

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

void DrawEntity(GameEntity* entity, bool withHitbox) {
	if (entity == NULL) {
		LogDebug("NULL pointer to entity!");
		return;
	}
	if (entity->sprite.texture != NULL) {
		DrawTextureRec(
			*entity->sprite.texture,
			entity->sprite.rect,
			// For a GameEntity, its Sprite position is relative to the entity position.
			Vector2Subtract(entity->position, entity->sprite.position),
			WHITE
		);
	} else {
		// Draw red square if no texture loaded for the entity.
		DrawRectangleV(Vector2SubtractValue(entity->position, 16), (Vector2){ 32, 32 }, RED);
	}
	if (withHitbox) {
		DrawRectangle(
			entity->position.x + entity->hitbox.x,
			entity->position.y + entity->hitbox.y,
			entity->hitbox.width,
			entity->hitbox.height,
			(Color){ 230, 41, 55, 90 }
		);
	}
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
		// TODO
	}
	// Draw level entities.
	if (level != NULL && level->entities != NULL && level->entityCount > 0) {
		for (int i = 0; i < level->entityCount; i++) {
			DrawEntity(&level->entities[i].entity, context->options->showGizmos);
			if (context->options->showGizmos) {
				DrawCircleV(level->entities[i].entity.position, level->entities[i].activeRadius, (Color){ 255, 109, 194, 90 });
			}
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
	RenderTexture2D* worldRender
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
	RenderScreen(context, worldRender);
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
		floor(monitorWidth / worldSize.width),
		floor(monitorHeight / worldSize.height)
	), 1, 10);
	ScreenSize newSize = { worldSize.width * scale, worldSize.height * scale };
	ResizeWindow(context, newSize);
	SetWindowPosition((monitorWidth - newSize.width) / 2, (monitorHeight - newSize.height) / 2);
	CalculateScreenSize(context);
}
