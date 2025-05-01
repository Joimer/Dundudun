#include "entity.h"

void UpdateInvuln(GameEntity* entity, float dt) {
	if (entity->invuln.active) {
		entity->invuln.elapsed += dt;
		if (entity->invuln.elapsed >= entity->invuln.duration) {
			entity->invuln.active = false;
			entity->invuln.elapsed = 0;
		}
	}
}

void SetStance(GameEntity* entity, Stance stance) {
	if (entity->stance != stance) {
		entity->stance = stance;
		entity->stanceTime = 0.0f;
	}
}

Rectangle HitboxWorldPosition(GameEntity* entity) {
	if (entity == NULL) {
		LogDebug("Invalid entity, returning empty rectangle.");
		return (Rectangle) {};
	}
	return (Rectangle) {
		entity->position.x + entity->hitbox.x,
		entity->position.y + entity->hitbox.y,
		entity->hitbox.width,
		entity->hitbox.height
	};
}

float MaxAttackRange(Enemy* enemy) {
	if (enemy == NULL || enemy->attack == NULL) {
		return 0.0f;
	}
	float baseDist = enemy->attack->centerDist;
	if (enemy->attack->type == HB_RECT) {
		baseDist += (enemy->attack->hitbox.rect.width + enemy->attack->hitbox.rect.height) / 4.0f;
	}
	if (enemy->attack->type == HB_CIRCLE) {
		baseDist += enemy->attack->hitbox.radius;
	}

	return baseDist + 1.0f;
}

static void UpdateStun(GameEntity* entity, float delta) {
	if (entity->stunned) {
		entity->stunElapsed += delta;
		if (entity->stunElapsed >= entity->stunDuration) {
			entity->stunned = false;
			// TODO: Probably better to manage these forces in a different way...
			entity->speed = 0.0f;
		}
	}
}

// Run update on all entity systems.
void UpdateEntity(GameEntity* entity, float delta) {
	entity->stanceTime += delta;

	// Check invulnerability status.
	UpdateInvuln(entity, delta);

	// Check stunned status.
	UpdateStun(entity, delta);
}

// Returns wether unwinding or unwinded (movement stops) or no related attack unwind action (pick other action).
int EntityUnwindAttack(
	GameEntity* entity,
	Attack* attack,
	Vector2* targetPos,
	ObjectPool* attackPool,
	AttackTarget at
) {
	if (entity->stance == ATTACKING) {
		if (entity->stanceTime < attack->windup) {
			// Winding up attack, nothing to do here yet.
			return 1;
		}

		// Attack windup has finished, instantiate actual attack hitbox.
		ActiveAttack att = InitiateAttack(entity, targetPos, attack, at);
		void* result = AddToPool(attackPool, &att);
		if (result == NULL) {
			LogDebug("Failed to allocate attack on object pool");
			return 0;
		}
		LogDebug("Amount of active attacks: %d", attackPool->activeItems);

		return 1;
	}

	return 0;
}

int EnemyCheckAttack(Enemy* enemy, float playTime, Vector2* targetPos) {
	if (
		enemy->entity.stance != ATTACKING
		&& (enemy->lastAttack == 0.0f || enemy->lastAttack + enemy->attackCd < playTime)
	) {
		// Shooting attack.
		bool doAttack = enemy->attack->speed > 0.0f;

		// Check if player is within range of entity attack.
		if (!doAttack) {
			float maxRange = MaxAttackRange(enemy);
			float dist = Vector2Distance(enemy->entity.position, *targetPos);
			doAttack = dist <= maxRange;
		}

		// In range for attack and no cooldown.
		if (doAttack) {
			// Initiate attack and finish.
			SetStance(&enemy->entity, ATTACKING);
			// TODO: everything else works with dt, these work with playTime instead...
			enemy->lastAttack = playTime;

			return 1;
		}
	}

	return 0;
}

void StandStill(GameEntity* entity) {
	SetStance(entity, STANDING);
	entity->speed = 0.0f;
}
