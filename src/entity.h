#ifndef ENTITY_H
#define ENTITY_H

#include <raylib.h>
#include "game.h"
#include "attack.h"
#include "object-pool.h"

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

void UpdateInvuln(GameEntity* entity, float dt);
void SetStance(GameEntity* entity, Stance stance);
Rectangle HitboxWorldPosition(GameEntity* entity);
float MaxAttackRange(Enemy* enemy);
void UpdateEntity(GameEntity* entity, float delta);
int EntityUnwindAttack(
	GameEntity* entity,
	Attack* attack,
	Vector2* targetPos,
	ObjectPool* attackPool,
	AttackTarget at
);
int EnemyCheckAttack(Enemy* enemy, float playTime, Vector2* targetPos);
void StandStill(GameEntity* entity);

#endif
