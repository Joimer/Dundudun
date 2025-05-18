#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <raylib.h>
#include "control.h"
#include "level.h"
#include "event.h"
#include "item.h"
#include "lib.h"
#include "game.h"
#include "player.h"
#include "attack.h"
#include "resource.h"
#include "frame.h"
#include "entity.h"
#include "boss.h"

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
	if (level.items != NULL) {
		free(level.items);
		level.itemCount = 0;
	}
}

void DestroyPlayer() {
	if (player.gear.weapons != NULL) {
		free(player.gear.weapons);
		player.gear.equippedWeaps = 0;
	}
	free(player.relics);
	player.relicCount = 0;
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
	const Vector2 corner = RoomOffset(room);
	return (Vector2){ corner.x + x * TILE_SIZE, corner.y + y * TILE_SIZE };
}

static void CreateDamageText(
	GameEntity* entity, int damage, DamageType type, float playTime, ObjectPool* textPool
) {
	const Vector2 spritePos = Vector2Subtract(entity->position, entity->sprite.position);
	const float startX = spritePos.x + entity->sprite.rect.width / 2.0f;
	ActiveText txt = {
		.content = IntToString(damage),
		.start = (Vector2){ startX, spritePos.y },
		.end = (Vector2){ startX, spritePos.y - 32.0f },
		.startTime = playTime,
		.endTime = playTime + 1.0f,
		.fontSize = 15,
		.color = type == D_POISON ? LIME : RED
	};
	void* result = AddToPool(textPool, &txt);
	if (result == NULL) {
		LogDebug("Failed to allocate text to pool");
	}
}

void onEntityDamage(Event* ev) {
	// ufbo
	Level* level = GetLevel();
	CreateDamageText(
		ev->params.dmg.entity,
		ev->params.dmg.amount,
		ev->params.dmg.type,
		level->playTime,
		&level->texts
	);
}

static void AddRoomExit(
	Room* room, Room* destination,
	int fromX, int fromY,
	int destX, int destY
) {
	if (room == NULL || destination == NULL) {
		LogDebug("Invalid room from %d to %d", (room == NULL), (destination == NULL));
		return;
	}
	const int index = fromX + (room->columns * fromY);
	if (index > room->tileCount - 1) {
		LogDebug("Invalid tile position: %d/%d on starting pos %d,%d", index, room->tileCount, fromX, fromY);
		return;
	}
	const Vector2 destPos = Vector2AddValue(RoomOffsetPos(destination, destX, destY), TILE_SIZE / 2.0f);
	LogDebug("Adding exit from %d,%d (index %d, columns %d) to pos %f,%f", fromX, fromY, index, room->columns, destPos.x, destPos.y);
	room->tiles[index].warp.dest = destination;
	room->tiles[index].warp.pos = destPos;
	room->tiles[index].warp.touched = false;
	room->tiles[index].type = DOOR;
	room->tiles[index].obstacle = false;
	// TODO: Not sure if the best way to do this tbh.
	if (fromX == destX && fromY > destY) {
		room->tiles[index].rotation = 180.0f;
	} else if (fromX > destX) {
		room->tiles[index].rotation = 90.0f;
	} else if (fromX < destX) {
		room->tiles[index].rotation = 270.0f;
	}
}

static Room GenerateRoom(
	GameContext* context, Level* level, int num, Vector2 pos, ItemType reward, int rewardAmount, RoomType type
) {
	// Room that fits world screen: 15x8
	// Default room values.
	// TODO: Other type of room sizes.
	Room room = {
		.roomNo = num,
		.type = type,
		.pos = pos,
		.tileCount = 200,
		.columns = 15,
		.rows = 8,
		.entityCount = 0,
		.entities = NULL,
		.tiles = NULL,
		.complete = num == 0,
		.reward = reward,
		.rewardAmount = rewardAmount,
		.visited = num == 0,
	};
	if (room.tileCount > 0) {
		room.tiles = malloc(sizeof(Tile) * room.tileCount);
		const TileType forGrass = num == 0 ? GROUND : GRASS;
		for (int row = 0; row < room.rows; row++) {
			for (int column = 0; column < room.columns; column++) {
				const bool isWall = (row == 0 || column == 0 || column == room.columns - 1 || row == room.rows - 1);
				const int index = column + (room.columns * row);
				const TileType type = isWall ? WALL : (index % 6 == 0 ? forGrass : GROUND);
				room.tiles[index] = (Tile){
					.type = type,
					.obstacle = isWall,
					.damage = 0,
					.speed = SpeedForTile(type),
					.rotation = 0.0f,
				};
			}
		}
	} else {
		LogDebug("Creating room with 0 tiles!!");
	}

	// Entities for enemies that will be in the room.
	if (room.type == ENCOUNTER) {
		// Pick enemy group apt for the room number and area.
		// TODO: Actually consider that
		// TODO: First rooms less weight on easy groups, ascension makes them same weight.
		int groupId = GetRandomMTValue(&context->state->mtrand) % 6;
		const EnemyGroup* group = GetEnemyGroup(groupId);
		room.entityCount = group->count;
		// Allocate enough for enemies and for 99 bombs.
		room.entities = calloc(sizeof(ActiveEnemy), group->count + 99);
		for (int i = 0; i < group->count; i++) {
			room.entities[i] = InstantiateEnemy(
				GetEnemy(group->enemies[i].enemyId),
				RoomOffsetPos(&room, group->enemies[i].pos.x, group->enemies[i].pos.y)
			);
		}
	} else {
		// Make site for bombs and such on non encounter rooms.
		room.entities = calloc(sizeof(ActiveEnemy),99);
	}

	// Pick boss for the path according to the level.
	if (room.type == BOSS) {
		LogDebug("montando room de boss");
		const Boss* boss = GetBoss(context, level->floor);
		room.boss = InstantiateBoss(
			boss,
			RoomOffsetPos(&room, room.columns / 2, room.rows / 2)
		);
	} else {
		room.boss.active = false;
	}

	return room;
}

static void CalcAddRoomExit(Level* level, int prevRoomId, int currentRoomId, Direction previousDir) {
	int fromX = 0, fromY = 0, destX = 1, destY = 1;
	switch (previousDir) {
		case SOUTH:
			fromX = level->rooms[prevRoomId].columns / 2;
			destX = level->rooms[currentRoomId].columns / 2;
			destY = level->rooms[currentRoomId].rows - 2;
			break;
		case NORTH:
			fromX = level->rooms[prevRoomId].columns / 2;
			fromY = level->rooms[prevRoomId].rows - 1;
			destX = level->rooms[currentRoomId].columns / 2;
			break;
		case WEST:
			fromY = level->rooms[prevRoomId].rows / 2;
			destX = level->rooms[currentRoomId].columns - 2;
			destY = level->rooms[currentRoomId].rows / 2;
			break;
		case EAST:
			fromY = level->rooms[prevRoomId].rows / 2;
			fromX = level->rooms[prevRoomId].columns - 1;
			destY = level->rooms[currentRoomId].rows / 2;
			break;
		default: break;
	}
	LogDebug("Adding exit on room %d on %d,%d to next room %d to %d,%d", prevRoomId, fromX, fromY, currentRoomId, destX, destY);
	AddRoomExit(
		&level->rooms[prevRoomId], &level->rooms[currentRoomId],
		fromX, fromY,
		destX, destY
	);
}

static bool IsRoomForbidden(int x, int y) {
	if (
		(x == -1 && y == -1)
		|| (x == 1 && y == -1)
		|| (x == 1 && y == 1)
		|| (x == -1 && y == 1)
	) {
		return true;
	}

	return false;
}

static bool IsFinalOrAdjacent(Level* level, int finalRoomNo, int x, int y) {
	if (level->rooms == NULL || (x == 0 && y == 0)) {
		return false;
	}
	// Find all final rooms and see if the wanted position lands on them or adjacent.
	for (int i = 0; i < level->totalRooms; i++) {
		if (
			level->rooms[i].roomNo == finalRoomNo
			&& (
				(x == level->rooms[i].pos.x && y == level->rooms[i].pos.y)
				|| (x == level->rooms[i].pos.x - 1 && y == level->rooms[i].pos.y)
				|| (x == level->rooms[i].pos.x && y == level->rooms[i].pos.y - 1)
				|| (x == level->rooms[i].pos.x + 1 && y == level->rooms[i].pos.y)
				|| (x == level->rooms[i].pos.x && y == level->rooms[i].pos.y + 1)
			)
		) {
			return true;
		}
	}

	return false;
}

// I think this is probably more efficient that storing a double key dict for rooms
// should probably test it or something idk muh cache lines etc.
static Room* FindRoom(Level* level, int x, int y) {
	if (level->rooms == NULL) {
		return NULL;
	}
	if (x == 0 && y == 0) {
		return &level->rooms[0];
	}
	for (int i = 0; i < level->totalRooms; i++) {
		if (level->rooms[i].roomNo == 0) {
			continue;
		}
		if (level->rooms[i].pos.x == x && level->rooms[i].pos.y == y) {
			return &level->rooms[i];
		}
	}

	return NULL;
}

static bool CanPlaceRoom(Level* level, int x, int y, Direction prevDir, int path) {
	// Check path boundaries to avoid path collision.
	// Also check forbidden rooms.
	if (
		(path == 1 && y < 2)
		|| (path == 2 && y > -2)
		|| (path == 3 && x > -2)
		|| (path == 4 && x < 2)
		|| IsRoomForbidden(x, y)
	) {
		return false;
	}

	// Find whether x,y is free and would not lead to a room with non adjacent rooms.
	if (
		FindRoom(level, x, y) != NULL
		|| (prevDir != NORTH && FindRoom(level, x, y - 1) != NULL)
		|| (prevDir != SOUTH && FindRoom(level, x, y + 1) != NULL)
		|| (prevDir != WEST && FindRoom(level, x + 1, y) != NULL)
		|| (prevDir != EAST && FindRoom(level, x - 1, y) != NULL)
	) {
		return false;
	}

	return true;
}

static Direction GetNextViableDir(
	Level* level, int currX, int currY, int path, Direction nextDir, Direction comeFrom, bool checkNext
);

static Direction CheckNextDir(Level* level, int x, int y, Direction nextDir, int path, bool checkNext) {
	switch (nextDir) {
		case NORTH: y++; break;
		case SOUTH: y--; break;
		case WEST: x--; break;
		case EAST: x++; break;
		default: break;
	}
	if (CanPlaceRoom(level, x, y, nextDir, path)) {
		// If we have to check one step ahead too, we re-call.
		if (checkNext) {
			if  (GetNextViableDir(level, x, y, path, nextDir, OppositeDir(nextDir), false) == NO_DIRECTION) {
				return NO_DIRECTION;
			}
		}
		return nextDir;
	}

	return NO_DIRECTION;
}

static Direction GetNextViableDir(
	Level* level, int currX, int currY, int path,
	Direction nextDir, Direction comeFrom, bool checkNext
) {
	if (CheckNextDir(level, currX, currY, nextDir, path, checkNext) != NO_DIRECTION) {
		return nextDir;
	}
	if (
		comeFrom != NORTH && nextDir != NORTH
		&& CheckNextDir(level, currX, currY, NORTH, path, checkNext) != NO_DIRECTION
	) {
		return NORTH;
	}
	if (
		comeFrom != SOUTH && nextDir != SOUTH
		&& CheckNextDir(level, currX, currY, SOUTH, path, checkNext) != NO_DIRECTION
	) {
		return SOUTH;
	}
	if (
		comeFrom != WEST && nextDir != WEST
		&& CheckNextDir(level, currX, currY, WEST, path, checkNext) != NO_DIRECTION
	) {
		return WEST;
	}
	if (
		comeFrom != EAST && nextDir != EAST
		&& CheckNextDir(level, currX, currY, EAST, path, checkNext) != NO_DIRECTION
	) {
		return EAST;
	}

	return NO_DIRECTION;
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
		.nextRoom = NULL,
		.itemCount = 0,
		.items = NULL
	};
	// Allocations that are run once per level or game execution should be set to 0.
	level.rooms = calloc(level.totalRooms, sizeof(Room));

	// There are 4 ways from the initial room.
	// Every time you pick a door, the other alternative ones remain closed.
	// You can go back to all previously open rooms.
	ItemType reward = I_NONE;
	level.rooms[0] = GenerateRoom(context, &level, 0, (Vector2){ 0, 0 }, reward, 0, HALL);
	level.currentRoom = &level.rooms[0];

	int roomId = 1;
	for (int path = 1; path < 5; path++) {
		LogDebug("Starting path %d", path);
		Direction pathDir;
		switch (path) {
			case 1: pathDir = NORTH; break;
			case 2: pathDir = SOUTH; break;
			case 3: pathDir = WEST; break;
			case 4: pathDir = EAST; break;
		}
		Direction nextDir;
		Direction previousDir = pathDir;
		int prevDirCount = 1;

		for (int currentRoom = 1; currentRoom < roomsPerPath + 1; currentRoom++) {
			// Since the 4 initial rooms are chosen pathwise, we decide direction of the next room
			// at the end of this loop. Initial entry will be preset with the proper initial room per path.
			int prevRoomId = currentRoom == 1 ? 0 : roomId - 1;
			int currentRoomId = roomId;
			int nextRoomId = ++roomId;

			// We generate the room struct before finalising position so we are able to do several checks on next placement.
			LogDebug("Room num %d Generating room %d", currentRoom, currentRoomId);
			// Calculate current room X and Y according to the room we came  from.
			int roomX = level.rooms[prevRoomId].pos.x, roomY = level.rooms[prevRoomId].pos.y;
			switch (previousDir) {
				case NORTH: roomY++; break;
				case SOUTH: roomY--; break;
				case WEST: roomX--; break;
				case EAST: roomX++; break;
				default: break;
			}
			// Pick reward for the room.
			int rewardAmount = 1;
			bool isBoss = currentRoom == roomsPerPath;
			if (currentRoom == 6 || isBoss) {
				reward = I_RELIC;
			} else {
				// For now, let's just do this.
				int dice = GetRandomMTValue(&context->state->mtrand) % 3;
				switch (dice) {
					case 0: reward = I_BOMB; break;
					case 1: reward = I_KEY; break;
					case 2:
						reward = I_EXP;
						rewardAmount = (GetRandomMTValue(&context->state->mtrand) % 3) + 2;
						break;
				}
			}
			// TODO: Pick here when rooms are shops.
			// Boss reward is always a relic as well.
			level.rooms[currentRoomId] = GenerateRoom(
				context, &level, currentRoomId, (Vector2){ roomX, roomY }, reward, rewardAmount, isBoss ? BOSS : ENCOUNTER
			);

			LogDebug("Adding room exits.");
			// Previous room exit can already be added, since here we already got which direction we came from.
			CalcAddRoomExit(&level, prevRoomId, currentRoomId, previousDir);
			CalcAddRoomExit(&level, currentRoomId, prevRoomId, OppositeDir(previousDir));
			//LogDebug("-- Placing room in %d,%d", roomX, roomY);
			if (level.maxX < roomX) {
				level.maxX = roomX;
			}
			if (level.minX > roomX) {
				level.minX = roomX;
			}
			if (level.maxY < roomY) {
				level.maxY = roomY;
			}
			if (level.minY > roomY) {
				level.minY = roomY;
			}

			// If it's the last room on the path, there is no need to calculate next position.
			// Continue onwards to the next path carving.
			if (isBoss) {
				LogDebug("Path done");
				break;
			}

			// First room on a path will always only contain a single exit continuing onwards.
			if (currentRoom == 1) {
				//LogDebug("First room, next path is repeated");
				nextDir = pathDir;
			} else {
				//LogDebug("Find out next direction");
				// North, south, east, west only.
				// It's computationally cheap to have an index until last used direction and use the enum to access.
				float chances[9] = { 0 };
				// The chance to repeat the previous direction starts slightly higher than the rest.
				// Then, it decreases on every step. This helps getting separate paths that aren't
				// going diagonally all the time.
				float preDirChance = 40.0f / (float)prevDirCount;

				// Chances to go on the path's initial way are higher unless repeated too often.
				float chanceRemainder;
				if (previousDir == pathDir || OppositeDir(previousDir) == pathDir) {
					chances[previousDir] = preDirChance;
					chances[pathDir] *= 1.05f;
					chanceRemainder = (100.0f - chances[previousDir]) / 2.0f;
					for (int i = 1; i < 9; i++) {
						if (
							(i != 1 && i != 2 && i != 4 && i != 8)
							|| i == pathDir
							|| i == OppositeDir(previousDir)
						) {
							continue;
						}
						chances[i] = chanceRemainder;
					}
				} else {
					chanceRemainder = 100.0f - preDirChance;
					float pathChance = (chanceRemainder / 2.0f) * 1.05f;
					float otherChance = chanceRemainder - pathChance;
					for (int i = 1; i < 9; i++) {
						chances[i] = otherChance;
					}
					chances[previousDir] = preDirChance;
					chances[pathDir] = pathChance;
					chances[OppositeDir(previousDir)] = 0.0f;
				}

				// Having decided the chance for each new direction, let's get roll to decide.
				float dice = (GetRandomMTValue(&context->state->mtrand) % 100) + 1;
				float acc = 0;
				for (int i = 1; i < 9; i++) {
					if ((i != 1 && i != 2 && i != 4 && i != 8) || chances[i] == 0.0f) {
						continue;
					}
					acc += chances[i];
					if (acc >= dice) {
						nextDir = (Direction) i;
						break;
					}
				}

				// Unless it is the first room on the path, it is possible to try to go to a used or blocked room.
				// Only check next room if there's 2 rooms to go, otherwise we can just place the next.
				nextDir = GetNextViableDir(
					&level,
					level.rooms[currentRoomId].pos.x,
					level.rooms[currentRoomId].pos.y,
					path,
					nextDir,
					OppositeDir(previousDir),
					path > 1 && currentRoom < roomsPerPath - 1
				);
				if (nextDir == NO_DIRECTION) {
					LogDebug(
						"-- Dead end trying to find next room!! room %d on path %d attempted %d,%d",
						currentRoom, path, (int)level.rooms[currentRoomId].pos.x, (int)level.rooms[currentRoomId].pos.y
					);
					break;
				}

				if (nextDir == previousDir) {
					prevDirCount++;
				} else {
					prevDirCount = 1;
				}
				previousDir = nextDir;
			}
		}

		// Alternative routes
		// TODO :)
	}

	// Number of attacks to allocate should be calculated by max enemies and their attack cadence.
	level.attacks = CreatePoolOf(ActiveAttack, 128);
	level.texts = CreatePoolOf(ActiveText, 128);

	return level;
}

void SetupLevel(GameContext* context) {
	if (levelSetup) {
		return;
	}
	Texture2D* characterTexture = GetTexture(PLAYER_TEXTURE);
	player = CreatePlayer(characterTexture);
	level = GenerateLevel(context, 1);
	SetupEntityEvents();
	SubEvent(GetEntityEvents(), E_DMG, &onEntityDamage);
	SetupPlayerEvents();
	SubEvent(GetPlayerEvents(), E_PLAYER_HIT, &onPlayerHit);
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

	// TODO? Every time the entities are checked in a loop it'd be cool to do an iterator or something
	// and add the boss or player or whatever too, in the end it's all game entities,,,
	// Think about it.
	if (room->boss.active) {
		entityWorldHitbox = HitboxWorldPosition(&room->boss.entity);
		if (CheckCollisionPointRec(*point, entityWorldHitbox)) {
			return &room->boss.entity;
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
	float speed = GetEntitySpeed(entity);
	if (speed == 0.0f) {
		return StandStill(entity);
	}
	Vector2 newPos = AdvancePointByVector(entity->position, entity->anglev, speed * tile->speed * force);
	Tile* newTile = GetTileByPos(room, &newPos);
	// Would hit an obstacle on next tile, stop movement.
	if (newTile->obstacle || (newTile->type == DOOR && !room->complete)) {
		StandStill(entity);
	} else {
		entity->position = newPos;
	}
}

Vector2 Raycast(Room* room, Vector2 start, Vector2 end, GameEntity* self) {
	int x0 = (int)floorf(start.x), y0 = (int)floorf(start.y), x1 = (int)ceilf(end.x), y1 = (int)ceilf(end.y);
	int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	int err = dx + dy, e2;
	int max = (abs(err) * 2) + 1;
	Vector2 point = {};
	GameEntity* coll = NULL;
	int tilesThrough = Vector2Distance(start, end) / TILE_SIZE;
	Vector2 tilePos = { floorf(x0 / TILE_SIZE), floorf(y0 / TILE_SIZE) };

	for (int i = 0; i < max; i++) {
		point.x = x0;
		point.y = y0;

		// Check if tile has changed.
		if (tilesThrough > 1) {
			float newTilePosX = floorf(x0 / TILE_SIZE);
			float newTilePosY = floorf(y0 / TILE_SIZE);
			if (newTilePosX != tilePos.x || newTilePosY != tilePos.y) {
				// New tile, check if it's a obstacle.
				tilePos.x = newTilePosX;
				tilePos.y = newTilePosY;
				Tile* newTile = GetTileByPos(room, &tilePos);
				if (newTile->obstacle || (newTile->type == DOOR && !room->complete)) {
					break;
				}
			}
		}
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
	Rectangle entityWorldHitbox;
	for (int j = 0; j < room->entityCount; j++) {
		if (!room->entities[j].active) {
			// Ignore inactive entities.
			continue;
		}
		if (self != NULL && self == &room->entities[j].entity) {
			// Ignore self.
			continue;
		}

		// Check collision with entity.
		entityWorldHitbox = HitboxWorldPosition(&room->entities[j].entity);
		if (CheckCollisionRecs(entityWorldHitbox, *newPos)) {
			return &room->entities[j].entity;
		}
	}

	// TODO: Same as the other todo with similar code
	if (room->boss.active) {
		entityWorldHitbox = HitboxWorldPosition(&room->boss.entity);
		if (CheckCollisionRecs(entityWorldHitbox, *newPos)) {
			return &room->boss.entity;
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

static void UpdateEnemy(GameContext* context, Player* player, Level* level, ActiveEnemy* enemy, float dt) {
	// Check for death.
	if (enemy->entity.health <= 0) {
		enemy->active = false;
		// TODO: Death animation :)
		return;
	}

	// Update the enemy entity to current game logic status.
	UpdateEntity(&enemy->entity, dt);

	// It is preferable to check for stun here once rather than on every single entity action.
	if (!enemy->entity.stunned) {
		int unwindState = EntityUnwindAttack(
			&enemy->entity, enemy->attack,
			&player->entity.position,
			&level->attacks,
			enemy->behaviour == BOMB ? T_ALL : T_PLAYER
		);
		if (unwindState > 0) {
			// Enemy just finished winding an attack.
			if (unwindState == 2) {
				if (enemy->behaviour == BOMB) {
					// Bombs are deactivated after exploding.
					enemy->active = false;
					return;
				} else {
					return StandStill(&enemy->entity);
				}
			}
			// Enemy is winding up.
			return;
		}

		// Check if player is within the entity's active area.
		if (!CheckCollisionPointCircle(
			player->entity.position, enemy->entity.position, enemy->activeRadius
		)) {
			// Inactive status.
			// If can be seen in screen or close by, idle behaviour.
			// Otherwise, completely ignore.
			// TODO: Idle.
			return StandStill(&enemy->entity);
		}

		// Check if entity status allows for attack.
		if (EnemyCheckAttack(enemy, level->playTime, &player->entity.position)) {
			// Attack execution started.
			return;
		}

		float vecDist = Vector2Distance(player->entity.position, enemy->entity.position);
		Direction dir = NO_DIRECTION;

		// Enemy that wants to distance itself from the player.
		if (enemy->behaviour == DISTANCE) {
			// When the enemy is a tad too far away within its active area, it actually approaches the player.
			if (vecDist > enemy->activeRadius * 0.8f) {
				float angle = Vector2LineAngle(enemy->entity.position, player->entity.position);
				dir = AngleToDirection(angle, false);
			} else if (vecDist > enemy->activeRadius * 0.7f) {
				// Enemy will stand still if within adequate distance from the player.
				return StandStill(&enemy->entity);
			} else {
				// Too close to player, get away from it.
				float angle = Vector2LineAngle(player->entity.position, enemy->entity.position);
				// TODO: If close to an obstacle in the direction, try to go around the player.
				dir = AngleToDirection(angle, false);
				// Check if there is an obstacle in the path of running away.
				if (
					vecDist < enemy->activeRadius * 0.5f
					&& TestPointDirCollision(
						level->currentRoom, &enemy->entity, enemy->entity.position.x, enemy->entity.position.y, dir
					)
				) {
					dir = OppositeDir(dir);
				}
			}
		}

		// Enemy is always approaching player.
		if (enemy->behaviour == APPROACH) {
			// Set direction towards player.
			// Min distance is entity hitbox in front of player hitbox.
			// Get the closest player hitbox corner to the enemy position.
			Vector2 closestCorner = ClosestRectCorner(HitboxWorldPosition(&player->entity), enemy->entity.position);
			dir = GetPointDirThreshold(
				enemy->entity.position,
				closestCorner,
				enemy->entity.hitbox.width,
				enemy->entity.hitbox.height
			);
		}

		// Enemy is close enough to target position, stand still.
		if (dir == NO_DIRECTION) {
			return StandStill(&enemy->entity);
		}

		// Here the enemy has picked a direction to walk towards.
		// Check if future movement will collide with something.
		// If far away, we check with next hitbox.
		// If getting close 2 tiles, we raycast a tile.
		// We draw a line from both advancing front corners to see if any edge would hit a box.
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
			newHitbox.x += anglev.x * enemy->entity.speed * dt;
			newHitbox.y += anglev.y * enemy->entity.speed * dt;
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
			return StandStill(&enemy->entity);
		}
		SetStance(&enemy->entity, WALKING);
		enemy->entity.dir = dir;
		enemy->entity.speed = enemy->speed;
		enemy->entity.anglev = DirectionToVector(enemy->entity.dir);
	}

	// Update entity position according to its movement.
	// Collision checks to be done before this.
	float speed = GetEntitySpeed(&enemy->entity);
	if (speed > 0.0f) {
		MoveEntityByForce(level->currentRoom, &enemy->entity, dt);
	}
}

static void DoesAttackHit(AttackCbArgs* cbArgs, GameEntity* entity, ActiveAttack* attack) {
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
	// TODO: Instantiate blood splash on ground from an event sub.
	int damage = AttackHitEntity(entity, attack);
	EmitDmgEvent(entity, damage, attack->attack->dmgType);
	if (attack->fromPlayer) {
		// Emit event if player is the one hitting.
		// This allows us to manufacture many on hit events or combos without having to know about them here.
		EmitPlayerHitEvent(entity);
	}
}

static void AttackCallback(ObjectPool* pool, int index, void* args) {
	// Ignore CB with invalid args, but does not mean item itself is invalid.
	if (args == NULL) {
		return;
	}
	ActiveAttack* attack = PoolIndexAddress(pool, index);
	if (attack == NULL || attack->attack == NULL) {
		LogDebug("Null pointer, invalid attack state on index %d", index);
		// This means some pointer is pointing at invalid data.
		goto cleanup;
	}
	// This attack has finished.
	if (attack->elapsed >= attack->attack->duration || attack->completed) {
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
		DoesAttackHit(cbArgs, &cbArgs->player->entity, attack);
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
			DoesAttackHit(cbArgs, &room->entities[i].entity, attack);
		}
	}
	if (room->boss.active && !room->boss.entity.invuln.active) {
		DoesAttackHit(cbArgs, &room->boss.entity, attack);
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
	room->visited = true;
	level->swappingRoom = true;
	level->playTime = 0.0f;
}

static void SpawnBomb(Level* level, Vector2 pos) {
	// TODO: Realloc?
	if (level->currentRoom->entities == NULL) {
		LogDebug("Invalid entities pointer on room");
		return;
	}
	int bombId = level->currentRoom->entityCount++;
	level->currentRoom->entities[bombId] = InstantiateEnemy(
		GetEnemy(PBOMB),
		pos
	);
	level->currentRoom->entities[bombId].entity.invuln.active = true;
	level->currentRoom->entities[bombId].entity.invuln.duration = 1000.0f;
	level->currentRoom->entities[bombId].entity.unstoppable = true;
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
			tile->warp.touched = true;
			return TriggerRoomChange(level, tile->warp.dest);
		}
	}

	// Update physics and status counters.
	// Each entity has their own because they could be individually frozen.
	UpdateEntity(&player->entity, delta);

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

	// Check for collision with active items.
	if (level->itemCount > 0 && level->items != NULL) {
		for (int i = 0; i < level->itemCount; i++) {
			if (!level->items[i].active) {
				continue;
			}
			// TODO: Put this on the item generation itself,,,
			const Rectangle itemBox = {
				level->items[i].pos.x,
				level->items[i].pos.y,
				32,
				32
			};
			if (CheckCollisionRecs(itemBox, HitboxWorldPosition(&player->entity))) {
				PlayerCollideItem(player, &level->items[i]);
			}
		}
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
	if (!player->entity.stunned && player->entity.stance != ATTACKING) {
		// Plant bomb, first action.
		if (IsActionOnce(ACTION_BOMB) && player->bombs > 0) {
			player->bombs--;
			SpawnBomb(level, player->entity.position);
		}

		// Execute dash.
		if (IsActionActive(ACTION_DASH) && player->dash.cdLeft == 0.0f) {
			return PlayerStartDash(context, player);
		}

		// Attack action.
		if (IsActionActive(ACTION_ATT)) {
			PlayerAttackAction(context, player, &level->attacks);
		}
	}

	// Execute movement. Last action so other actions that may require directionality take precedence.
	if (newDir != NO_DIRECTION) {
		MoveEntityByForce(level->currentRoom, &player->entity, delta);
	}
}

static Vector2 FindRewardPosition(Room* room) {
	// TODO: Find out if tile is obstacle or inacesible and then find out nearest proper tile.
	// Could also just pick a tile adjacent to player that's free and towards the center.
	return (Vector2){ room->columns / 2.0f, room->rows / 2.0f };
}

static void AddItem(Level* level, Item item) {
	int itemId = level->itemCount++;
	if (level->items == NULL) {
		level->items = malloc(sizeof(Item) * level->itemCount);
	} else {
		level->items = realloc(level->items, sizeof(Item) * level->itemCount);
	}
	level->items[itemId] = item;
}

static void UpdateRoomBoss(Level* level, ActiveBoss* boss, float dt) {
	// Update boss state.
	UpdateBoss(boss, dt);
	// Apply boss specific behaviour.
	boss->boss->behaviour(boss, &player);
	// Boss behaviour does not implement movement as it is agnostic to the room.
	// We apply here movement and stop it if necessary.
	// Boss behaviour will find out if it's been stopped.
	MoveEntityByForce(level->currentRoom, &boss->entity, dt);
	if (boss->entity.health <= 0.0f) {
		boss->active = false;
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

	if (IsActionOnce(ACTION_TAB)) {
		context->options->fullMap = !context->options->fullMap;
	}

	// TODO: Add death/game over check here.
	UpdatePlayer(context, &level, &player, dt);

	// Run ongoing attacks.
	// Attacks are instantiated from a template and ran on their own afterwards.
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
	if (level.currentRoom != NULL) {
		// Run boss behaviour update on boss rooms.
		if (level.currentRoom->type == BOSS && level.currentRoom->boss.active) {
			UpdateRoomBoss(&level, &level.currentRoom->boss, dt);
			activeEntities++;
		}

		// Boss rooms may also have regular entities.
		if (level.currentRoom->entityCount > 0) {
			for (int i = 0; i < level.currentRoom->entityCount; i++) {
				if (level.currentRoom->entities[i].active) {
					UpdateEnemy(context, &player, &level, &level.currentRoom->entities[i], dt);
					activeEntities++;
				}
			}
		}
	}

	// Check if room has been completed to open doors.
	if (level.currentRoom != NULL && !level.currentRoom->complete && activeEntities == 0) {
		level.currentRoom->complete = true;
		if (level.currentRoom->reward != I_NONE) {
			LogDebug("Room has reward, instantiating it.");
			Vector2 rpos = FindRewardPosition(level.currentRoom);
			// TODO: Right now world offset thinks all rooms must be same size.
			// Precalculate world position on room creation one by one.
			Vector2 worldPos = RoomOffsetPos(level.currentRoom, rpos.x, rpos.y);
			int amount = level.currentRoom->reward == I_EXP ? 5 : 1;
			ItemType itype;
			AddItem(&level, (Item){
				.pos = worldPos,
				.type = level.currentRoom->reward,
				.amount = amount,
				.active = true,
			});
		}
	}
}
