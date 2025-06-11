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

#define TOTAL_ENEMIES 5
#define TOTAL_ENEMY_GROUPS 25
#define DEFAULT_ENEMY_RADIUS 200.0f
#define ENEMY_DEFAULT_SPEED 150.0f
#define MAX_PATH_POINTS 25

// Will I use this? Could be better idk haha
typedef enum { PLAYER_ENT, ENEMY_ENT, BOSS_ENT, ITEM_ENT } EntityType;

typedef enum { APPROACH, STAND, DISTANCE, BOMB } EnemyBehaviour;

typedef enum { MAINT_MELEE, MAINT_SHOOTER, MAINT_FAT, RAT, PBOMB } EnemyType;

// Enemy template to instantiate an live entity on a level.
typedef struct {
	EnemyType id;
	float activeRadius;
	EnemyBehaviour behaviour;
	// This is the base speed of the enemy.
	// Can be modified. Entity holds final speed.
	float baseSpeed;
	float attackCd;
	int attackId;
	int maxhp;
	int touchDmg;
	float weight;
} Enemy;

typedef struct {
	const Enemy* enemy;
	GameEntity entity;
	bool active;
	float lastAttack;
	Attack* attack;
	// Movement target and pathing.
	int lastTile;
	Vector2 targetPos;
	unsigned int pathPoints;
	Vector2 path[MAX_PATH_POINTS];
} ActiveEnemy;

typedef struct {
	EnemyType enemyId;
	// Spawn position is x,y relative to default room size, 15x8
	Vector2 pos;
} EnemySpawn;

typedef struct {
	int count;
	EnemySpawn enemies[10];
	int difficulty;
} EnemyGroup;

Enemy* GetEnemy(int i);
ActiveEnemy InstantiateEnemy(Enemy* enemy, Vector2 pos);
const EnemyGroup* GetEnemyGroup(int i);
int DamageEntity(GameEntity* entity, int damage, float dmgMod);
void ApplyStatus(GameEntity* entity, StatusName status, float value);
void ApplyStun(GameEntity* entity, float duration);
int AttackHitEntity(GameEntity* entity, ActiveAttack* attack);
void SetStance(GameEntity* entity, Stance stance);
Rectangle HitboxWorldPosition(GameEntity* entity);
float MaxAttackRange(ActiveEnemy* enemy);
void UpdateEntity(GameEntity* entity, float delta);
int CreateEntityAttack(
	GameEntity* entity,
	Attack* attack,
	Vector2* targetPos,
	ObjectPool* attackPool,
	AttackTarget at
);
int EntityUnwindAttack(
	GameEntity* entity,
	Attack* attack,
	Vector2* targetPos,
	ObjectPool* attackPool,
	AttackTarget at
);
int EnemyCheckAttack(ActiveEnemy* enemy, float playTime, Vector2* targetPos);
void StandStill(GameEntity* entity);
GameEntity CreateEntity(int health, Vector2 pos, Sprite sprite, Rectangle hitbox, float invuln);
void SetupEntityEvents();
Observable* GetEntityEvents();
void EmitDmgEvent(GameEntity* entity, int damage, DamageType type);
float GetEntitySpeed(GameEntity* entity);

#endif
