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

int clamp(int num, int min, int max);
float clampf(float num, float min, float max);

#endif
