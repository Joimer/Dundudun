#include <stdlib.h>
#include <raylib.h>
#include "game.h"
#include "character.h"
#include "frame.h"
#include "resource.h"
#include "level.h"

const Vector2 initialPos = { 100.0f, 100.0f };

void Update(GameContext* context, Player* player) {
	float dt = GetFrameTime();
	UpdatePlayer(context, player, dt);
	context->state->camera.target = player->entity.position;
	if (IsKeyPressed(KEY_H)) {
		context->options->showHitbox = !context->options->showHitbox;
	}
}

int RunGame(GameContext* context) {
	// Game cursor configuration.
	// TODO: Check in options if system cursor is being used.
	HideCursor();
	LoadCustomCursor(context->options);

	// Load necessary textures.
	RenderTexture2D worldRender = LoadRenderTexture(worldSize.width, worldSize.height);
	SetTextureFilter(worldRender.texture, TEXTURE_FILTER_POINT);

	// Load player character (TODO: Only on necessary screens)
	Texture2D* characterTexture = GetTexture(PLAYER_TEXTURE);
	printf("Create player\n");
	Player player = CreatePlayer(characterTexture);
	printf("Created player\n");

	// Main game loop
	bool generated = false;
	while (!WindowShouldClose()) {
		if (!generated) {
			printf("Generating seed\n");
			context->state->seed = GenerateGameSeed();
			context->state->seedStr = SeedToString(context->state->seed);
			generated = true;
		}
		// First run the logic updates.
		Update(context, &player);

		// Run draw frame logic.
		Render(
			context,
			&worldRender,
			&player
		);
	}

	// Unload resources before exit.
	UnloadTextures();

	// Free used memory.
	if (context->state->seedStr != NULL) {
		free(context->state->seedStr);
	}
	CloseWindow();

	return GAME_CLOSE_SUCCESS;
}
