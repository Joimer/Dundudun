/*********************************
 * Main game logic and structure *
 *********************************/

#ifndef GAME_H
#define GAME_H

#include <raylib.h>
#include <raymath.h>
#include "lib.h"
#include "screens.h"

#define GAME_NAME "Dundudun"
#define GAME_CLOSE_SUCCESS 0
#define SEED_LENGTH 8
#define BITS_PER_SEED_CHAR 6
#define MIN_TICKS 54.0f
#define MAX_DELTA 1.0f / MIN_TICKS
#define TOTAL_STATUSES 4

typedef struct {
	float elapsed;
	float duration;
	bool active;
} TimedStatus;

typedef enum { STANDING, ATTACKING, WALKING, RUNNING, FLYING, DASHING, STAGGERING } Stance;

typedef enum { POISON, BURN, FROZEN, PARALYSED } StatusName;

typedef struct {
	StatusName id;
	float tickRate;
	float duration;
	float speedMod;
	float dmgMod;
	bool accumulative;
} Status;

extern const Status statuses[TOTAL_STATUSES];

typedef struct {
	float value;
	bool active;
	float tickElapsed;
	float totalElapsed;
} ActiveStatus;

typedef struct {
	Sprite sprite;
	int stanceAnimation[8][7];
	Vector2 position;
	int maxHealth;
	int health;
	TimedStatus invuln;
	Rectangle hitbox;
	// Direction the entity is facing.
	// Cues sprite position.
	Direction dir;
	// Movement vector out of an angle.
	Vector2 anglev;
	// Speed at which the entity will move in its angle.
	float speed;
	// Percentual modifier to final speed.
	float speedMod;
	// Percentual modifier to final damage.
	float dmgMod;
	// Whether the entity can be affected by external physical forces.
	bool unstoppable;
	// Entity state and the time it's been in that state.
	Stance stance;
	float stanceTime;
	// Stun status.
	TimedStatus stun;
	ActiveStatus statuses[TOTAL_STATUSES];
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
	int menuOption;
	int menuContext;
	bool shouldClose;
} GameState;

typedef enum { DIRECTIONAL, MOUSE } DashMode;

typedef enum { EN, ES } Language;

typedef struct {
	DashMode dashMode;
	int targetFps;
	bool showGizmos;
	Rectangle screenSize;
	bool systemCursor;
	Sprite cursor;
	bool fullMap;
	Language lang;
} GameOptions;

typedef struct GameContext {
	GameState* state;
	GameOptions* options;
} GameContext;

int RunGame(GameContext* context);
const char* SeedToString(unsigned long seed);
Vector2 GetWorldMousePos(GameContext* context);

// Should go in screens.h but right now redundancy with game.h
void LoadNextScreen(GameContext* context, GameScreen next);

#endif
