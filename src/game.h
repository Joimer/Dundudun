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
	float start;
	float duration;
	bool active;
} Invulnerability;

typedef struct {
	Sprite sprite;
	Vector2 position;
	int health;
	Invulnerability invuln;
	Rectangle hitbox;
} GameEntity;

typedef struct {
	GameScreen currentScreen;
	GameScreen nextScreen;
	Camera2D camera;
	bool paused;
	int seed;
	// We keep this at hand so it is calculated only once.
	const char* seedStr;
} GameState;

typedef enum { DIRECTIONAL, MOUSE } DashMode;

typedef struct {
	DashMode dashMode;
	int targetFps;
	bool showHitbox;
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
} Dash;

typedef struct {
	GameEntity entity;
	float speed;
	Dash dash;
} Player;

void Update(GameContext* context, Player* player);
int RunGame(GameContext* context);

#endif
