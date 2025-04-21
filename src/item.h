#ifndef ITEM_H
#define ITEM_H

#include <raylib.h>
#include "game.h"

typedef enum { MELEE, SHOOTING } WeaponType;

typedef struct {
	Vector2 pos;
	Vector2 direction;
	float force;
} Bullet;

typedef struct {
	int damage;
	WeaponType type;
	float cooldown;
	Hitbox hitbox;
	float centerDist;
} Weapon;

typedef struct {
	float speed;
	int dashes;
} Boots;

typedef struct {
	int weaponSlot;
	int maxWeaps;
	Weapon** weapons;
	Boots* boots;
} Gear;

#endif
