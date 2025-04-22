// Frame construction and graphic management.

#ifndef FRAME_H
#define FRAME_H

#include <raylib.h>
#include "game.h"
#include "character.h"
#include "level.h"

typedef struct {
	int width;
	int height;
} ScreenSize;

extern const ScreenSize resolutions[5];
extern const ScreenSize worldSize;

void ResizeWindow(GameContext* context, ScreenSize newSize);
void CalculateScreenSize(GameContext* context);
void LoadCustomCursor(GameOptions* options);
void SetupGameWindow(GameContext* context);

void Render(
	GameContext* context,
	RenderTexture2D* render,
	Player* player,
	Level* level
);

// Drawing game related objects.
void DrawCursor(GameContext* context);
void DrawSprite(Sprite* sprite);

#endif
