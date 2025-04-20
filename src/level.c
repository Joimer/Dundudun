#include <stdlib.h>
#include <raylib.h>
#include <string.h>
#include "level.h"
#include "lib.h"
#include "game.h"
#include "character.h"

#define ENEMY_DEFAULT_SPEED 150.0f

static Attack attacks[2] = {
	{
		.damage = 5,
		.duration = 0.33f,
		.centerDist = 32.0f,
		.type = 1,
		.data = { .hitbox = { 24.0f, 24.0f } }
	},
	{
		.damage = 6,
		.duration = 0.33f,
		.centerDist = 32.0f,
		.type = 2,
		.data = 24.0f
	}
};

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
			.attack = &attacks[0],
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
				.health = 30,
				.maxHealth = 30,
				.invuln = {},
				.hitbox = { 0, 0, 16, 16 },
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
	if (enemy->attack->type == 1) {
		baseDist += enemy->attack->data.hitbox.height / 2.0f;
	}
	if (enemy->attack->type == 2) {
		baseDist += enemy->attack->data.radius;
	}

	return baseDist + 1.0f;
}

static void UpdateEnemy(GameContext* context, Player* player, Level* level, Enemy* enemy, float dt) {
	// TODO: Own functions for entities for movement/action and state machine for those.
	// Check if player is within the entity's active area.
	if (player == NULL || !IsPointInCircle(
		player->entity.position,
		(Circle){ enemy->entity.position, enemy->activeRadius }
	)) {
		// Inactive status.
		// If can be seen in screen or close by, idle behaviour.
		// Otherwise, completely ignore.
	} else {
		// Check if player is within range of entity attack.
		float maxRange = MaxAttackRange(enemy);
		float dist = Vector2Distance(enemy->entity.position, player->entity.position);

		// In range for attack and no cooldown.
		if (dist <= maxRange && (enemy->lastAttack == 0.0f || enemy->lastAttack + enemy->attackCd < level->playTime)) {
			enemy->lastAttack = level->playTime;
			float angle = Vector2LineAngle(enemy->entity.position, player->entity.position);
			Vector2 attackPos = Vector2Add(
				enemy->entity.position,
				(Vector2){
					.x = cosf(angle) * enemy->attack->centerDist - enemy->attack->data.hitbox.width / 2.0f,
					.y = -(sinf(angle) * enemy->attack->centerDist + enemy->attack->data.hitbox.height / 2.0f)
				}
			);
			Rectangle attackHitbox = {
				.x = attackPos.x,
				.y = attackPos.y,
				.width = enemy->attack->data.hitbox.width,
				.height = enemy->attack->data.hitbox.height
			};
			ActiveAttack att = {
				.attack = enemy->attack,
				//.start = level->playTime,
				.elapsed = 0.0f,
				.hitbox = attackHitbox
			};
			void* result = AddToPool(&level->attacks, &att);
			if (result == NULL) {
				LogDebug("Failed to allocate attack to pool");
			}
			LogDebug("Amount of active items: %d", level->attacks.activeItems);
			return;
		}

		// TODO If mid pushback, cannot move nor attack.

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
				return;
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
				return;
			}
			enemy->entity.dir = (Direction) dir;
			bool isUp = IsBitSet(dir, 4);
			bool isDown = IsBitSet(dir, 3);
			bool isLeft = IsBitSet(dir, 2);
			bool isRight = IsBitSet(dir, 1);

			// Have to check if there's another hitbox in the desired direction.
			float maxSpeed = enemy->speed * dt;

			// Entity hitbox rectangle on its own with the future movement thresholds.
			float neighbourXThres = isRight ? maxSpeed : (isLeft ? -maxSpeed : 0);
			float neighbourYThres = isUp ? -maxSpeed : (isDown ? maxSpeed : 0);

			// Pre-calculate all potential hitboxes so no need to recalcualte for every entity.
			Rectangle hitBoxArea = {
				.x = enemy->entity.position.x + enemy->entity.hitbox.x,
				.y = enemy->entity.position.y + enemy->entity.hitbox.y,
				.width = enemy->entity.hitbox.width,
				.height = enemy->entity.hitbox.height
			};
			Rectangle movedRectX = hitBoxArea;
			movedRectX.x += neighbourXThres;
			Rectangle movedRectY = hitBoxArea;
			movedRectY.y += neighbourYThres;
			Rectangle movedRectBoth = hitBoxArea;
			movedRectBoth.x += neighbourXThres;
			movedRectBoth.y += neighbourYThres;
			Rectangle entityWorldHitbox;

			for (int j = 0; j < level->entityCount; j++) {
				if (!isLeft && !isRight && !isUp && !isDown) {
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
				entityWorldHitbox = (Rectangle){
					.x = level->entities[j].entity.position.x + level->entities[j].entity.hitbox.x,
					.y = level->entities[j].entity.position.y + level->entities[j].entity.hitbox.y,
					.width = level->entities[j].entity.hitbox.width,
					.height = level->entities[j].entity.hitbox.height
				};
				// Check future hitbox for intended movements for each axis.
				// We'll block unavailable movements per axis if necessary.
				if (isLeft) {
					if (DoesRectCollideRect(movedRectX, entityWorldHitbox)) {
						isLeft = false;
					}
				}
				if (isRight) {
					if (DoesRectCollideRect(movedRectX, entityWorldHitbox)) {
						isRight = false;
					}
				}
				if (isUp) {
					if (DoesRectCollideRect((isLeft || isRight ? movedRectBoth : movedRectY), entityWorldHitbox)) {
						isUp = false;
						continue;
					}
				}
				if (isDown) {
					if (DoesRectCollideRect((isLeft || isRight ? movedRectBoth : movedRectY), entityWorldHitbox)) {
						isDown = false;
						continue;
					}
				}
			}

			// If the entity was completely stopped, we can check on next one already.
			if (!isLeft && !isRight && !isUp && !isDown) {
				//LogDebug("Enemy cannot move, obstacle");
				// TODO: Find angle in respect to blocking entity and find a way around it to get to the player.
				return;
			}

			// Final movement.
			float diagSpeed = maxSpeed * 0.7f;
			if (isLeft) {
				enemy->entity.position.x -= (isUp || isDown ? diagSpeed : maxSpeed);
			}
			if (isRight) {
				enemy->entity.position.x += (isUp || isDown ? diagSpeed : maxSpeed);
			}
			if (isUp) {
				enemy->entity.position.y -= (isLeft || isRight ? diagSpeed : maxSpeed);
			}
			if (isDown) {
				enemy->entity.position.y += (isLeft || isRight ? diagSpeed : maxSpeed);
			}
		}
	}
}

void AttackCallback(ObjectPool* pool, int index, void* args) {
	// Ignore CB with invalid args, but does not mean item itself is invalid.
	if (args == NULL) {
		return;
	}
	ActiveAttack* attack = PoolIndexAddress(pool, index);
	if (attack == NULL || attack->attack == NULL) {
		// Warning or something?
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
	if (cbArgs->player != NULL && !cbArgs->player->dash.dashing && !cbArgs->player->entity.invuln.active) {
		Rectangle playerHitbox = HitboxWorldPosition(&cbArgs->player->entity);
		if (attack->attack->type == 1) {
			LogDebug("Checking hit collision");
			// TODO: Potentially 0 damage attacks that have effects.
			if (DoesRectCollideRect(attack->hitbox, playerHitbox) && attack->attack->damage > 0) {
				LogDebug("It did hit");
				// Attack is hitting player.
				cbArgs->player->entity.invuln.active = true;
				// TODO: Instantiate blood splash on ground.
				int damage = attack->attack->damage > cbArgs->player->entity.health ? cbArgs->player->entity.health : attack->attack->damage;
				cbArgs->player->entity.health -= damage;
				Vector2 playerSpritePos = Vector2Subtract(cbArgs->player->entity.position, cbArgs->player->entity.sprite.position);
				float startX = playerSpritePos.x + cbArgs->player->entity.sprite.rect.width / 2.0f;
				ActiveText txt = {
					.content = IntToString(damage),
					.start = (Vector2){ startX, playerSpritePos.y },
					.end = (Vector2){ startX, playerSpritePos.y - 32.0f },
					.startTime = cbArgs->levelTime,
					.endTime = cbArgs->levelTime + 1.0f,
					.fontSize = 15,
					.color = RED
				};
				void* result = AddToPool(cbArgs->textPool, &txt);
				if (result == NULL) {
					LogDebug("Failed to allocate text to pool");
				}
			}
		}
		if (attack->attack->type == 2) {
			/*Circle attackHitbox = {
				.center = attack->center,
				.radius = attack->attack->data.radius
			};*/
			// TODO
		}
	}
	return;

	cleanup: RemoveFromPool(pool, index);
}

void UpdateLevel(GameContext* context, Player* player, Level* level, float dt) {
	if (level == NULL) {
		return;
	}
	level->playTime += dt;

	if (player != NULL) {
		UpdatePlayer(context, player, dt);
		context->state->camera.target = player->entity.position;
	}

	// Run ongoing attacks.
	// Attacks are instantiated by enemies from a template and ran on their on afterwards.
	if (level->attacks.activeItems > 0) {
		AttackCbArgs args = {
			.dt = dt,
			.player = player,
			.levelTime = level->playTime,
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
