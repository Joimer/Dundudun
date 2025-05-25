/*********************
 * Utility functions *
 *********************/

#ifndef LIB_H
#define LIB_H

#include <stdio.h>
#include <raylib.h>

#define MAX_LEVEL 10
#define LOG_YELL "\e[0;33m"
#define LOG_PURP "\e[0;35m"
#define LOG_ENDC "\e[0m"

#define TILE_SIZE 32.0f

#define LOG_DEBUG
#ifdef LOG_DEBUG
#define LogDebug(fmt, ...) printf((LOG_YELL "[DEBUG]" LOG_ENDC LOG_PURP "[%s:%d]" LOG_ENDC " %s: " fmt "\n"), __FILE__, __LINE__, __PRETTY_FUNCTION__, ##__VA_ARGS__);
#else
#define LogDebug(...)
#endif

#define DEG_360 PI * 2.0f
#define DEG_45 PI / 4.0f
#define DEG_90 PI / 2.0f
#define DEG_135 3.0f * PI / 4.0f
#define DEG_225 5.0f * PI / 4.0f
#define DEG_270 3.0f * PI / 2.0f
#define DEG_315 7.0f * PI / 4.0f

typedef struct {
	Texture2D* texture;
	Rectangle rect;
	Vector2 position;
	// Cache of absolute position to avoid unnecessary calcs each frame.
	Vector2 worldPos;
	bool visible;
	int layer;
} Sprite;

// We can use a 4 bit value to indicate a direction.
// 0000 - north south west east
typedef enum {
	NO_DIRECTION = 0,
	EAST = 1,
	NORTH = 8,
	SOUTH = 4,
	WEST = 2,
	NORTHEAST = 9,
	NORTHWEST = 10,
	SOUTHEAST = 5,
	SOUTHWEST = 6
} Direction;

Vector2 ClosestRectCorner(Rectangle rect, Vector2 point);
bool IsBitSet(int val, int bit);
Direction GetPointDir(Vector2 origin, Vector2 target);
Direction GetPointDirThreshold(Vector2 origin, Vector2 target, float xThreshold, float yThreshold);
char* IntToString(int val);
Vector2 AngleToVector(float angle);
Vector2 DirectionToVector(Direction dir);
float DirectionToAngle(Direction dir);
Direction AngleToDirection(float angle, bool strict);
Vector2 AdvancePointByAngle(Vector2 start, float angle, float force);
Vector2 AdvancePointByDir(Vector2 start, Direction dir, float force);
Vector2 AdvancePointByVector(Vector2 start, Vector2 anglev, float force);
Direction OppositeDir(Direction dir);

/**
 * Improved initialisation Mersenne Twister for the secondary PRNG.
 * Adapted from the original work on https://www.math.sci.hiroshima-u.ac.jp/m-mat/MT/emt.html
 * Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura, All rights reserved.
 */
// Period parameters.
#define MTRAND_VECTOR_LENGTH 624
#define MTRAND_VECTOR_M 397
// Constant vector a.
#define MTRAND_MATRIX_A 0x9908b0dfUL
// Most significant w-r bits.
#define MTRAND_UPPER_MASK 0x80000000UL
// Least significant r bits.
#define MTRAND_LOWER_MASK 0x7fffffffUL

typedef struct {
	unsigned long mt[MTRAND_VECTOR_LENGTH];
	int index;
} MTRand;

MTRand SeedMTRand(unsigned long seed);
unsigned long GetRandomMTValue(MTRand* rand);

#endif
