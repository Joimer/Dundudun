#ifndef CHARACTER_H
#define CHARACTER_H

#include <raylib.h>
#include "game.h"
#include "item.h"

typedef struct {
	bool dashing;
	float elapsed;
	Vector2 direction;
	int max;
	int consecutive;
	// Total cooldown after a dash sequence
	float cooldown;
	// Time to wait until player can dash again.
	float cdLeft;
} Dash;

typedef struct {
	GameEntity entity;
	float speed;
	Dash dash;
	Gear gear;
} Player;


Player CreatePlayer(Texture2D* characterTexture);
void UpdatePlayer(GameContext* context, Player* player, float delta);
int EquipWeapon(Player* player, Weapon* weapon);
int SwapWeapon(Player* player);

#endif
