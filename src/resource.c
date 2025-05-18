#include <raylib.h>
#include "resource.h"

#define TEXTURE_COUNT 4

static Texture2D gameTextures[TEXTURE_COUNT] = {};

const char* textToSource[TEXTURE_COUNT] = {
	[CURSOR_TEXTURE] = "src/resources/cursor.png",
	[PLAYER_TEXTURE] = "src/resources/testpj.png",
	[DOOR_TEXTURE] = "src/resources/doapi.png",
	[DOOR_OPEN_TEXTURE] = "src/resources/opendoor.png",
};

static void LoadGameTexture(GameTexture text) {
	if (gameTextures[text].id == 0) {
		gameTextures[text] = LoadTexture(textToSource[text]);
		SetTextureFilter(gameTextures[text], TEXTURE_FILTER_POINT);
	}
}

// TODO: Use a texture atlas.
Texture2D* GetTexture(GameTexture text) {
	LoadGameTexture(text);

	return &gameTextures[text];
}

void UnloadTextures() {
	for (int i = 0; i < TEXTURE_COUNT; i++) {
		if (gameTextures[i].id != 0) {
			UnloadTexture(gameTextures[i]);
		}
	}
}

void LoadAllTextures() {
	for (int i = 0; i < TEXTURE_COUNT; i++) {
		LoadGameTexture(i);
	}
}
