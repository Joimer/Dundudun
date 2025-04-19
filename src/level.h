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

typedef struct {
	int damage;
	float duration;
	// Distance from the center of the attacker and the center of the attack.
	float centerDist;
	int type;
	union AttackType {
		struct { float width; float height; } hitbox;
		float radius;
	} data;
} Attack;

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
	Attack* attack;
	float start;
	Vector2 center;
	Rectangle hitbox;
} ActiveAttack;

typedef struct {
	int floor;
	int tileCount;
	Tile* tiles;
	int entityCount;
	Enemy* entities;
	int attackIndexStart;
	int attackIndexEnd;
	float playTime;
	ActiveAttack* attacks;
} Level;

Level GenerateLevel(GameContext* context, int floor);
void Update(GameContext* context, Player* player, Level* level);
float MaxAttackRange(Enemy* enemy);
Rectangle HitboxWorldPosition(GameEntity* entity);
void UpdateLevel(GameContext* context, Player* player, Level* level, float dt);

#endif
