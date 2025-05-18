#ifndef LEVEL_H
#define LEVEL_H

#include <raylib.h>
#include "item.h"
#include "lib.h"
#include "game.h"
#include "object-pool.h"
#include "player.h"
#include "entity.h"
#include "event.h"
#include "boss.h"

#define COLL_RAYCAST_ACTIVE 64.0f
#define COLL_RAYCAST_DIST 16.0f

typedef enum { WALL = 0, GROUND = 1, GRASS, DOOR } TileType;

typedef struct {
	TileType type;
	bool obstacle;
	int damage;
	float speed;
	struct {
		struct Room* dest;
		Vector2 pos;
	} warp;
	float rotation;
} Tile;

typedef struct {
	char* content;
	Vector2 start;
	Vector2 end;
	float startTime;
	float endTime;
	int fontSize;
	Color color;
} ActiveText;

typedef enum { HALL, ENCOUNTER, SHOP, BOSS } RoomType;

typedef struct Room {
	// Room number in the player pathing.
	int roomNo;
	RoomType type;
	// Room position in regards to the central room.
	Vector2 pos;
	bool complete;
	// Total tiles for this room.
	int tileCount;
	// Rooms can have different amount of columns and rows.
	// We have all metadata here because it makes more sense to store an int rather than to calculate it over and over.
	int rows;
	int columns;
	Tile* tiles;
	// Each room has its own entities.
	int entityCount;
	ActiveEnemy* entities;
	// TODO: Store world corner positions? Would avoid some calcs
	ItemType reward;
	int rewardAmount;
	bool visited;
	ActiveBoss boss;
} Room;

typedef struct {
	int floor;
	// Pointers for room swap upon using a door or warp.
	Room* currentRoom;
	Room* nextRoom;
	bool swappingRoom;
	int totalRooms;
	float playTime;
	ObjectPool attacks;
	ObjectPool texts;
	// Room 0 is the center of the floor.
	Room* rooms;
	// Precalculate limits for minimap display
	int minX;
	int maxX;
	int minY;
	int maxY;
	// Items on the floor.
	int itemCount;
	Item* items;
} Level;

typedef struct {
	float dt;
	Player* player;
	Level* level;
	ObjectPool* textPool;
} AttackCbArgs;

void SetupLevel(GameContext* context);
void DestroyLevel();
void DestroyPlayer();
void UpdateLevel(GameContext* context, float dt);
Player* GetPlayer();
Level* GetLevel();
Vector2 RoomOffset(Room* room);
Vector2 RoomOffsetPos(Room* room, int x, int y);

#endif
