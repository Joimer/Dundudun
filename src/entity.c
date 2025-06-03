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
		.touchDmg = 0,
		.weight = 100.0f,
	},
	// Basic shooting from distance enemy.
	[MAINT_SHOOTER] = {
		.activeRadius = DEFAULT_ENEMY_RADIUS,
		.behaviour = DISTANCE,
		.baseSpeed = ENEMY_DEFAULT_SPEED,
		.attackCd = 2.0f,
		.maxhp = 40,
		.attackId = 4,
		.touchDmg = 0,
		.weight = 100.0f,
	},
	// Basic slow heavy hitter.
	[MAINT_FAT] = {
		.activeRadius = DEFAULT_ENEMY_RADIUS * 1.1f,
		.behaviour = APPROACH,
		.baseSpeed = ENEMY_DEFAULT_SPEED * 0.33f,
		.attackCd = 2.0f,
		.maxhp = 80,
		.attackId = 1,
		.touchDmg = 0,
		.weight = 150.0f,
	},
	// Weak, fast, small enemy.
	[RAT] = {
		.activeRadius = DEFAULT_ENEMY_RADIUS,
		.behaviour = APPROACH,
		.baseSpeed = ENEMY_DEFAULT_SPEED * 1.5f,
		.attackCd = 1.25f,
		.maxhp = 9,
		.attackId = 5,
		.touchDmg = 0,
		.weight = 30.0f,
	},
	// Player bomb is treated as an enemy that does not hurt on hit.
	[PBOMB] = {
		.activeRadius = DEFAULT_ENEMY_RADIUS * 10,
		.behaviour = BOMB,
		.baseSpeed = 0,
		.attackCd = 2.0f,
		.maxhp = 1,
		.attackId = 6,
		.touchDmg = 0,
		.weight = 0.0f,
	},
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
		.enemy = enemy,
		.active = true,
		.attack = GetAttack(enemy->attackId),
		.lastAttack = 0.0f,
		.targetPos = (Vector2){ 0 },
		.pathPoints = 0,
		.lastTile = 0,
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
		.invuln = (Invulnerability){ .duration = invuln },
		.speedMod = 1.0f,
		.dmgMod = 1.0f,
		.unstoppable = false,
	};
	for (int i = POISON; i <= PARALYSED; i++) {
		entity.statuses[i] = (ActiveStatus){ .value = 0.0f, .active = false	};
	}

	return entity;
}

int DamageEntity(GameEntity* entity, int damage, float dmgMod) {
	if (entity->invuln.active) {
		return 0;
	}
	entity->invuln.active = true;
	damage = damage > entity->health ? entity->health : damage;
	if (dmgMod != 1.0f) {
		damage = damage + damage * dmgMod / 100.0f;
	}
	entity->health -= damage;

	return damage;
}

void ApplyStatus(GameEntity* entity, StatusName status, float value) {
	// If status wasn't active prior, set tick timing to 0.
	if (!entity->statuses[status].active) {
		entity->statuses[status].tickElapsed = 0.0f;
		entity->statuses[status].totalElapsed = 0.0f;
	}
	entity->statuses[status].active = true;
	if (statuses[status].accumulative) {
		entity->statuses[status].value += value;
	} else {
		entity->statuses[status].value = value;
	}
	if (statuses[status].speedMod != 0.0f) {
		entity->speedMod += statuses[status].speedMod;
	}
	if (statuses[status].dmgMod != 0.0f) {
		entity->dmgMod += statuses[status].dmgMod;
	}
}

static void RemoveStatus(GameEntity* entity, StatusName status) {
	if (entity->statuses[status].active) {
		entity->statuses[status].active = false;
		if (statuses[status].speedMod != 0.0f) {
			entity->speedMod -= statuses[status].speedMod;
		}
		if (statuses[status].dmgMod != 0.0f) {
			entity->dmgMod -= statuses[status].dmgMod;
		}
	}
}

void ApplyStun(GameEntity* entity, float duration) {
	if (!entity->stunned) {
		entity->stunned = true;
		entity->stunDuration = duration;
		entity->stunElapsed = 0.0f;
	}
}

int AttackHitEntity(GameEntity* entity, ActiveAttack* attack) {
	// Calculate attack damage from source and active modifiers.
	const int damage = DamageEntity(entity, attack->attack->damage, attack->source->dmgMod);

	// Apply knockback.
	if (!entity->unstoppable && attack->pushForce > 0.0f) {
		entity->speed = attack->pushForce;
		entity->anglev = attack->angle;
		// Interrupts attacking stances.
		SetStance(entity, WALKING);
	}

	// Apply damage received stun.
	if (!entity->unstoppable && attack->stunDuration > 0.0f) {
		ApplyStun(entity, attack->stunDuration);
	}

	// Destroy projectiles.
	// TODO: Detect collision for projectiles and destroy it too.
	if (attack->attack->projectile) {
		attack->completed = true;
	}

	// Apply statuses, if any.
	for (int i = POISON; i <= PARALYSED; i++) {
		if (attack->attack->statuses[i] > 0) {
			ApplyStatus(entity, i, attack->attack->statuses[i]);
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

static void RunStatus(GameEntity* entity, StatusName status) {
	if (!entity->statuses[status].active) {
		return;
	}
	switch (status) {
		case POISON:
			if (entity->statuses[status].value <= 0) {
				RemoveStatus(entity, status);
			} else {
				EmitDmgEvent(entity, entity->statuses[status].value, D_POISON);
				entity->statuses[status].value--;
			}
			break;
		case BURN:
			EmitDmgEvent(entity, entity->statuses[status].value, D_BURN);
			break;
		default: break;
	}
}

float GetEntitySpeed(GameEntity* entity) {
	if (entity->speedMod == 1.0f) {
		return entity->speed;
	}
	if (entity->speedMod <= -100.0f) {
		return 0.0f;
	}
	return entity->speed + entity->speed * entity->speedMod / 100.0f;
}

static void UpdateStatuses(GameEntity* entity, float delta) {
	for (int i = POISON; i <= PARALYSED; i++) {
		if (entity->statuses[i].active) {
			entity->statuses[i].tickElapsed += delta;
			entity->statuses[i].totalElapsed += delta;
			if (statuses[i].tickRate != 0.0f && entity->statuses[i].tickElapsed >= statuses[i].tickRate) {
				RunStatus(entity, i);
				entity->statuses[i].tickElapsed -= statuses[i].tickRate;
			}
			if (entity->statuses[i].totalElapsed >= statuses[i].duration) {
				RemoveStatus(entity, i);
			}
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

int CreateEntityAttack(
	GameEntity* entity,
	Attack* attack,
	Vector2* targetPos,
	ObjectPool* attackPool,
	AttackTarget at
) {
	ActiveAttack att = InitiateAttack(entity, targetPos, attack, at, at == T_ALL);
	if (att.source == NULL) {
		LogDebug("Failed creating active attack instance");
		return -1;
	}
	void* result = AddToPool(attackPool, &att);
	if (result == NULL) {
		LogDebug("Failed to allocate attack on object pool");
		return -1;
	}
	//LogDebug("Amount of active attacks: %d", attackPool->activeItems);

	// Attack was instantiated.
	return 2;
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
		return CreateEntityAttack(entity, attack, targetPos, attackPool, at);
	}

	// Not currently attacking nor readying an attack.
	return 0;
}

int EnemyCheckAttack(ActiveEnemy* enemy, float playTime, Vector2* targetPos) {
	if (
		enemy->entity.stance != ATTACKING
		&& (enemy->lastAttack == 0.0f || enemy->lastAttack + enemy->enemy->attackCd < playTime)
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
