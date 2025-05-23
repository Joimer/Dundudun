/*********************************************
 * Game scenes config, rendering, and logic. *
 *********************************************/

#ifndef SCREENS_H
#define SCREENS_H

#include <raylib.h>

#define LOGO_DURATION 4.0f
#define LOGO_FADE_TIME 1.0f
#define MIN_ELAPSED_FOR_INPUT 0.1f

// These are the screens the game can be in.
typedef enum { LOADING = -1, LOGO = 0, TITLE, OPTIONS, GAMEPLAY, ENDING } GameScreen;

// Defined in game.h
struct GameContext;

// We put the non game scenes rendering in screens.c to keep clutter to a minimum on frame.c
void RenderTitle(struct GameContext* context, RenderTexture2D* worldRender);
void RenderLogo(struct GameContext* context, RenderTexture2D* worldRender);
void UpdateLogo(struct GameContext* context);
void UpdateTitle(struct GameContext* context);

#endif
