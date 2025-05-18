/****************************
 * Game resource management *
 ****************************/

#ifndef RESOURCE_H
#define RESOURCE_H

#include <raylib.h>

typedef enum {
	CURSOR_TEXTURE, PLAYER_TEXTURE, DOOR_TEXTURE, DOOR_OPEN_TEXTURE
} GameTexture;

Texture2D* GetTexture(GameTexture text);
void UnloadTextures();
void LoadAllTextures();

#endif
