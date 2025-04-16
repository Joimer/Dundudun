#include <stdlib.h>
#include <raylib.h>
#include "game.h"
#include "character.h"
#include "frame.h"
#include "resource.h"
#include "level.h"
#include "lib.h"

const Vector2 initialPos = { 100.0f, 100.0f };

inline unsigned long GenerateGameSeed() {
	// Seeds are 48 bits long.
	// 6 bits per character, all numbers and caps ASCII alphabet
	// From 000000000000000000000000000000000000000000000000
	// To   111111111111111111111111111111111111111111111111
	// We assume 64 bits system and let's go.
	unsigned long seed = 0;
	for (int i = 0; i < SEED_LENGTH; i++) {
		unsigned long part = GetRandomValue(0, 35);
		seed |= part << BITS_PER_SEED_CHAR * i;
	}
	return seed;
}

inline static char SeedIntToChar(int val) {
	if (val > 35) {
		val = val % 36;
	}
	if (val < 10) {
		return (char)(48 + val);
	}
	return (char)(55 + val);
}

const char* SeedToString(unsigned long seed) {
	char* seedString = malloc(SEED_LENGTH + 1);
	for (int i = 0; i < SEED_LENGTH; i++) {
		// Each 6 bits on the number defines a character.
		// Create a mask where only the bits corresponding to the character position are gotten.
		unsigned long mask = (((1 << BITS_PER_SEED_CHAR) - 1) << (i * BITS_PER_SEED_CHAR));
		// We only need the first bits here, so can use int for this.
		int charBits = (int)(seed & mask);
		seedString[i] = SeedIntToChar(charBits);
	}
	// string termination char.
	seedString[SEED_LENGTH] = '\0';

	// Note: This is used once currently, if used more, you would need to free() the result.
	return seedString;
}

void SetupGamePRNG(GameContext* context) {
	context->state->seed = GenerateGameSeed();
	context->state->seedStr = SeedToString(context->state->seed);
	// We want to players with the same seed to experience the same maps and drops and bosses.
	// Therefor, we use the MT Rand only for these, always in the same step.
	context->state->mtrand = SeedMTRand(context->state->seed);
}

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
	if (!context->options->systemCursor) {
		HideCursor();
		LoadCustomCursor(context->options);
	}

	// Load necessary textures.
	RenderTexture2D worldRender = LoadRenderTexture(worldSize.width, worldSize.height);
	SetTextureFilter(worldRender.texture, TEXTURE_FILTER_POINT);

	// Load player character (TODO: Only on necessary screens)
	Texture2D* characterTexture = GetTexture(PLAYER_TEXTURE);
	Player player = CreatePlayer(characterTexture);

	// Generate initial seed (can be set by player later, too).
	SetupGamePRNG(context);

	// Main game loop
	while (!WindowShouldClose()) {
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
