#include "entity.h"
#include "attack.h"
#include "event.h"
#include "game.h"
#include "lib.h"

static Observable entityEvents;

Enemy enemies[TOTAL_ENEMIES] = {
	// Basic approaching enemy.
	[MAINT_MELEE] = {
		.activeRadius = DEFAULT_ENEMY_RADIUS,
		.behaviour = APPROACH,
		.baseSpeed = ENEMY_DEFAULT_SPEED,
		.attackCd = 2.0f,
		.maxhp = 40,
		.attackId = 0,
	},
	// Basic shooting from distance enemy.
	[MAINT_SHOOTER] = {
		.activeRadius = DEFAULT_ENEMY_RADIUS,
		.behaviour = DISTANCE,
		.baseSpeed = ENEMY_DEFAULT_SPEED,
		.attackCd = 2.0f,
		.maxhp = 40,
		.attackId = 4
	},
	// Basic slow heavy hitter.
	[MAINT_FAT] = {
		.activeRadius = DEFAULT_ENEMY_RADIUS * 1.1f,
		.behaviour = APPROACH,
		.baseSpeed = ENEMY_DEFAULT_SPEED * 0.33f,
		.attackCd = 2.0f,
		.maxhp = 80,
		.attackId = 1
	},
	// Weak, fast, small enemy.
	[RAT] = {
		.activeRadius = DEFAULT_ENEMY_RADIUS,
		.behaviour = APPROACH,
		.baseSpeed = ENEMY_DEFAULT_SPEED * 1.5f,
		.attackCd = 1.25f,
		.maxhp = 9,
		.attackId = 5
	}
};

// Enemy groups that are placed in picked fight rooms.
// TODO: Pools weighted by difficulty and then divided by areas.
// Can just divide areas by starting and ending indices.
static const EnemyGroup enemyPool[TOTAL_ENEMY_GROUPS] = {
	// Three basic melee enemies in triangle formation
	{
		.count = 3,
		.enemies = {
			(EnemySpawn){ .enemyId = MAINT_MELEE, .pos = (Vector2){ 10, 3 } },
			(EnemySpawn){ .enemyId = MAINT_MELEE, .pos = (Vector2){ 6, 5 } },
			(EnemySpawn){ .enemyId = MAINT_MELEE, .pos = (Vector2){ 13, 5 } },
		},
		.difficulty = 1
	},
	// Two shooters, one melee
	{
		.count = 3,
		.enemies = {
			(EnemySpawn){ .enemyId = MAINT_MELEE, .pos = (Vector2){ 10, 3 } },
			(EnemySpawn){ .enemyId = MAINT_SHOOTER, .pos = (Vector2){ 6, 5 } },
			(EnemySpawn){ .enemyId = MAINT_SHOOTER, .pos = (Vector2){ 13, 5 } },
		},
		.difficulty = 2
	},
	// One shooter, two melee.
	{
		.count = 3,
		.enemies = {
			(EnemySpawn){ .enemyId = MAINT_SHOOTER, .pos = (Vector2){ 10, 3 } },
			(EnemySpawn){ .enemyId = MAINT_MELEE, .pos = (Vector2){ 6, 5 } },
			(EnemySpawn){ .enemyId = MAINT_MELEE, .pos = (Vector2){ 13, 5 } },
		},
		.difficulty = 1
	},
	// Two shooters, fat one
	{
		.count = 3,
		.enemies = {
			(EnemySpawn){ .enemyId = MAINT_FAT, .pos = (Vector2){ 10, 3 } },
			(EnemySpawn){ .enemyId = MAINT_SHOOTER, .pos = (Vector2){ 6, 5 } },
			(EnemySpawn){ .enemyId = MAINT_SHOOTER, .pos = (Vector2){ 13, 5 } },
		},
		.difficulty = 3
	},
	// Group of rats
	{
		.count = 6,
		.enemies = {
			(EnemySpawn){ .enemyId = RAT, .pos = (Vector2){ 3, 3 } },
			(EnemySpawn){ .enemyId = RAT, .pos = (Vector2){ 8, 3 } },
			(EnemySpawn){ .enemyId = RAT, .pos = (Vector2){ 13, 3 } },
			(EnemySpawn){ .enemyId = RAT, .pos = (Vector2){ 3, 6 } },
			(EnemySpawn){ .enemyId = RAT, .pos = (Vector2){ 8, 6 } },
			(EnemySpawn){ .enemyId = RAT, .pos = (Vector2){ 13, 6 } },
		},
		.difficulty = 1
	},
	// Two fat ones.
	{
		.count = 2,
		.enemies = {
			(EnemySpawn){ .enemyId = MAINT_FAT, .pos = (Vector2){ 6, 4 } },
			(EnemySpawn){ .enemyId = MAINT_FAT, .pos = (Vector2){ 8, 4 } },
		},
		.difficulty = 1
	},
};

Enemy* GetEnemy(int i) {
	if (i > TOTAL_ENEMIES - 1) {
		LogDebug("Attempting to get invalid enemy %d", i);
		return NULL;
	}
	return &enemies[i];
}

const EnemyGroup* GetEnemyGroup(int i) {
	if (i > TOTAL_ENEMY_GROUPS - 1) {
		LogDebug("Attempting to get invalid enemy group %d", i);
		return NULL;
	}
	return &enemyPool[i];
}

ActiveEnemy InstantiateEnemy(Enemy* enemy, Vector2 pos) {
	return (ActiveEnemy){
		.active = true,
		.activeRadius = enemy->activeRadius,
		.behaviour = enemy->behaviour,
		.speed = enemy->baseSpeed,
		.attack = GetAttack(enemy->attackId),
		.lastAttack = 0.0f,
		.attackCd = enemy->attackCd,
		.entity = CreateEntity(
			enemy->maxhp,
			pos,
			(Sprite){
				.rect = (Rectangle){ 0, 0, 32, 32 },
				.position = (Vector2){ -16, -16 },
				.visible = true,
				.layer = 4
			},
			(Rectangle){ -8, -8, 16, 16 },
			0.5f
		)
	};
}

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

void ApplyStatus(GameEntity* entity, Status status, float value) {
	// If status wasn't active prior, set tick timing to 0.
	if (!entity->statuses[status].active) {
		entity->statuses[status].tickElapsed = 0.0f;
	}
	entity->statuses[status].active = true;
	entity->statuses[status].value += value;
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

	// Apply statuses, if any.
	for (int i = POISON; i <= PARALYSED; i++) {
		if (attack->attack->statuses[i] > 0) {
			// If status wasn't active prior, set tick timing to 0.
			if (!entity->statuses[i].active) {
				entity->statuses[i].tickElapsed = 0.0f;
			}
			entity->statuses[i].active = true;
			entity->statuses[i].value += attack->attack->statuses[i];
		}
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

float MaxAttackRange(ActiveEnemy* enemy) {
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
			break;
		case BURN: break;
		case FROZEN: break;
		case PARALYSED: break;
		default: break;
	}
}

static void UpdateStatuses(GameEntity* entity, float delta) {
	for (int i = POISON; i <= PARALYSED; i++) {
		entity->statuses[i].tickElapsed += delta;
		if (entity->statuses[i].active && entity->statuses[i].tickElapsed >= statusTickrates[i]) {
			RunStatus(entity, i);
			entity->statuses[i].tickElapsed -= statusTickrates[i];
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
		ActiveAttack att = InitiateAttack(entity, targetPos, attack, at, false);
		void* result = AddToPool(attackPool, &att);
		if (result == NULL) {
			LogDebug("Failed to allocate attack on object pool");
			return -1;
		}
		LogDebug("Amount of active attacks: %d", attackPool->activeItems);

		// Attack was instantiated.
		return 2;
	}

	// Not currently attacking nor readying an attack.
	return 0;
}

int EnemyCheckAttack(ActiveEnemy* enemy, float playTime, Vector2* targetPos) {
	if (
		enemy->entity.stance != ATTACKING
		&& (enemy->lastAttack == 0.0f || enemy->lastAttack + enemy->attackCd < playTime)
	) {
		// Shooting attack.
		bool doAttack = enemy->attack->speed > 0.0f;

		// Check if player is within range of entity attack for melee attacks.
		if (!doAttack && enemy->attack->speed == 0.0f) {
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
