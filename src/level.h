#ifndef LEVEL_H
#define LEVEL_H

#define SEED_LENGTH 8
#define BITS_PER_SEED_CHAR 6

#include "lib.h"
#include "game.h"

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
} Enemy;

typedef struct {
	int floor;
	int tileCount;
	Tile* tiles;
	int entityCount;
	Enemy* entities;
} Level;

Level GenerateLevel(GameContext* context, int floor);
void Update(GameContext* context, Player* player, Level* level);

#endif
