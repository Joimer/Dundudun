#include <stdlib.h>
#include <raylib.h>
#include <string.h>
#include "level.h"
#include "lib.h"
#include "game.h"

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
		int pos = 64 * i + 64;
		level.entities[i] = (Enemy){
			.activeRadius = DEFAULT_ENEMY_RADIUS,
			.behaviour = APPROACH,
			.entity = (GameEntity){
				.sprite = (Sprite){
					.rect = (Rectangle){ 0, 0, 32, 32 },
					.position = (Vector2){ -16, -16 },
				},
				.position = (Vector2){ pos, pos },
				.health = 30,
				.invuln = {},
				.hitbox = { 0, 0, 16, 16 }
			}
		};
	}

	return level;
}
