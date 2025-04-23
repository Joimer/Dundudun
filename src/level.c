#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <raylib.h>
#include "control.h"
#include "level.h"
#include "lib.h"
#include "game.h"
#include "character.h"
#include "attack.h"

#define ENEMY_DEFAULT_SPEED 150.0f

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

static GameEntity* FindEntityCollisionPoint(Level* level, Vector2* point, GameEntity* self) {
	Rectangle entityWorldHitbox;
	for (int j = 0; j < level->entityCount; j++) {
		if (!level->entities[j].active) {
			continue;
		}
		if (self != NULL && self == &level->entities[j].entity) {
			// Ignore self.
			continue;
		}

		// Check collision with entity.
		entityWorldHitbox = HitboxWorldPosition(&level->entities[j].entity);
		if (CheckCollisionPointRec(*point, entityWorldHitbox)) {
			return &level->entities[j].entity;
		}
	}

	return NULL;
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

Vector2 Raycast(Level* level, Vector2 start, Vector2 end, GameEntity* self) {
	int x0 = (int)floorf(start.x), y0 = (int)floorf(start.y), x1 = (int)ceilf(end.x), y1 = (int)ceilf(end.y);
	int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	int err = dx + dy, e2;
	int max = (abs(err) * 2) + 1;
	Vector2 point = {};
	GameEntity* coll = NULL;

	for (int i = 0; i < max; i++) {
		point.x = x0;
		point.y = y0;
		if (x0 == x1 && y0 == y1) {
			break;
		}
		coll = FindEntityCollisionPoint(level, &point, self);
		if (coll != NULL && (self == NULL || self != coll)) {
			// Next point collides.
			break;
		}
		e2 = 2 * err;
		if (e2 >= dy) {
			err += dy;
			x0 += sx;
		}
		if (e2 <= dx) {
			err += dx;
			y0 += sy;
		}
	}

	return point;
}

static GameEntity* FindEntityCollision(Level* level, GameEntity* self, Rectangle* newPos) {
	if (level == NULL || newPos == NULL) {
		LogDebug("Invalid parameter, null pointer");
		return NULL;
	}
	for (int j = 0; j < level->entityCount; j++) {
		if (!level->entities[j].active || &level->entities[j].entity == NULL) {
			// Ignore inactive entities.
			continue;
		}
		if (self != NULL && self == &level->entities[j].entity) {
			// Ignore self.
			continue;
		}

		// Check collision with entity.
		Rectangle entityWorldHitbox = HitboxWorldPosition(&level->entities[j].entity);
		if (CheckCollisionRecs(entityWorldHitbox, *newPos)) {
			return &level->entities[j].entity;
		}
	}

	return NULL;
}

static bool TestPointDirCollision(Level* level, GameEntity* self, float cornerX, float cornerY, Direction dir) {
	Vector2 point = { cornerX, cornerY };
	Vector2 nextPos = AdvancePointByDir(point, dir, COLL_RAYCAST_DIST);
	Vector2 hit = Raycast(level, point, nextPos, self);
	return (hit.x != ceilf(nextPos.x) || hit.y != ceilf(nextPos.y));
}

// Find if a new position hitbox for the game entity will find an obstacle in the attempted direction.
static bool TestRectDirCollision(Level* level, GameEntity* self, Rectangle hitbox, Direction dir) {
	float cornerX, cornerY;

	// If the movement is diagonal, must first test the corner for that diagonal.
	// If this does not hit, then the other 2 corners in opposite sides.
	if ((IsBitSet(dir, 1) || IsBitSet(dir, 2)) && (IsBitSet(dir, 3) || IsBitSet(dir, 4))) {
		cornerX = IsBitSet(dir, 2) ? hitbox.x : hitbox.x + hitbox.width;
		cornerY = IsBitSet(dir, 3) ? hitbox.y + hitbox.height : hitbox.y;
		if (TestPointDirCollision(level, self, cornerX, cornerY, dir)) {
			return true;
		}
	}

	// Test corner A.
	cornerX = dir == EAST ? hitbox.x + hitbox.width : hitbox.x;
	cornerY = (dir == NORTHWEST || dir == SOUTHEAST || dir == SOUTH) ? hitbox.y + hitbox.height : hitbox.y;
	if (TestPointDirCollision(level, self, cornerX, cornerY, dir)) {
		return true;
	}

	// Test corner B.
	cornerX = dir == WEST ? hitbox.x : hitbox.x + hitbox.width;
	cornerY = (dir == NORTHEAST || dir == EAST || dir == WEST || dir == SOUTH) ? hitbox.y + hitbox.height : hitbox.y;
	return TestPointDirCollision(level, self, cornerX, cornerY, dir);
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
			float xThreshold = player->entity.hitbox.width / 2.0f + enemy->entity.hitbox.width / 2.0f;
			float yThreshold = player->entity.hitbox.height / 2.0f + enemy->entity.hitbox.height / 2.0f;
			float vecDist = Vector2Distance(player->entity.position, enemy->entity.position);

			// Entity is close enough to player, ignore movement.
			if (vecDist < xThreshold + yThreshold) {
				goto stand;
			}

			// Get the closest player hitbox corner to the enemy position.
			Vector2 closestCorner = ClosestRectCorner(HitboxWorldPosition(&player->entity), enemy->entity.position);
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

			// Check if future movement will collide with something.
			// If far away, we check with next hitbox.
			// If getting close 2 tiles, we raycast a tile.
			// We draw a line from both advancing front corners to see if any edge would hit a box.
			// TODO: If a rect is in the way and there is a smaller collision box,
			// it will not be found by raycast from corner, but found by ray from center.
			// Do 3 casts per attempt? Too much? Test 3 rays vs displace rect and test that rect per point.
			bool willCollide = false;
			if (vecDist < COLL_RAYCAST_ACTIVE) {
				Rectangle hitbox = HitboxWorldPosition(&enemy->entity);
				willCollide = TestRectDirCollision(level, &enemy->entity, hitbox, dir);
				// Decided direction collides.
				// If previous direction is different to new one, attempt to follow through.
				if (willCollide && dir != enemy->entity.dir && enemy->entity.dir != NO_DIRECTION) {
					willCollide = TestRectDirCollision(level, &enemy->entity, hitbox, enemy->entity.dir);
					if (!willCollide) {
						dir = enemy->entity.dir;
					}
				}
			} else {
				Vector2 anglev = DirectionToVector(enemy->entity.dir);
				Rectangle newHitbox = HitboxWorldPosition(&enemy->entity);
				newHitbox.x += anglev.x * enemy->speed * dt;
				newHitbox.y += anglev.y * enemy->speed * dt;
				willCollide = (FindEntityCollision(level, &enemy->entity, &newHitbox) != NULL);
			}

			// Entity will collide on new position, try to find another path.
			// We only check with raycasts here, otherwise a far away enemy could do weird pathing before getting close.
			if (willCollide) {
				float angle = DirectionToAngle(dir);
				dir = NO_DIRECTION;
				Rectangle hitbox = HitboxWorldPosition(&enemy->entity);
				// Raycast every 45º to find a decent path around obstacle.
				// Should try closest angles up to opposite angle: +45, -45, +90, -90, +135, -135, +180
				Direction newDir;
				for (int i = 1; i < 8; i++) {
					switch (i) {
						case 1: newDir = AngleToDirection(angle + DEG_45, false); break;
						case 2: newDir = AngleToDirection(angle - DEG_45, false); break;
						case 3: newDir = AngleToDirection(angle + DEG_90, false); break;
						case 4: newDir = AngleToDirection(angle - DEG_90, false); break;
						case 5: newDir = AngleToDirection(angle + DEG_135, false); break;
						case 6: newDir = AngleToDirection(angle - DEG_135, false); break;
						case 7: newDir = AngleToDirection(angle + PI, false); break;
					}
					if (newDir == dir || newDir == enemy->entity.dir) {
						// Ignore directions that have already been tested.
						continue;
					}
					// This direction won't collide, can use it.
					if (!TestRectDirCollision(level, &enemy->entity, hitbox, newDir)) {
						dir = newDir;
						break;
					}
				}
			}

			// If the entity was completely stopped, we can check on next one already.
			if (dir == NO_DIRECTION) {
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
		Rectangle newHitbox = HitboxWorldPosition(&enemy->entity);
		newHitbox.x += enemy->entity.anglev.x * enemy->entity.speed * dt;
		newHitbox.y += enemy->entity.anglev.y * enemy->entity.speed * dt;
		if (FindEntityCollision(level, &enemy->entity, &newHitbox) == NULL) {
			enemy->entity.position = AdvancePointByDir(enemy->entity.position, enemy->entity.dir, enemy->entity.speed * dt);
		} else {
			LogDebug("Will collide, stopped movement!! %f,%f dir %d", newHitbox.x, newHitbox.y, enemy->entity.dir);
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
		.color = attack->target == T_ENEMY ? (Color){ 128, 80, 0, 255 } : RED
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
		player->entity.position = AdvancePointByDir(player->entity.position, player->entity.dir, PLAYER_SPEED * delta);
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
