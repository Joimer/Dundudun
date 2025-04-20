// Game logic.

#ifndef GAME_H
#define GAME_H

#include <raylib.h>
#include <raymath.h>
#include "lib.h"
#include "screens.h"

#define GAME_CLOSE_SUCCESS 0

extern const Vector2 initialPos;

typedef struct {
	float elapsed;
	float duration;
	bool active;
} Invulnerability;

typedef struct {
	Sprite sprite;
	Vector2 position;
	int maxHealth;
	int health;
	Invulnerability invuln;
	Rectangle hitbox;
	Direction dir;
} GameEntity;

typedef struct {
	GameScreen currentScreen;
	GameScreen nextScreen;
	Camera2D camera;
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

typedef struct {
	bool dashing;
	float elapsed;
	Vector2 direction;
	int max;
	int consecutive;
	// Total cooldown after a dash sequence
	float cooldown;
	// Time to wait until player can dash again.
	float cdLeft;
} Dash;

typedef struct {
	GameEntity entity;
	float speed;
	Dash dash;
} Player;

int RunGame(GameContext* context);
unsigned long GenerateGameSeed();
const char* SeedToString(unsigned long seed);
void UpdateInvuln(GameEntity* entity, float dt);

#endif
