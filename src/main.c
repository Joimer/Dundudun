#include <raylib.h>
#include <raymath.h>
#include "game.h"
#include "control.h"
#include "frame.h"
#include "character.h"
#include "level.h"
#include "resource.h"

#define GAME_NAME "Dundudun"

int main(void) {
	// Default game options.
	// TODO: Load game config from options file.
	GameOptions options = {
		.dashMode = DIRECTIONAL,
		.targetFps = 150,
		.showHitbox = false,
		.screenSize = (Rectangle){ 0, 0, 1920, 1080 }
	};

	// Initial game window configuration.
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	InitWindow(worldSize.width, worldSize.height, GAME_NAME);
	SetTargetFPS(options.targetFps);
	InitAudioDevice();

	// Set up the initial game state.
	GameState gameState = {
		.currentScreen = LOGO,
		.nextScreen = TITLE,
		// 2d World camera.
		.camera = {
			.target = (Vector2){ initialPos.x, initialPos.y },
			.offset = (Vector2){ worldSize.width / 2.0f, worldSize.height / 2.0f },
			.zoom = 1.0f
		}
	};

	// The game context that all functions need to decide an outcome.
	GameContext context = { &gameState, &options };

	// Adapt initial Window to user monitor size, unless specific setting stored (TODO).
	SetupGameWindow(&context);

	return RunGame(&context);
}
