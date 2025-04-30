#include <raylib.h>
#include <raymath.h>
#include "attack.h"
#include "game.h"

Attack attacks[TOTAL_ATTACKS] = {
	// Enemy melee hit.
	{
		.damage = 5,
		.windup = 0.25f,
		.duration = 0.33f,
		.centerDist = 16.0f,
		.type = HB_RECT,
		.hitbox = { .rect = { 0, 0, 32.0f, 32.0f } }
	},
	// Enemy circle explosion like attack?
	{
		.damage = 6,
		.windup = 0.3f,
		.duration = 0.5f,
		.centerDist = 32.0f,
		.type = HB_CIRCLE,
		.hitbox = { .radius = 24.0f }
	},
	// Player melee hit.
	{
		.damage = 7,
		.duration = 0.15f,
		.centerDist = 0.0f,
		.type = HB_RECT,
		.hitbox = { .rect = { 0, 0, 40.0f, 40.0f } },
	},
	// Player shooting.
	{
		.damage = 5,
		.duration = 0.05f,
		.centerDist = 0.0f,
		.type = HB_CIRCLE,
		.hitbox = { .radius = 2.5f }
	},
};

ActiveAttack InitiateAttack(GameEntity* attacker, Vector2* target, Attack* attack, AttackTarget at) {
	if (attacker == NULL || target == NULL || attack == NULL) {
		LogDebug("Invalid call to InitiateAttack with a null pointer!");
		return (ActiveAttack){};
	}
	float angle = Vector2LineAngle(attacker->position, *target);
	float stunDuration = at == T_ENEMY ? 0.2f : 0.1f;
	float pushForce = at == T_ENEMY ? 150.0f : 50.0f;
	ActiveAttack att = {
		.attack = attack,
		.elapsed = 0.0f,
		.target = at,
		.pushForce = pushForce,
		.stunDuration = stunDuration,
	};

	// Rectangle hitbox attack.
	if (attack->type == HB_RECT) {
		float halfWidth = attack->hitbox.rect.width / 2.0f;
		float halfHeight = attack->hitbox.rect.height / 2.0f;
		Vector2 attackPos = Vector2Add(
			attacker->position,
			(Vector2){
				.x = cosf(angle) * attack->centerDist - halfWidth,
				.y = -(sinf(angle) * attack->centerDist + halfHeight)
			}
		);
		att.hitbox.rect = (Rectangle){
			.x = attackPos.x,
			.y = attackPos.y,
			.width = attack->hitbox.rect.width,
			.height = attack->hitbox.rect.height
		};
		att.center = (Vector2){ attackPos.x + halfWidth, attackPos.y - halfHeight };
	}

	// Circle hitbox attack.
	if (attack->type == HB_CIRCLE) {
		att.center = Vector2Add(
			attacker->position,
			(Vector2){
				.x = cosf(angle) * attack->centerDist,
				.y = -(sinf(angle) * attack->centerDist)
			}
		);
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
