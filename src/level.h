#ifndef LEVEL_H
#define LEVEL_H

#include <raylib.h>
#include "lib.h"
#include "game.h"
#include "object-pool.h"
#include "character.h"
#include "attack.h"

#define DEFAULT_ENEMY_RADIUS 200.0f

typedef enum { WALL = 0, GROUND = 1, GRASS = 2 } TileType;

typedef struct {
	TileType type;
	bool obstacle;
	int damage;
} Tile;

typedef enum { APPROACH, STAND, DISTANCE } EnemyBehaviour;

typedef struct {
	GameEntity entity;
	float activeRadius;
	EnemyBehaviour behaviour;
	bool active;
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
	float levelTime;
	ObjectPool* textPool;
} AttackCbArgs;

Level GenerateLevel(GameContext* context, int floor);
void Update(GameContext* context, Player* player, Level* level);
float MaxAttackRange(Enemy* enemy);
Rectangle HitboxWorldPosition(GameEntity* entity);
void UpdateLevel(GameContext* context, Player* player, Level* level, float dt);

#endif
