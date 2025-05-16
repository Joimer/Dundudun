#include <raylib.h>
#include <raymath.h>
#include "attack.h"
#include "game.h"
#include "lib.h"

Attack attacks[TOTAL_ATTACKS] = {
	// Enemy melee hit.
	{
		.damage = 5,
		.windup = 0.25f,
		.duration = 0.33f,
		.centerDist = 20.0f,
		.type = HB_RECT,
		.hitbox = { .rect = { 0, 0, 32.0f, 32.0f } },
		.speed = 0,
		.projectile = false,
		.dmgType = D_PHYSICAL
	},
	// Enemy circle explosion like attack
	{
		.damage = 10,
		.windup = 0.3f,
		.duration = 0.5f,
		.centerDist = 32.0f,
		.type = HB_CIRCLE,
		.hitbox = { .radius = 24.0f },
		.speed = 0,
		.projectile = false,
		.dmgType = D_PHYSICAL
	},
	// Player melee hit.
	{
		.damage = 7,
		.windup = 0.0f,
		.duration = 0.15f,
		.centerDist = 26.0f,
		.type = HB_RECT,
		.hitbox = { .rect = { 0, 0, 40.0f, 40.0f } },
		.speed = 0,
		.projectile = false,
		.dmgType = D_PHYSICAL
	},
	// Player shooting.
	{
		.damage = 5,
		.windup = 0.0f,
		.duration = 20.0f,
		.centerDist = 0.0f,
		.type = HB_CIRCLE,
		.hitbox = { .radius = 3.0f },
		.speed = 300.0f,
		.projectile = true,
		.dmgType = D_PHYSICAL,
		//.statuses = { 4, 0, 0, 0 }
	},
	// Enemy shooting.
	{
		.damage = 4,
		.windup = 0.0f,
		.duration = 20.0f,
		.centerDist = 0.0f,
		.type = HB_CIRCLE,
		.hitbox = { .radius = 3.0f },
		.speed = 300.0f,
		.projectile = true,
		.dmgType = D_PHYSICAL
	},
	// Enemy weak melee hit.
	{
		.damage = 2,
		.windup = 0.15f,
		.duration = 0.33f,
		.centerDist = 20.0f,
		.type = HB_RECT,
		.hitbox = { .rect = { 0, 0, 32.0f, 32.0f } },
		.speed = 0,
		.projectile = false,
		.dmgType = D_PHYSICAL
	}
};

ActiveAttack InitiateAttack(
	GameEntity* attacker, Vector2* target, Attack* attack, AttackTarget at, bool fromPlayer
) {
	if (attacker == NULL || target == NULL || attack == NULL) {
		LogDebug("Invalid call to InitiateAttack with a null pointer!");
		return (ActiveAttack){ .source = NULL };
	}
	float angle = Vector2LineAngle(attacker->position, *target);
	float stunDuration = at == T_ENEMY ? 0.2f : 0.1f;
	float pushForce = at == T_ENEMY ? 150.0f : 50.0f;
	Vector2 anglev = { cosf(angle), -(sinf(angle)) };
	ActiveAttack att = {
		.source = attacker,
		.attack = attack,
		.elapsed = 0.0f,
		.target = at,
		.pushForce = pushForce,
		.stunDuration = stunDuration,
		.angle = anglev,
		.completed = false,
		.center = attack->centerDist == 0.0f ? attacker->position : Vector2Add(
			attacker->position,
			(Vector2){
				.x = anglev.x * attack->centerDist,
				.y = anglev.y * attack->centerDist
			}
		),
		.fromPlayer = fromPlayer
	};

	// Rectangle hitbox attack.
	if (attack->type == HB_RECT) {
		Vector2 attackPos = Vector2Subtract(
			att.center,
			(Vector2){
				.x = attack->hitbox.rect.width / 2.0f,
				.y = attack->hitbox.rect.height / 2.0f
			}
		);
		att.hitbox.rect = (Rectangle){
			.x = attackPos.x,
			.y = attackPos.y,
			.width = attack->hitbox.rect.width,
			.height = attack->hitbox.rect.height
		};
	}

	// Circle hitbox attack.
	if (attack->type == HB_CIRCLE) {
		att.hitbox.radius = attack->hitbox.radius;
	}

	return att;
}

Attack* GetAttack(int i) {
	if (i > TOTAL_ATTACKS - 1) {
		LogDebug("Attempting to get invalid attack %d", i);
		return NULL;
	}
	return &attacks[i];
}
