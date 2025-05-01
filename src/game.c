#include <stdlib.h>
#include <raylib.h>
#include "game.h"
#include "control.h"
#include "screens.h"
#include "lib.h"
#include "character.h"
#include "frame.h"
#include "resource.h"
#include "level.h"

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

static void UpdateLogo(GameContext* context) {
	if (
		context->state->elapsed > LOGO_DURATION
		|| (
			context->state->elapsed > 0.1f && (
			IsActionActive(ACCEPT)
			|| IsActionActive(CANCEL)
			|| IsActionActive(ACTION_A)
			|| IsActionActive(ACTION_B)
			|| IsActionActive(ACTION_C)
			|| IsActionActive(ACTION_D)
		))
	) {
		LoadNextScreen(context, TITLE);
	}
}

static void UpdateTitle(GameContext* context) {
	if (
		IsActionActive(ACCEPT)
		|| IsActionActive(CANCEL)
		|| IsActionActive(ACTION_A)
		|| IsActionActive(ACTION_B)
		|| IsActionActive(ACTION_C)
		|| IsActionActive(ACTION_D)
	) {
		LoadNextScreen(context, GAMEPLAY);
	}
}

static void Update(GameContext* context) {
	float dt = GetFrameTime();
	context->state->elapsed += dt;

	if (IsKeyPressed(KEY_H)) {
		context->options->showGizmos = !context->options->showGizmos;
	}
	if (IsKeyPressed(KEY_KP_SUBTRACT)) {
		context->state->camera.zoom -= 0.05f;
	}
	if (IsKeyPressed(KEY_KP_ADD)) {
		context->state->camera.zoom += 0.05f;
	}
	// TODO? Pass dt to all and function pointer?
	switch (context->state->screen) {
		case LOGO: return UpdateLogo(context);
		case GAMEPLAY: return UpdateLevel(context, dt);
		case TITLE: return UpdateTitle(context);
		default: return;
	}
}

int RunGame(GameContext* context) {
	// Game cursor configuration.
	if (!context->options->systemCursor) {
		HideCursor();
		LoadCustomCursor(context->options);
	}

	// Load world texture where the game native resolution will be loaded.
	RenderTexture2D worldRender = LoadRenderTexture(WORLD_SIZE_WIDTH, WORLD_SIZE_HEIGHT);

	// Generate initial seed (can be set by player later, too).
	SetupGamePRNG(context);

	// Main game loop
	while (!WindowShouldClose()) {
		// First run the logic updates.
		Update(context);

		// Run draw frame logic.
		Render(context, &worldRender);
	}

	// Unload resources and memory before exit.
	DestroyLevel();
	UnloadTextures();
	if (context->state->seedStr != NULL) {
		free((char*)context->state->seedStr);
	}
	CloseWindow();

	return GAME_CLOSE_SUCCESS;
}

void UpdateInvuln(GameEntity* entity, float dt) {
	if (entity->invuln.active) {
		entity->invuln.elapsed += dt;
		if (entity->invuln.elapsed >= entity->invuln.duration) {
			entity->invuln.active = false;
			entity->invuln.elapsed = 0;
		}
	}
}

void SetStance(GameEntity* entity, Stance stance) {
	if (entity->stance != stance) {
		entity->stance = stance;
		entity->stanceTime = 0.0f;
	}
}

Vector2 GetWorldMousePos(GameContext* context) {
	Vector2 mousePos = GetMousePosition();
	float worldHeight = (float) WORLD_SIZE_HEIGHT;
	float worldWidth = (float) WORLD_SIZE_WIDTH;
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
