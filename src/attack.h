/****************************************
 * Attacks both from player and enemies *
 ****************************************/

#ifndef ATTACK_H
#define ATTACK_H

#include <raylib.h>
#include "game.h"

#define TOTAL_ATTACKS 10

// Hitboxes are either rectangles or circles, for now.
typedef enum { HB_RECT = 1, HB_CIRCLE = 2 } HitboxType;

typedef union {
	Rectangle rect;
	float radius;
} Hitbox;

typedef enum { D_PHYSICAL, D_BURN, D_POISON, D_ELECTRIC, D_ICE } DamageType;

typedef struct {
	int damage;
	DamageType dmgType;
	float windup;
	float duration;
	// Distance from the center of the attacker and the center of the attack.
	float centerDist;
	HitboxType type;
	Hitbox hitbox;
	float speed;
	// Projectile attacks destroy upon hitting a target.
	bool projectile;
	int statuses[TOTAL_STATUSES];
} Attack;

// TODO To declare the attack pointer straight in the weapon list, we need attack list to be a global.
// (item.c)
// Another option would be data setup functions that create the structures from a preset.
// This could also make it easier to load data from config/mod files.
extern Attack attacks[TOTAL_ATTACKS];

typedef enum { T_PLAYER, T_ENEMY, T_ALL } AttackTarget;

typedef struct {
	GameEntity* source;
	Attack* attack;
	float elapsed;
	Vector2 center;
	Hitbox hitbox;
	AttackTarget target;
	float stunDuration;
	float pushForce;
	Vector2 angle;
	bool completed;
	bool fromPlayer;
} ActiveAttack;

ActiveAttack InitiateAttack(GameEntity* attacker, Vector2* target, Attack* attack, AttackTarget at, bool fromPlayer);
Attack* GetAttack(int i);

#endif
