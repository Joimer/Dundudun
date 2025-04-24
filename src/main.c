#include <raylib.h>
#include "game.h"
#include "frame.h"

#define GAME_NAME "Dundudun"

int main(void) {
	// Default game options.
	// TODO: Load game config from options file.
	GameOptions options = {
		.dashMode = DIRECTIONAL,
		.targetFps = 150,
		.showGizmos = true,
		.screenSize = (Rectangle){ 0, 0, 1920, 1080 }
	};

	// Initial game window configuration.
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	InitWindow(WORLD_SIZE_WIDTH, WORLD_SIZE_HEIGHT, GAME_NAME);
	SetTargetFPS(options.targetFps);
	InitAudioDevice();

	// Set up the initial game state.
	GameState gameState = {
		//.currentScreen = LOGO,
		.currentScreen = GAMEPLAY,
		.nextScreen = TITLE,
		// 2d World camera.
		.camera = {
			.target = (Vector2){ initialPos.x, initialPos.y },
			.offset = (Vector2){ WORLD_SIZE_WIDTH / 2.0f, WORLD_SIZE_HEIGHT / 2.0f },
			.zoom = 1.0f
		}
	};

	// The game context that all functions need to decide an outcome.
	GameContext context = { &gameState, &options };

	// Adapt initial Window to user monitor size, unless specific setting stored (TODO).
	SetupGameWindow(&context);

	return RunGame(&context);
}
