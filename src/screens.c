#include "control.h"
#include "frame.h"
#include "game.h"
#include "screens.h"
#include "level.h"
#include "text.h"
#include <raylib.h>

void LoadNextScreen(GameContext* context, GameScreen next) {
	if (next == GAMEPLAY) {
		SetupLevel(context);
	}
	context->state->menuOption = 0;
	context->state->menuContext = 0;
	context->state->screen = next;
}

static bool PressedForNextScene(float elapsed) {
	return elapsed > MIN_ELAPSED_FOR_INPUT && (
		IsActionOnce(ACCEPT)
		|| IsActionOnce(CANCEL)
		|| IsActionOnce(ACTION_ATT)
		|| IsActionOnce(ACTION_SWAP)
		|| IsActionOnce(ACTION_BOMB)
		|| IsActionOnce(ACTION_DASH)
	);
}

void UpdateLogo(GameContext* context) {
	if (
		context->state->elapsed > LOGO_DURATION
		|| PressedForNextScene(context->state->elapsed)
	) {
		LoadNextScreen(context, TITLE);
	}
}

void RenderLogo(
	GameContext* context,
	RenderTexture2D* worldRender
) {
	// Render logo state in world render size.
	BeginTextureMode(*worldRender);
	ClearBackground(RAYWHITE);
	const char* text = "Mantis Shrimp";
	int fontSize = 42;
	int textPxSize = MeasureText(text, fontSize);
	DrawColourText(text, worldRender->texture.width / 2 - textPxSize / 2, worldRender->texture.height / 2 - fontSize / 2, fontSize, BLACK);

	// Fade in logo 2s, fade out
	if (context->state->elapsed <= LOGO_FADE_TIME) {
		unsigned char t = (unsigned char)(255.0f * (LOGO_FADE_TIME - context->state->elapsed));
		DrawRectangle(0, 0, worldRender->texture.width, worldRender->texture.height, (Color){ 0, 0, 0, t});
	}
	float fadeDiff = LOGO_DURATION - LOGO_FADE_TIME;
	if (context->state->elapsed >= fadeDiff) {
		unsigned char t = context->state->elapsed < LOGO_DURATION ? (unsigned char)(255.0f * -(fadeDiff - context->state->elapsed)) : 255;
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

void UpdateTitle(GameContext* context) {
	bool menuMoved = false;
	if (IsActionOnce(GO_UP) || IsActionOnce(ACTION_ATT_DUP)) {
		context->state->menuOption--;
		menuMoved = true;
	}
	if (IsActionOnce(GO_DOWN) || IsActionOnce(ACTION_ATT_DDOWN)) {
		context->state->menuOption++;
		menuMoved = true;
	}
	if (context->state->menuOption < 0) {
		context->state->menuOption = 3;
	}
	if (context->state->menuOption >= 4) {
		context->state->menuOption = 0;
	}
	if (!menuMoved) {
		if (IsActionOnce(ACCEPT) || IsActionOnce(ACTION_ATT)) {
			switch (context->state->menuOption) {
				case 1: break;
				case 2: break;
				case 3: context->state->shouldClose = true; break;
				default: return LoadNextScreen(context, GAMEPLAY);
			}
		}
	}
}

void RenderTitle(
	GameContext* context,
	RenderTexture2D* worldRender
) {
	// TODO: Skip this render when there are no changes on menu.
	BeginTextureMode(*worldRender);
	ClearBackground(RAYWHITE);
	// Game title.
	const int fontSize = 35;
	const char* text = GAME_NAME;
	const int textPxSize = MeasureText(text, fontSize);

	// Game options.
	const int menuSize = 20;
	const char* newGame = GetText(context->options->lang, M_NEW_GAME);
	const int ngPxSize = MeasureText(newGame, menuSize);
	const char* cont = GetText(context->options->lang, M_CONTINUE);
	const int contPxSize = MeasureText(cont, menuSize);
	const char* options = GetText(context->options->lang, M_OPTIONS);
	const int optPxSize = MeasureText(options, menuSize);
	const char* exitGame = GetText(context->options->lang, M_EXIT);
	const int exitPxSize = MeasureText(exitGame, menuSize);
	const int middleX = worldRender->texture.width / 2;
	const int middleY = worldRender->texture.height / 2;

	// Draw the strings on the corresponding positions and finish up game world size texture.
	const int separation = 5;
	DrawColourText(text, middleX - textPxSize / 2, worldRender->texture.height / 10, fontSize, BLACK);
	const int ngY = middleY;
	const int ngX = middleX - ngPxSize / 2;
	DrawColourText(newGame, ngX, ngY, menuSize, BLACK);
	const int contY = middleY + menuSize + separation;
	const int contX = middleX - contPxSize / 2;
	DrawColourText(cont, contX, contY, menuSize, BLACK);
	const int optsY = middleY + menuSize * 2 + separation * 2;
	const int optsX = middleX - optPxSize / 2;
	DrawColourText(options, optsX, optsY, menuSize, BLACK);
	const int exitY = middleY + menuSize * 3 + separation * 3;
	const int exitX = middleX - exitPxSize / 2;
	DrawColourText(exitGame, exitX, exitY, menuSize, BLACK);
	int selectionYPos, selectionXPos;
	switch (context->state->menuOption) {
		case 1:
			selectionYPos = contY;
			selectionXPos = contX;
			break;
		case 2:
			selectionYPos = optsY;
			selectionXPos = optsX;
			break;
		case 3:
			selectionYPos = exitY;
			selectionXPos = exitX;
			break;
		default:
			selectionYPos = ngY;
			selectionXPos = ngX;
			break;
	}
	DrawColourText("> ", selectionXPos - menuSize, selectionYPos, menuSize, BLACK);
	const char* version = "0.0.71";
	const int vfsize = 12;
	const int versionPxSize = MeasureText(version, vfsize);
	DrawColourText(version, WORLD_SIZE_WIDTH - versionPxSize - vfsize - separation, WORLD_SIZE_HEIGHT - vfsize - separation, vfsize, BLACK);
	EndTextureMode();

	// TODO: UI in game is being done without this world size thingie, uhoh
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
