#include "entity.h"
#include "event.h"

static Observable entityEvents;

void SetupEntityEvents() {
	entityEvents = CreateEventEmitter(0);
}

// Probably should have an item within context able to hold arbitrary amounts of observables and observers,,,
Observable* GetEntityEvents() {
	return &entityEvents;
}

void EmitDmgEvent(GameEntity* entity, int damage, DamageType type) {
	EmitEvent(&entityEvents, (Event){
		.type = E_DMG,
		.params = { .dmg = (DamageEvent){
			.entity = entity,
			.amount = damage,
			.type = type,
		}}
	});
}

GameEntity CreateEntity(
	int health, Vector2 pos, Sprite sprite, Rectangle hitbox, float invuln
) {
	GameEntity entity = {
		.health = health,
		.maxHealth = health,
		.position = pos,
		.dir = SOUTH,
		.sprite = sprite,
		.hitbox = hitbox,
		.invuln = (Invulnerability){ .duration = invuln }
	};
	for (int i = POISON; i <= PARALYSED; i++) {
		entity.statuses[i] = (ActiveStatus){ .value = 0.0f, .active = false	};
	}

	return entity;
}

int DamageEntity(GameEntity* entity, int damage) {
	entity->invuln.active = true;
	damage = damage > entity->health ? entity->health : damage;
	entity->health -= damage;

	return damage;
}

int AttackHitEntity(GameEntity* entity, ActiveAttack* attack) {
	int damage = DamageEntity(entity, attack->attack->damage);

	// Apply knockback.
	if (attack->pushForce > 0.0f) {
		entity->speed = attack->pushForce;
		float angle = Vector2LineAngle(attack->center, entity->position);
		entity->anglev = (Vector2){ .x = cosf(angle), .y = -(sinf(angle)) };
	}

	// Apply damage received stun.
	if (attack->stunDuration > 0.0f) {
		entity->stunned = true;
		entity->stunDuration = attack->stunDuration;
		entity->stunElapsed = 0.0f;
	}

	// Destroy projectiles.
	// TODO: Detect collision for projectiles and destroy it too.
	if (attack->attack->projectile) {
		attack->completed = true;
	}

	return damage;
}

static void UpdateInvuln(GameEntity* entity, float dt) {
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

static void RunStatus(GameEntity* entity, Status status) {
	if (!entity->statuses[status].active) {
		return;
	}
	int damage;
	switch (status) {
		case POISON:
			damage = entity->statuses[status].value;
			if (damage <= 0) {
				entity->statuses[status].active = false;
			} else {
				entity->statuses[status].value--;
				EmitDmgEvent(entity, damage, D_POISON);
			}
			/// level.c AttackHitEntity
			/// ahora mismo recibe el daño y crea el texto etc.
			/// cómo pasar aquí el texto
			/// y si meto un sistema global de eventos para consumir luego en la UI
			/// AddEvent o algo así y se mira la lista entera en el frame para ver qué acciones meter
			break;
		case BURN: break;
		case FROZEN: break;
		case PARALYSED: break;
		default: break;
	}
}

static void UpdateStatuses(GameEntity* entity, float delta) {
	for (int i = POISON; i <= PARALYSED; i++) {
		if (entity->statuses[i].active) {

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

	// Update statuses.
	UpdateStatuses(entity, delta);
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
