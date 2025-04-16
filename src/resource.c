#include <raylib.h>
#include "resource.h"

#define TEXTURE_COUNT 2

Texture2D gameTextures[TEXTURE_COUNT] = {};
const char* textToSource[] = {
	"src/resources/cursor.png",
	"src/resources/testpj.png"
};

// TODO: Use a texture atlas.
Texture2D* GetTexture(GameTexture text) {
	if (gameTextures[text].id == 0) {
		gameTextures[text] = LoadTexture(textToSource[text]);
		SetTextureFilter(gameTextures[text], TEXTURE_FILTER_POINT);
	}

	return &gameTextures[text];
}

void UnloadTextures() {
	for (int i = 0; i < TEXTURE_COUNT; i++) {
		if (gameTextures[i].id != 0) {
			UnloadTexture(gameTextures[i]);
		}
	}
}
