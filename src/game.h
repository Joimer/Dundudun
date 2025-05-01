// Game logic.

#ifndef GAME_H
#define GAME_H

#include <raylib.h>
#include <raymath.h>
#include "lib.h"
#include "screens.h"

#define GAME_CLOSE_SUCCESS 0
#define SEED_LENGTH 8
#define BITS_PER_SEED_CHAR 6

extern const Vector2 initialPos;

typedef struct {
	float elapsed;
	float duration;
	bool active;
} Invulnerability;

typedef enum { STANDING, ATTACKING, WALKING, RUNNING, FLYING, DASHING } Stance;

typedef struct {
	Sprite sprite;
	Vector2 position;
	int maxHealth;
	int health;
	Invulnerability invuln;
	Rectangle hitbox;
	// Direction the entity is facing.
	// Cues sprite position.
	Direction dir;
	// Movement vector out of an angle.
	Vector2 anglev;
	// Speed at which the entity will move in its angle.
	float speed;
	// Entity state and the time it's been in that state.
	Stance stance;
	float stanceTime;
	// Stun status.
	bool stunned;
	float stunElapsed;
	float stunDuration;
} GameEntity;

typedef struct {
	GameScreen screen;
	float elapsed;
	Camera2D camera;
	// To store camera position for transitions etc.
	Vector2 lastCamPos;
	bool paused;
	unsigned long seed;
	// We keep this at hand so it is calculated only once.
	const char* seedStr;
	MTRand mtrand;
} GameState;

typedef enum { DIRECTIONAL, MOUSE } DashMode;

typedef struct {
	DashMode dashMode;
	int targetFps;
	bool showGizmos;
	Rectangle screenSize;
	bool systemCursor;
	Sprite cursor;
} GameOptions;

typedef struct {
	GameState* state;
	GameOptions* options;
} GameContext;

int RunGame(GameContext* context);
unsigned long GenerateGameSeed();
const char* SeedToString(unsigned long seed);
Vector2 GetWorldMousePos(GameContext* context);

// Should go in screens.h but right now redundancy with game.h
void LoadNextScreen(GameContext* context, GameScreen next);

#endif
