#include <stdlib.h>
#include <raylib.h>
#include <string.h>
#include "control.h"
#include "level.h"
#include "lib.h"
#include "game.h"
#include "character.h"
#include "attack.h"

#define ENEMY_DEFAULT_SPEED 150.0f

Rectangle HitboxWorldPosition(GameEntity* entity) {
	return (Rectangle) {
		entity->position.x + entity->hitbox.x,
		entity->position.y + entity->hitbox.y,
		entity->hitbox.width,
		entity->hitbox.height
	};
}

Level GenerateLevel(GameContext* context, int floor) {
	floor = (int) Clamp(floor, 1, MAX_LEVEL);
	int entityCount = 3;
	int tileCount = 5;
	Level level = {
		.floor = floor,
		.tileCount = tileCount,
		.entityCount = entityCount
	};
	/*
	for (int i = 0; i < tileCount; i++) {
		level.tiles[i] = (Tile){};
	}
	*/
	// TODO: When doing a new level, free past level and realloc here.
	level.tiles = malloc(sizeof(Tile) * tileCount);
	level.entities = malloc(sizeof(Enemy) * entityCount);
	level.tiles[0] = (Tile){ .type = WALL, .obstacle = true, .damage = 0 };
	level.tiles[1] = (Tile){ .type = GROUND, .obstacle = false, .damage = 33 };
	level.tiles[2] = (Tile){ .type = GRASS, .obstacle = false, .damage = 0 };
	level.tiles[3] = (Tile){ .type = GROUND, .obstacle = false, .damage = 0 };
	level.tiles[4] = (Tile){ .type = WALL, .obstacle = true, .damage = 10 };
	for (int i = 0; i < entityCount; i++) {
		int pos = 128 * i + 256;
		level.entities[i] = (Enemy){
			.active = true,
			.activeRadius = DEFAULT_ENEMY_RADIUS,
			.behaviour = APPROACH,
			.speed = ENEMY_DEFAULT_SPEED,
			.attack = GetAttack(0),
			.lastAttack = 0.0f,
			.attackCd = 2.0f,
			.entity = (GameEntity){
				.sprite = (Sprite){
					.rect = (Rectangle){ 0, 0, 32, 32 },
					.position = (Vector2){ -16, -16 },
					.visible = true,
					.layer = 4
				},
				.position = (Vector2){ pos, pos },
				.health = 40,
				.maxHealth = 40,
				.invuln = (Invulnerability){ .duration = 0.5f },
				.hitbox = { -8, -8, 16, 16 },
				.dir = SOUTH
			}
		};
	}
	// Number of attacks to allocate should be calculated by total enemies and their attack cadence.
	level.attacks = CreatePoolOf(ActiveAttack, 32);
	level.texts = CreatePoolOf(ActiveText, 32);

	return level;
}

float MaxAttackRange(Enemy* enemy) {
	if (enemy == NULL || enemy->attack == NULL) {
		return 0.0f;
	}
	float baseDist = enemy->attack->centerDist;
	if (enemy->attack->type == HB_RECT) {
		baseDist += enemy->attack->hitbox.rect.height / 2.0f;
	}
	if (enemy->attack->type == HB_CIRCLE) {
		baseDist += enemy->attack->hitbox.radius;
	}

	return baseDist + 1.0f;
}

static bool DoesCollideAny(Level* level, GameEntity* self, Rectangle* newPos) {
	for (int j = 0; j < level->entityCount; j++) {
		if (!level->entities[j].active) {
			// Ignore inactive entities.
			continue;
		}
		if (self == &level->entities[j].entity) {
			// Ignore self.
			continue;
		}

		// Check collision with entity.
		Rectangle entityWorldHitbox = HitboxWorldPosition(&level->entities[j].entity);
		if (CheckCollisionRecs(entityWorldHitbox, *newPos)) {
			return true;
		}
	}

	return false;
}

// TODO: This seems like it could use being broken down to a handful of functions
static void UpdateEnemy(GameContext* context, Player* player, Level* level, Enemy* enemy, float dt) {
	// Check for death.
	if (enemy->entity.health <= 0) {
		enemy->active = false;
		// TODO: Death animation :)
		return;
	}

	// Check invulnerability status.
	UpdateInvuln(&enemy->entity, dt);
	enemy->entity.stanceTime += dt;

	// Check stunned status.
	if (enemy->entity.stunned) {
		enemy->entity.stunElapsed += dt;
		if (enemy->entity.stunElapsed >= enemy->entity.stunDuration) {
			enemy->entity.stunned = false;
			// TODO: Probably better to manage these forces in a different way...
			enemy->entity.speed = 0.0f;
		}
	}

	// Enemy is winding up an attack.
	if (!enemy->entity.stunned && enemy->entity.stance == ATTACKING) {
		if (enemy->entity.stanceTime < enemy->attack->windup) {
			// Winding up attack, nothing to do here.
			return;
		}

		// Attack windup has finished, instantiate actual attack hitbox.
		ActiveAttack att = InitiateAttack(&enemy->entity, &player->entity.position, enemy->attack, T_PLAYER);
		void* result = AddToPool(&level->attacks, &att);
		if (result == NULL) {
			LogDebug("Failed to allocate enemy attack on object pool");
		}
		LogDebug("Amount of active items: %d", level->attacks.activeItems);
		goto stand;
	}

	// TODO: Own functions for entities for movement/action and state machine for those.
	// Check if player is within the entity's active area.
	if (!enemy->entity.stunned && (player == NULL || !CheckCollisionPointCircle(
		player->entity.position, enemy->entity.position, enemy->activeRadius
	))) {
		// Inactive status.
		// If can be seen in screen or close by, idle behaviour.
		// Otherwise, completely ignore.
		// TODO: Idle.
		goto stand;
	}

	// Check for attacking behaviour.
	if (!enemy->entity.stunned) {
		// Check if entity status allows for attack.
		if (
			enemy->entity.stance != ATTACKING
			&& (enemy->lastAttack == 0.0f || enemy->lastAttack + enemy->attackCd < level->playTime)
		) {
			// Check if player is within range of entity attack.
			float maxRange = MaxAttackRange(enemy);
			float dist = Vector2Distance(enemy->entity.position, player->entity.position);

			// In range for attack and no cooldown.
			if (dist <= maxRange) {
				// Initiate attack and finish.
				SetStance(&enemy->entity, ATTACKING);
				enemy->lastAttack = level->playTime;
				return;
			}
		}

		if (enemy->behaviour == APPROACH) {
			// Set direction towards player.
			// Min distance is entity hitbox in front of player hitbox.
			float xDiff = fabs(player->entity.position.x - enemy->entity.position.x);
			float yDiff = fabs(player->entity.position.y - enemy->entity.position.y);
			float xThreshold = player->entity.hitbox.width / 2.0f + enemy->entity.hitbox.width / 2.0f;
			float yThreshold = player->entity.hitbox.height / 2.0f + enemy->entity.hitbox.height / 2.0f;
			//float vecDist = Vector2Distance(player->entity.position, enemy->entity.position);

			// Entity is close enough to player, ignore movement.
			if (xDiff <= xThreshold && yDiff <= yThreshold) {
				goto stand;
			}

			// Get the closest player hitbox corner to the enemy position.
			Vector2 closestCorner = ClosestRectCorner(
				(Rectangle){
					.x = player->entity.position.x + player->entity.hitbox.x,
					.y = player->entity.position.y + player->entity.hitbox.y,
					.width = player->entity.hitbox.width,
					.height = player->entity.hitbox.height
				},
				enemy->entity.position
			);
			Direction dir = GetPointDirThreshold(
				enemy->entity.position,
				closestCorner,
				enemy->entity.hitbox.width,
				enemy->entity.hitbox.height
			);

			// Hitbox is close enough to player, ignore movement.
			if (dir == NO_DIRECTION) {
				goto stand;
			}

			// Have to check if there's another hitbox in the desired direction.
			float maxSpeed = enemy->speed * dt;

			// Entity hitbox rectangle on its own with the future movement thresholds.
			float neighbourXThres = IsBitSet(dir, 1) ? maxSpeed : (IsBitSet(dir, 2) ? -maxSpeed : 0);
			float neighbourYThres = IsBitSet(dir, 4) ? -maxSpeed : (IsBitSet(dir, 3) ? maxSpeed : 0);

			// Pre-calculate all potential hitboxes so no need to recalcualte for every entity.
			Rectangle hitBoxArea = HitboxWorldPosition(&enemy->entity);
			Rectangle movedRectX = hitBoxArea;
			movedRectX.x += neighbourXThres;
			Rectangle movedRectY = hitBoxArea;
			movedRectY.y += neighbourYThres;
			Rectangle movedRectBoth = hitBoxArea;
			movedRectBoth.x += neighbourXThres;
			movedRectBoth.y += neighbourYThres;
			Rectangle entityWorldHitbox;

			for (int j = 0; j < level->entityCount; j++) {
				if (dir == NO_DIRECTION) {
					// We found out that the entity cannot move, no need for more calculations.
					break;
				}
				if (enemy == &level->entities[j]) {
					// Ignore self.
					continue;
				}
				if (!level->entities[j].active) {
					// Ignore inactive entities.
					continue;
				}

				entityWorldHitbox = HitboxWorldPosition(&level->entities[j].entity);
				// Check future hitbox for intended movements for each axis.
				// We'll block unavailable movements per axis if necessary.
				if (IsBitSet(dir, 2) && CheckCollisionRecs(movedRectX, entityWorldHitbox)) {
					dir &= ~(1 << 1);
				}
				if (IsBitSet(dir, 1) && CheckCollisionRecs(movedRectX, entityWorldHitbox)) {
					dir &= ~1;
				}
				if (IsBitSet(dir, 4)) {
					if (CheckCollisionRecs((IsBitSet(dir, 2) || IsBitSet(dir, 1) ? movedRectBoth : movedRectY), entityWorldHitbox)) {
						dir &= ~(1 << 3);
						continue;
					}
				}
				if (IsBitSet(dir, 3)) {
					if (CheckCollisionRecs((IsBitSet(dir, 2) || IsBitSet(dir, 1) ? movedRectBoth : movedRectY), entityWorldHitbox)) {
						dir &= ~(1 << 2);
						continue;
					}
				}
			}

			// If the entity was completely stopped, we can check on next one already.
			if (dir == NO_DIRECTION) {
				//LogDebug("Enemy cannot move, obstacle");
				// TODO: Find angle in respect to blocking entity and find a way around it to get to the player.
				goto stand;
			}
			SetStance(&enemy->entity, WALKING);
			enemy->entity.dir = dir;
			enemy->entity.speed = enemy->speed;
			enemy->entity.anglev = DirectionToVector(enemy->entity.dir);
		}
	}

	// Update entity position according to its movement.
	if (enemy->entity.speed > 0.0f) {
		// TODO: When colliding with pushback, full stop is not the most adequate...
		Vector2 newPos = Vector2Add(enemy->entity.position, (Vector2){
			enemy->entity.anglev.x * enemy->entity.speed * dt,
			enemy->entity.anglev.y * enemy->entity.speed * dt,
		});
		if (!DoesCollideAny(level, &enemy->entity, &newPos)) {
			enemy->entity.position = newPos;
		}
	}
	return;

	stand: SetStance(&enemy->entity, STANDING);
	enemy->entity.speed = 0.0f;
}

static void AttackHitEntity(AttackCbArgs* cbArgs, GameEntity* entity, ActiveAttack* attack) {
	Rectangle hitbox = HitboxWorldPosition(entity);
	bool doesHit = false;
	if (attack->attack->type == 1) {
		doesHit = CheckCollisionRecs(attack->hitbox, hitbox);
	}
	if (attack->attack->type == 2) {
		/*Circle attackHitbox = {
			.center = attack->center,
			.radius = attack->attack->data.radius
		};*/
		// TODO
	}
	if (!doesHit) {
		return;
	}
	// Attack is hitting entity.
	// TODO: Instantiate blood splash on ground.
	entity->invuln.active = true;
	int damage = attack->attack->damage > entity->health ? entity->health : attack->attack->damage;
	entity->health -= damage;
	Vector2 spritePos = Vector2Subtract(entity->position, entity->sprite.position);
	float startX = spritePos.x + entity->sprite.rect.width / 2.0f;
	ActiveText txt = {
		.content = IntToString(damage),
		.start = (Vector2){ startX, spritePos.y },
		.end = (Vector2){ startX, spritePos.y - 32.0f },
		.startTime = cbArgs->level->playTime,
		.endTime = cbArgs->level->playTime + 1.0f,
		.fontSize = 15,
		.color = attack->target == T_ENEMY ? ORANGE : RED
	};
	void* result = AddToPool(cbArgs->textPool, &txt);
	if (result == NULL) {
		LogDebug("Failed to allocate text to pool");
	}
	if (attack->pushForce > 0.0f) {
		entity->speed = attack->pushForce;
		float angle = Vector2LineAngle(attack->center, entity->position);
		entity->anglev = (Vector2){ .x = cosf(angle), .y = -(sinf(angle)) };
	}
	if (attack->stunDuration > 0.0f) {
		entity->stunned = true;
		entity->stunDuration = attack->stunDuration;
		entity->stunElapsed = 0.0f;
	}
}

void AttackCallback(ObjectPool* pool, int index, void* args) {
	// Ignore CB with invalid args, but does not mean item itself is invalid.
	if (args == NULL) {
		return;
	}
	ActiveAttack* attack = PoolIndexAddress(pool, index);
	if (attack == NULL || attack->attack == NULL) {
		LogDebug("Null pointer, invalid attack state");
		// This means some pointer is pointing at invalid data.
		goto cleanup;
	}
	// This attack has finished.
	if (attack->elapsed >= attack->attack->duration) {
		goto cleanup;
	}

	AttackCbArgs* cbArgs = (AttackCbArgs*) args;
	// Add elapsed time.
	attack->elapsed += cbArgs->dt;
	if (
		(attack->target == T_PLAYER || attack->target == T_ALL)
		&& cbArgs->player != NULL
		&& CanPlayerBeHit(cbArgs->player)
	) {
		return AttackHitEntity(cbArgs, &cbArgs->player->entity, attack);
	}

	// Attack that can hit enemies, go over them.
	// Attacks can modify intended enemy status and it's likely there'll be more attacks than enemies,
	// thus we'd rather loop enemies here than attacks on enemy update.
	if (cbArgs->level->entityCount > 0 && (attack->target == T_ENEMY || attack->target == T_ALL)) {
		for (int i = 0; i < cbArgs->level->entityCount; i++) {
			if (!cbArgs->level->entities[i].active) {
				continue;
			}
			if (cbArgs->level->entities[i].entity.invuln.active
				|| !CheckCollisionRecs(
				attack->hitbox,
				HitboxWorldPosition(&cbArgs->level->entities[i].entity)
			)) {
				continue;
			}
			// Attack hit this this entity.
			AttackHitEntity(cbArgs, &cbArgs->level->entities[i].entity, attack);
		}
	}
	return;

	cleanup: RemoveFromPool(pool, index);
}

static void UpdatePlayer(GameContext* context, Level* level, Player* player, float delta) {
	// Check invulnerability status.
	UpdateInvuln(&player->entity, delta);

	// Update physics and status counters.
	// Each entity has their own because they could be individually frozen.
	player->entity.stanceTime += delta;

	// Player is mid dash, no control on actions until it is finished.
	if (player->dash.dashing) {
		return PlayerDashUpdate(player, delta);
	}

	// Update dash cooldown only after it has finished, as it is set at the end of the dash.
	if (player->dash.cdLeft > 0) {
		player->dash.cdLeft -= delta > player->dash.cdLeft ? player->dash.cdLeft : delta;
	}

	// TODO: Add pushback here.
	// Player cannot move or act during a pushback action.

	// Check if player is in the middle of an attack sequence.
	if (player->entity.stance == ATTACKING) {
		Weapon* usedWeapon = player->gear.weapons[player->gear.weaponSlot];
		// If we are here, weapon cannot be null because it was used to start the attack.
		// Check if the attack is complete.
		if (player->entity.stanceTime > usedWeapon->attack->duration) {
			// Attack finished.
			SetStance(&player->entity, STANDING);
		}
	}

	// Movement actions being pressed to pick current direction.
	Direction newDir = PlayerUpdateDirection(player);

	// Execute dash.
	if (player->entity.stance != ATTACKING && IsActionPressed(ACTION_D) && player->dash.cdLeft == 0.0f) {
		return PlayerStartDash(context, player);
	}

	// Attack action.
	if (player->entity.stance != ATTACKING && IsActionPressed(ACTION_A)) {
		Weapon* usedWeapon = player->gear.weapons[player->gear.weaponSlot];
		if (usedWeapon != NULL && usedWeapon->elapsed == 0.0f) {
			if (usedWeapon->attack == NULL) {
				LogDebug("NULL attack on player weapon! %d %f", usedWeapon->type, usedWeapon->cooldown);
				return;
			}
			//usedWeapon->centerDist;
			SetStance(&player->entity, ATTACKING);
			// Create attack.
			Vector2 mpos = GetWorldMousePos(context);
			ActiveAttack att = InitiateAttack(&player->entity, &mpos, usedWeapon->attack, T_ENEMY);
			void* result = AddToPool(&level->attacks, &att);
			if (result == NULL) {
				LogDebug("Failed to allocate character attack on object pool");
			}
		}
	}

	// Execute movement. Last action so other actions that may require directionality take precedence.
	if (newDir != NO_DIRECTION) {
		Vector2 playerDir = DirectionToVector(player->entity.dir);
		player->entity.position = Vector2Add(player->entity.position, (Vector2){
			playerDir.x * PLAYER_SPEED * delta,
			playerDir.y * PLAYER_SPEED * delta,
		});
	}
}

void UpdateLevel(GameContext* context, Player* player, Level* level, float dt) {
	if (level == NULL) {
		return;
	}
	level->playTime += dt;

	if (player != NULL) {
		UpdatePlayer(context, level, player, dt);
		context->state->camera.target = player->entity.position;
	}

	// Run ongoing attacks.
	// Attacks are instantiated by enemies from a template and ran on their on afterwards.
	if (level->attacks.activeItems > 0) {
		AttackCbArgs args = {
			.dt = dt,
			.player = player,
			.level = level,
			.textPool = &level->texts
		};
		IteratePool(&level->attacks, &AttackCallback, &args);
	}

	// Update all active entities.
	if (level->entityCount > 0) {
		for (int i = 0; i < level->entityCount; i++) {
			if (!level->entities[i].active) {
				continue;
			}
			UpdateEnemy(context, player, level, &level->entities[i], dt);
		}
	}
}
