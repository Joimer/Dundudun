#ifndef CHARACTER_H
#define CHARACTER_H

#include <raylib.h>
#include "game.h"

Player CreatePlayer(Texture2D* characterTexture);
void UpdatePlayer(GameContext* context, Player* player, float delta);

#endif
