#ifndef LEVEL_H
#define LEVEL_H

#include <raylib.h>
#include "lib.h"
#include "game.h"
#include "object-pool.h"
#include "character.h"
#include "attack.h"

#define DEFAULT_ENEMY_RADIUS 200.0f
#define ENEMY_DEFAULT_SPEED 150.0f
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
} Tile;

typedef enum { APPROACH, STAND, DISTANCE } EnemyBehaviour;

typedef struct {
	GameEntity entity;
	float activeRadius;
	EnemyBehaviour behaviour;
	bool active;
	// This is the base speed of the enemy.
	// Can be modified. Entity holds final speed.
	float speed;
	float lastAttack;
	float attackCd;
	Attack* attack;
} Enemy;

typedef struct {
	char* content;
	Vector2 start;
	Vector2 end;
	float startTime;
	float endTime;
	int fontSize;
	Color color;
} ActiveText;

typedef struct Room {
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
	Enemy* entities;
	// TODO: Store world corner positions? Would avoid some calcs
} Room;

typedef struct {
	int floor;
	Room* currentRoom;
	Room* nextRoom;
	bool swappingRoom;
	int totalRooms;
	float playTime;
	ObjectPool attacks;
	ObjectPool texts;
	// Room 0 is the center of the floor.
	Room* rooms;
} Level;

typedef struct {
	float dt;
	Player* player;
	Level* level;
	ObjectPool* textPool;
} AttackCbArgs;

void SetupLevel(GameContext* context);
void DestroyLevel();
float MaxAttackRange(Enemy* enemy);
Rectangle HitboxWorldPosition(GameEntity* entity);
void UpdateLevel(GameContext* context, float dt);
Player* GetPlayer();
Level* GetLevel();
Vector2 RoomOffset(Room* room);
Vector2 RoomOffsetPos(Room* room, int x, int y);

#endif
