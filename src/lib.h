#ifndef LIB_H
#define LIB_H

#include <stdio.h>
#include <raylib.h>

#define LOG_DEBUG
#define NULL ((void *)0)
#define MAX_LEVEL 10
#define LOG_YELL "\e[0;33m"
#define LOG_PURP "\e[0;35m"
#define LOG_ENDC "\e[0m"

#ifdef LOG_DEBUG
#define LogDebug(fmt, ...) printf((LOG_YELL "[DEBUG]" LOG_ENDC LOG_PURP "[%s:%d]" LOG_ENDC " %s: " fmt "\n"), __FILE__, __LINE__, __PRETTY_FUNCTION__, ##__VA_ARGS__);
#else
#define LogDebug(...)
#endif

typedef struct {
	Texture2D* texture;
	Rectangle rect;
	Vector2 position;
} Sprite;

bool IsPointInRectangle(Vector2 point, Rectangle rect);
bool DoesRectCollideCircle(Rectangle rect, Vector2 circleCenter, float radius);
bool IsPointInCircle(Vector2 point, Vector2 circleCenter, float radius);

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
