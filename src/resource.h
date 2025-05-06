/****************************
 * Game resource management *
 ****************************/

#ifndef RESOURCE_H
#define RESOURCE_H

#include <raylib.h>

typedef enum { CURSOR_TEXTURE = 0, PLAYER_TEXTURE = 1 } GameTexture;

Texture2D* GetTexture(GameTexture text);
void UnloadTextures();

#endif
