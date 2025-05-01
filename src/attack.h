#ifndef ATTACK_H
#define ATTACK_H

#include <raylib.h>
#include "game.h"

#define TOTAL_ATTACKS 5

// Hitboxes are either rectangles or circles, for now.
typedef enum { HB_RECT = 1, HB_CIRCLE = 2 } HitboxType;

typedef union {
	Rectangle rect;
	float radius;
} Hitbox;

typedef struct {
	int damage;
	float windup;
	float duration;
	// Distance from the center of the attacker and the center of the attack.
	float centerDist;
	HitboxType type;
	Hitbox hitbox;
	float speed;
	// Projectile attacks destroy upon hitting a target.
	bool projectile;
} Attack;

// TODO To declare the attack pointer straight in the weapon list, we need attack list to be a global.
// Another option would be data setup functions that create the structures from a preset.
// This could also make it easier to load data from config/mod files.
extern Attack attacks[TOTAL_ATTACKS];

typedef enum { T_PLAYER, T_ENEMY, T_ALL } AttackTarget;

typedef struct {
	Attack* attack;
	float elapsed;
	Vector2 center;
	Hitbox hitbox;
	AttackTarget target;
	float stunDuration;
	float pushForce;
	Vector2 angle;
} ActiveAttack;

ActiveAttack InitiateAttack(GameEntity* attacker, Vector2* target, Attack* attack, AttackTarget at);
Attack* GetAttack(int i);

#endif
