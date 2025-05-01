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
#include "resource.h"
#include "frame.h"

static bool levelSetup = false;
static Player player;
static Level level;

Player* GetPlayer() {
	return &player;
}

Level* GetLevel() {
	return &level;
}

void DestroyLevel() {
	if (level.rooms != NULL) {
		for (int i = 0; i < level.totalRooms; i++) {
			if (level.rooms[i].tiles != NULL) {
				free(level.rooms[i].tiles);
			}
			if (level.rooms[i].entities != NULL) {
				free(level.rooms[i].entities);
			}
		}
		free(level.rooms);
	}
	level.totalRooms = 0;
	level.currentRoom = NULL;
	level.nextRoom = NULL;
	DestroyPool(&level.attacks);
	DestroyPool(&level.texts);
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

static float SpeedForTile(TileType type) {
	switch (type) {
		case WALL: return 0.0f;
		case GRASS: return 0.75f;
		case GROUND:
		default: return 1.0f;
	}
}

Vector2 RoomOffset(Room* room) {
	if (room->pos.x == 0.0f && room->pos.y == 0.0f) {
		return room->pos;
	}
	return (Vector2){
		room->pos.x * room->columns * TILE_SIZE,
		room->pos.y * room->rows * TILE_SIZE
	};
}

Vector2 RoomOffsetPos(Room* room, int x, int y) {
	Vector2 corner = RoomOffset(room);
	return (Vector2){ corner.x + x * TILE_SIZE, corner.y + y * TILE_SIZE };
}

static void AddRoomExit(
	Room* room, Room* destination,
	int fromX, int fromY,
	int destX, int destY
) {
	const int index = fromX + (room->columns * fromY);
	if (index > room->tileCount - 1) {
		LogDebug("Invalid tile position: %d/%d on starting pos %d,%d", index, room->tileCount, fromX, fromY);
		return;
	}
	Vector2 destPos = Vector2AddValue(RoomOffsetPos(destination, destX, destY), TILE_SIZE / 2.0f);
	LogDebug("Adding exit from %d,%d (index %d, columns %d) to pos %f,%f", fromX, fromY, index, room->columns, destPos.x, destPos.y);
	room->tiles[index].warp.dest = destination;
	room->tiles[index].warp.pos = destPos;
}

static Room GenerateRoom(GameContext* context, Level* level, int num, Vector2 pos) {
	const int entityCount = num == 0 ? 0 : 3;
	// Room that fits world screen: 15x8
	// Default room values.
	// TODO: Other type of room sizes.
	Room room = {
		.pos = pos,
		.tileCount = 200,
		.columns = 15,
		.rows = 8,
		.entityCount = entityCount,
		.entities = NULL,
		.tiles = NULL,
		.complete = entityCount == 0
	};
	if (room.tileCount > 0) {
		room.tiles = malloc(sizeof(Tile) * room.tileCount);
		TileType forGrass = num == 0 ? GROUND : GRASS;
		for (int row = 0; row < room.rows; row++) {
			for (int column = 0; column < room.columns; column++) {
				bool isWall = (row == 0 || column == 0 || column == room.columns - 1 || row == room.rows - 1);
				bool isDoor = (
					(row == 0 || row == room.rows - 1)
					&& (column == room.columns / 2)
				) || (
					(column == 0 || column == room.columns - 1)
					&& (row == room.rows / 2)
				);
				const int index = column + (room.columns * row);
				TileType type = isDoor ? DOOR : (isWall ? WALL : (index % 3 == 0 ? forGrass : GROUND));
				room.tiles[index] = (Tile){
					.type = type,
					.obstacle = isWall && !isDoor,
					.damage = 0,
					.speed = SpeedForTile(type)
				};
			}
		}
	} else {
		LogDebug("Creating room with 0 tiles!!");
	}

	// Entities for enemies that will be in the room.
	if (entityCount > 0) {
		room.entities = malloc(sizeof(Enemy) * entityCount);
		for (int i = 0; i < entityCount; i++) {
			room.entities[i] = (Enemy){
				.active = true,
				.activeRadius = DEFAULT_ENEMY_RADIUS,
				.behaviour = i == 2 ? DISTANCE : APPROACH,
				.speed = ENEMY_DEFAULT_SPEED,
				.attack = i == 1 ? GetAttack(1) : (i == 2 ? GetAttack(4) : GetAttack(0)),
				.lastAttack = 0.0f,
				.attackCd = 2.0f,
				.entity = (GameEntity){
					.sprite = (Sprite){
						.rect = (Rectangle){ 0, 0, 32, 32 },
						.position = (Vector2){ -16, -16 },
						.visible = true,
						.layer = 4
					},
					.position = RoomOffsetPos(&room, 3, 3 + i),
					.health = 40,
					.maxHealth = 40,
					.invuln = (Invulnerability){ .duration = 0.5f },
					.hitbox = { -8, -8, 16, 16 },
					.dir = SOUTH
				}
			};
		}
	}

	return room;
}

static Level GenerateLevel(GameContext* context, int floor) {
	floor = (int) Clamp(floor, 1, MAX_LEVEL);
	// Rooms per level: 6 base, 2 extra per floor, then either 0 or 1 more plus room 0.
	int roomsPerPath = 6 + floor * 2 + (GetRandomMTValue(&context->state->mtrand) % 2);
	Level level = {
		.floor = floor,
		.currentRoom = NULL,
		.swappingRoom = false,
		.totalRooms = roomsPerPath * 4 + 1,
		.nextRoom = NULL
	};
	// Right now we need calloc while testing and initialised rooms are less than total.
	// However, it is likely convenient to initialise room data and set everything to 0 for potential validity checks.
	level.rooms = calloc(level.totalRooms, sizeof(Room));

	// There are 4 ways from the initial room.
	// Every time you pick a door, the other alternative ones remain closed.
	// You can go back to all previously open rooms.
	level.rooms[0] = GenerateRoom(context, &level, 0, (Vector2){ 0, 0 });
	level.currentRoom = &level.rooms[0];
	level.rooms[1] = GenerateRoom(context, &level, 1, (Vector2){ 0, -1 });
	level.rooms[2] = GenerateRoom(context, &level, 2, (Vector2){ 0, 1 });
	level.rooms[3] = GenerateRoom(context, &level, 3, (Vector2){ -1, 0 });
	level.rooms[4] = GenerateRoom(context, &level, 4, (Vector2){ 1, 0 });

	// Add exit rooms.
	// 0-North to 1-South and otherwise.
	AddRoomExit(
		&level.rooms[0], &level.rooms[1],
		level.rooms[0].columns / 2, 0,
		level.rooms[1].columns / 2, level.rooms[1].rows - 2
	);
	AddRoomExit(
		&level.rooms[1], &level.rooms[0],
		level.rooms[1].columns / 2,
		level.rooms[1].rows - 1,
		level.rooms[0].columns / 2, 1
	);

	// South.
	AddRoomExit(
		&level.rooms[0], &level.rooms[2],
		level.rooms[0].columns / 2,
		level.rooms[0].rows - 1,
		level.rooms[2].columns / 2, 1
	);
	AddRoomExit(
		&level.rooms[2], &level.rooms[0],
		level.rooms[2].columns / 2, 0,
		level.rooms[0].columns / 2, level.rooms[0].rows - 2
	);

	// West.
	AddRoomExit(
		&level.rooms[0], &level.rooms[3],
		0, level.rooms[0].rows / 2,
		level.rooms[3].columns - 2,
		level.rooms[3].rows / 2
	);
	AddRoomExit(
		&level.rooms[3], &level.rooms[0],
		level.rooms[3].columns - 1,
		level.rooms[3].rows / 2,
		1, level.rooms[0].rows / 2
	);

	// East.
	AddRoomExit(
		&level.rooms[0], &level.rooms[4],
		level.rooms[0].columns - 1,
		level.rooms[0].rows / 2,
		1, level.rooms[4].rows / 2
	);
	AddRoomExit(
		&level.rooms[4], &level.rooms[0],
		0, level.rooms[4].rows / 2,
		level.rooms[0].columns - 2,
		level.rooms[0].rows / 2
	);

	/*int currentRoom = 1;
	for (int path = 1; path < 5; path++) {
		Room nextRoom;
		// First room on a path will always only contain a single exit continuing onwards.
		if (currentRoom == 1) {

		}
		rooms->rooms[currentRoom++] = nextRoom;
	}*/
	// Number of attacks to allocate should be calculated by max enemies and their attack cadence.
	level.attacks = CreatePoolOf(ActiveAttack, 64);
	level.texts = CreatePoolOf(ActiveText, 64);

	return level;
}

void SetupLevel(GameContext* context) {
	if (levelSetup) {
		return;
	}
	Texture2D* characterTexture = GetTexture(PLAYER_TEXTURE);
	player = CreatePlayer(characterTexture);
	level = GenerateLevel(context, 1);
	levelSetup = true;
}

static Tile* GetTileByPos(Room* room, Vector2* pos) {
	if (room == NULL || pos == NULL || room->tileCount == 0 || room->rows == 0 || room->columns == 0) {
		LogDebug("Invalid parameters");
		return NULL;
	}

	// We will use room offset to calculate indexing as in room 0.
	// Then, w use tile size to determine index from pixel position.
	Vector2 roomOffset = RoomOffset(room);
	const int x = (int)((pos->x - roomOffset.x) / TILE_SIZE);
	const int y = (int)((pos->y - roomOffset.y) / TILE_SIZE);
	const int index = x + (room->columns * y);
	if (index < room->tileCount) {
		return &room->tiles[index];
	}

	return NULL;
}

static GameEntity* FindEntityCollisionPoint(Room* room, Vector2* point, GameEntity* self) {
	Rectangle entityWorldHitbox;
	for (int j = 0; j < room->entityCount; j++) {
		if (!room->entities[j].active) {
			continue;
		}
		if (self != NULL && self == &room->entities[j].entity) {
			// Ignore self.
			continue;
		}

		// Check collision with entity.
		entityWorldHitbox = HitboxWorldPosition(&room->entities[j].entity);
		if (CheckCollisionPointRec(*point, entityWorldHitbox)) {
			return &room->entities[j].entity;
		}
	}

	return NULL;
}

static void MoveEntityByForce(Room* room, GameEntity* entity, float force) {
	Tile* tile = GetTileByPos(room, &entity->position);
	if (tile == NULL) {
		LogDebug("Invalid tile!!");
		return;
	}
	Vector2 newPos = AdvancePointByVector(entity->position, entity->anglev, entity->speed * tile->speed * force);
	Tile* newTile = GetTileByPos(room, &newPos);
	// Would hit an obstacle on next tile, stop movement.
	if (newTile->obstacle || (newTile->type == DOOR && !room->complete)) {
		entity->speed = 0.0f;
	} else {
		entity->position = newPos;
	}
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

Vector2 Raycast(Room* room, Vector2 start, Vector2 end, GameEntity* self) {
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
		coll = FindEntityCollisionPoint(room, &point, self);
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

static GameEntity* FindEntityCollision(Room* room, GameEntity* self, Rectangle* newPos) {
	if (room == NULL || newPos == NULL) {
		LogDebug("Invalid parameter, null pointer");
		return NULL;
	}
	for (int j = 0; j < room->entityCount; j++) {
		if (!room->entities[j].active || &room->entities[j].entity == NULL) {
			// Ignore inactive entities.
			continue;
		}
		if (self != NULL && self == &room->entities[j].entity) {
			// Ignore self.
			continue;
		}

		// Check collision with entity.
		Rectangle entityWorldHitbox = HitboxWorldPosition(&room->entities[j].entity);
		if (CheckCollisionRecs(entityWorldHitbox, *newPos)) {
			return &room->entities[j].entity;
		}
	}

	return NULL;
}

static bool TestPointDirCollision(Room* room, GameEntity* self, float cornerX, float cornerY, Direction dir) {
	Vector2 point = { cornerX, cornerY };
	Vector2 nextPos = AdvancePointByDir(point, dir, COLL_RAYCAST_DIST);
	Vector2 hit = Raycast(room, point, nextPos, self);
	return (hit.x != ceilf(nextPos.x) || hit.y != ceilf(nextPos.y));
}

// Find if a new position hitbox for the game entity will find an obstacle in the attempted direction.
static bool TestRectDirCollision(Room* room, GameEntity* self, Rectangle hitbox, Direction dir) {
	float cornerX, cornerY;

	// If the movement is diagonal, must first test the corner for that diagonal.
	// If this does not hit, then the other 2 corners in opposite sides.
	if ((IsBitSet(dir, 1) || IsBitSet(dir, 2)) && (IsBitSet(dir, 3) || IsBitSet(dir, 4))) {
		cornerX = IsBitSet(dir, 2) ? hitbox.x : hitbox.x + hitbox.width;
		cornerY = IsBitSet(dir, 3) ? hitbox.y + hitbox.height : hitbox.y;
		if (TestPointDirCollision(room, self, cornerX, cornerY, dir)) {
			return true;
		}
	}

	// Test corner A.
	cornerX = dir == EAST ? hitbox.x + hitbox.width : hitbox.x;
	cornerY = (dir == NORTHWEST || dir == SOUTHEAST || dir == SOUTH) ? hitbox.y + hitbox.height : hitbox.y;
	if (TestPointDirCollision(room, self, cornerX, cornerY, dir)) {
		return true;
	}

	// Test corner B.
	cornerX = dir == WEST ? hitbox.x : hitbox.x + hitbox.width;
	cornerY = (dir == NORTHEAST || dir == EAST || dir == WEST || dir == SOUTH) ? hitbox.y + hitbox.height : hitbox.y;
	return TestPointDirCollision(room, self, cornerX, cornerY, dir);
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
	UpdateStun(&enemy->entity, dt);

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
			// Shooting attack.
			bool doAttack = enemy->attack->speed > 0.0f;

			// Check if player is within range of entity attack.
			if (!doAttack) {
				float maxRange = MaxAttackRange(enemy);
				float dist = Vector2Distance(enemy->entity.position, player->entity.position);
				doAttack = dist <= maxRange;
			}

			// In range for attack and no cooldown.
			if (doAttack) {
				// Initiate attack and finish.
				SetStance(&enemy->entity, ATTACKING);
				enemy->lastAttack = level->playTime;
				return;
			}
		}

		// Distance from entity to player.
		float vecDist = Vector2Distance(player->entity.position, enemy->entity.position);
		if (enemy->behaviour == DISTANCE) {
			// Check if vecDist is less than radius x 1.9 (within attack range but not border)
		}
		// TODO
		//if (enemy->behaviour == DISTANCE) {
		// Pick target position, speed
		// Check collision
		// Run action
		// Check collision after checking movement for either type of movement.

		if (enemy->behaviour == APPROACH) {
			// Set direction towards player.
			// Min distance is entity hitbox in front of player hitbox.
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
				willCollide = TestRectDirCollision(level->currentRoom, &enemy->entity, hitbox, dir);
				// Decided direction collides.
				// If previous direction is different to new one, attempt to follow through.
				if (willCollide && dir != enemy->entity.dir && enemy->entity.dir != NO_DIRECTION) {
					willCollide = TestRectDirCollision(level->currentRoom, &enemy->entity, hitbox, enemy->entity.dir);
					if (!willCollide) {
						dir = enemy->entity.dir;
					}
				}
			} else {
				Vector2 anglev = DirectionToVector(enemy->entity.dir);
				Rectangle newHitbox = HitboxWorldPosition(&enemy->entity);
				newHitbox.x += anglev.x * enemy->speed * dt;
				newHitbox.y += anglev.y * enemy->speed * dt;
				willCollide = (FindEntityCollision(level->currentRoom, &enemy->entity, &newHitbox) != NULL);
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
					if (!TestRectDirCollision(level->currentRoom, &enemy->entity, hitbox, newDir)) {
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
	// Collision checks to be done before this.
	if (enemy->entity.speed > 0.0f) {
		Tile* tile = GetTileByPos(level->currentRoom, &enemy->entity.position);
		if (tile == NULL) {
			LogDebug("Invalid tile!");
		} else {
			enemy->entity.position = AdvancePointByDir(enemy->entity.position, enemy->entity.dir, enemy->entity.speed * tile->speed * dt);
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
		doesHit = CheckCollisionRecs(attack->hitbox.rect, hitbox);
	}
	if (attack->attack->type == 2) {
		doesHit = CheckCollisionCircleRec(attack->center, attack->hitbox.radius, hitbox);
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

static void AttackCallback(ObjectPool* pool, int index, void* args) {
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

	// Change attack position if it moves.
	if (attack->attack->speed > 0.0f) {
		const float xAdvance = attack->angle.x * attack->attack->speed * cbArgs->dt;
		const float yAdvance = attack->angle.y * attack->attack->speed * cbArgs->dt;
		attack->center = Vector2Add(attack->center, (Vector2){
			xAdvance,
			yAdvance
		});
		if (attack->attack->type == HB_RECT) {
			attack->hitbox.rect.x += xAdvance;
			attack->hitbox.rect.y += yAdvance;
		}
	}

	// Check if it hits the player.
	if (
		(attack->target == T_PLAYER || attack->target == T_ALL)
		&& cbArgs->player != NULL
		&& CanPlayerBeHit(cbArgs->player)
	) {
		AttackHitEntity(cbArgs, &cbArgs->player->entity, attack);
	}

	// Attack that can hit enemies, go over them.
	// Attacks can modify intended enemy status and it's likely there'll be more attacks than enemies,
	// thus we'd rather loop enemies here than attacks on enemy update.
	Room* room = cbArgs->level->currentRoom;
	if (room->entityCount > 0 && (attack->target == T_ENEMY || attack->target == T_ALL)) {
		for (int i = 0; i < room->entityCount; i++) {
			if (!room->entities[i].active) {
				continue;
			}
			if (room->entities[i].entity.invuln.active) {
				continue;
			}
			// Check if attack hits the entity and process it.
			AttackHitEntity(cbArgs, &room->entities[i].entity, attack);
		}
	}
	return;

	cleanup: RemoveFromPool(pool, index);
}

static void TriggerRoomChange(Level* level, Room* room) {
	if (level == NULL || level->swappingRoom || room == NULL) {
		LogDebug("Invalid parameters");
		return;
	}
	level->nextRoom = room;
	level->swappingRoom = true;
	level->playTime = 0.0f;
}

static void UpdatePlayer(GameContext* context, Level* level, Player* player, float delta) {
	if (level->swappingRoom) {
		return;
	}

	// Check if we hit an exit.
	// TODO: Implement opening doors by bombs when bombs are implemented.
	if (level->currentRoom->complete && !level->swappingRoom) {
		Tile* tile = GetTileByPos(level->currentRoom, &player->entity.position);
		if (tile != NULL && tile->warp.dest != NULL) {
			LogDebug("Warping player to %f,%f", tile->warp.pos.x, tile->warp.pos.y);
			// Set player to the warp position.
			context->state->lastCamPos = context->state->camera.target;
			player->entity.position = tile->warp.pos;
			return TriggerRoomChange(level, tile->warp.dest);
		}
	}

	// Update physics and status counters.
	// Each entity has their own because they could be individually frozen.
	player->entity.stanceTime += delta;

	// Check invulnerability status.
	UpdateInvuln(&player->entity, delta);

	// Stun status.
	UpdateStun(&player->entity, delta);

	// Update weapon statuses.
	UpdateWeaponStatus(player, delta);

	// Player is mid dash, no control on actions until it is finished.
	if (player->dash.dashing) {
		PlayerDashUpdate(player, delta);
		return MoveEntityByForce(level->currentRoom, &player->entity, delta);
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
	if (!player->entity.stunned && player->entity.stance != ATTACKING && IsActionActive(ACTION_D) && player->dash.cdLeft == 0.0f) {
		return PlayerStartDash(context, player);
	}

	// Attack action.
	if (!player->entity.stunned && player->entity.stance != ATTACKING && IsActionActive(ACTION_A)) {
		Weapon* usedWeapon = player->gear.weapons[player->gear.weaponSlot];
		if (usedWeapon != NULL && !usedWeapon->attacking) {
			if (usedWeapon->attack == NULL) {
				LogDebug("NULL attack on player weapon! %d %f", usedWeapon->type, usedWeapon->cooldown);
				return;
			}
			SetStance(&player->entity, ATTACKING);
			// Create attack.
			//usedWeapon->attacking = true;
			//usedWeapon->elapsed = 0.0f;
			Vector2 mpos = GetWorldMousePos(context);
			ActiveAttack att = InitiateAttack(&player->entity, &mpos, usedWeapon->attack, T_ENEMY);
			void* result = AddToPool(&level->attacks, &att);
			if (result == NULL) {
				LogDebug("Failed to allocate character attack on object pool");
			} else {
				usedWeapon->attacking = true;
				usedWeapon->elapsed = 0.0f;
			}
		}
	}

	// Execute movement. Last action so other actions that may require directionality take precedence.
	if (newDir != NO_DIRECTION) {
		MoveEntityByForce(level->currentRoom, &player->entity, delta);
	}
}

void UpdateLevel(GameContext* context, float dt) {
	// During first update we set up the level.
	// TODO: Loading screen step for this
	if (!levelSetup) {
		SetupLevel(context);
	}
	level.playTime += dt;

	// If we are in the middle of changing rooms, we can ignore the other updates.
	if (level.swappingRoom) {
		if (level.playTime > ROOM_CHANGE_TIME) {
			level.swappingRoom = false;
			level.playTime = 0.0f;
			level.currentRoom = level.nextRoom;
			level.nextRoom = NULL;
		}
		// Even if the movement was finished, we don't run updates yet until next frame.
		return;
	}

	UpdatePlayer(context, &level, &player, dt);

	// Run ongoing attacks.
	// Attacks are instantiated by enemies from a template and ran on their own afterwards.
	if (level.attacks.activeItems > 0) {
		AttackCbArgs args = {
			.dt = dt,
			.player = &player,
			.level = &level,
			.textPool = &level.texts
		};
		IteratePool(&level.attacks, &AttackCallback, &args);
	}

	// Update all active entities on current room.
	int activeEntities = 0;
	if (level.currentRoom != NULL && level.currentRoom->entityCount > 0) {
		for (int i = 0; i < level.currentRoom->entityCount; i++) {
			if (!level.currentRoom->entities[i].active) {
				continue;
			}
			UpdateEnemy(context, &player, &level, &level.currentRoom->entities[i], dt);
			activeEntities++;
		}
	}

	// Check if room has been completed to open doors.
	if (level.currentRoom != NULL && !level.currentRoom->complete && activeEntities == 0) {
		level.currentRoom->complete = true;
	}
}
