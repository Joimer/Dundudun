/*****************************************
 * Live game entities (enemies, summons) *
 *****************************************/

#ifndef ENTITY_H
#define ENTITY_H

#include <raylib.h>
#include "game.h"
#include "attack.h"
#include "object-pool.h"
#include "event.h"

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

int DamageEntity(GameEntity* entity, int damage);
void ApplyStatus(GameEntity* entity, Status status, float value);
int AttackHitEntity(GameEntity* entity, ActiveAttack* attack);
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
GameEntity CreateEntity(int health, Vector2 pos, Sprite sprite, Rectangle hitbox, float invuln);
void SetupEntityEvents();
Observable* GetEntityEvents();
void EmitDmgEvent(GameEntity* entity, int damage, DamageType type);

#endif
