// Frame construction and graphic management.

#ifndef FRAME_H
#define FRAME_H

#include <raylib.h>
#include "game.h"
#include "character.h"

typedef struct {
	int width;
	int height;
} ScreenSize;

extern const ScreenSize resolutions[5];

#define WORLD_SIZE_WIDTH 480
#define WORLD_SIZE_HEIGHT 270

#define BG_LAYER 10000
#define ENTITY_LAYER 20000
#define EFFECTS_LAYER 30000
#define GIZMO_LAYER 40000
#define UI_LAYER 50000

#define ROOM_CHANGE_TIME 0.5f

typedef enum { DRAW_RECT, DRAW_TEXT, DRAW_TEXTURE, DRAW_LINE, DRAW_CIRCLE } DrawFn;

typedef struct {
	Rectangle rec;
	Color color;
} DrawRectArgs;

typedef struct {
	const char *text;
	int posX;
	int posY;
	int fontSize;
	Color color;
} DrawTextArgs;

typedef struct {
	Texture2D texture;
	Rectangle source;
	Vector2 position;
	Color tint;
} DrawTextureArgs;

typedef struct {
	int startPosX;
	int startPosY;
	int endPosX;
	int endPosY;
	Color color;
} DrawLineArgs;

typedef struct {
	Vector2 center;
	float radius;
	Color color;
} DrawCircleArgs;

typedef union {
	DrawRectArgs rect;
	DrawTextArgs text;
	DrawTextureArgs texture;
	DrawLineArgs line;
	DrawCircleArgs circle;
} DrawCallArgs;

typedef struct {
	DrawFn fun;
	int layer;
	DrawCallArgs args;
} DrawCall;

typedef struct {
	size_t count;
	size_t max;
	DrawCall calls[];
} DrawQueue;

typedef struct {
	bool showGizmos;
	DrawQueue* queue;
} DrawAttackCbArgs;

typedef struct {
	float playTime;
	DrawQueue* queue;
} DrawTextCbArgs;

void ResizeWindow(GameContext* context, ScreenSize newSize);
void CalculateScreenSize(GameContext* context);
void LoadCustomCursor(GameOptions* options);
void SetupGameWindow(GameContext* context);
void Render(GameContext* context, RenderTexture2D* render);

#endif
