#ifndef LEVEL_H
#define LEVEL_H

#include <raylib.h>
#include "lib.h"
#include "game.h"
#include "object-pool.h"
#include "character.h"
#include "attack.h"

#define DEFAULT_ENEMY_RADIUS 200.0f
#define ENEMY_DEFAULT_SPEED 150.0f
#define COLL_RAYCAST_ACTIVE 64.0f
#define COLL_RAYCAST_DIST 16.0f

typedef enum { WALL = 0, GROUND = 1, GRASS = 2 } TileType;

typedef struct {
	TileType type;
	bool obstacle;
	int damage;
	float speed;
} Tile;

typedef enum { APPROACH, STAND, DISTANCE } EnemyBehaviour;

typedef struct {
	GameEntity entity;
	float activeRadius;
	EnemyBehaviour behaviour;
	bool active;
	// This is the base speed of the enemy.
	// Can be modified. Entity holds final speed.
	float speed;
	float lastAttack;
	float attackCd;
	Attack* attack;
} Enemy;

typedef struct {
	char* content;
	Vector2 start;
	Vector2 end;
	float startTime;
	float endTime;
	int fontSize;
	Color color;
} ActiveText;

typedef struct {
	int floor;
	int tilesPerRow;
	int tileCount;
	Tile* tiles;
	int entityCount;
	Enemy* entities;
	float playTime;
	ObjectPool attacks;
	ObjectPool texts;
} Level;

typedef struct {
	float dt;
	Player* player;
	Level* level;
	ObjectPool* textPool;
} AttackCbArgs;

Level GenerateLevel(GameContext* context, int floor);
void Update(GameContext* context, Player* player, Level* level);
float MaxAttackRange(Enemy* enemy);
Rectangle HitboxWorldPosition(GameEntity* entity);
void UpdateLevel(GameContext* context, Player* player, Level* level, float dt);

#endif
