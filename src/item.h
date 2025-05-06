/***********************************
 * Game items to be used by player *
 ***********************************/

#ifndef ITEM_H
#define ITEM_H

#include <raylib.h>
#include "game.h"
#include "attack.h"

#define TOTAL_WEAPONS 2
#define TOTAL_RELICS 1

typedef enum { MELEE, SHOOTING } WeaponType;

typedef struct {
	Vector2 pos;
	Vector2 direction;
	float force;
} Bullet;

// A Weapon holds a type of attack.
// Attacks have a windup and duration, but cooldown for usage is dependent on weapon.
// TODO FIXME uuuh maybe attack.windup, duration, should be on the weapon and attack just be a collection of hitboxes with damage and associated sprite?
// Later on I must add attack strings for melee weapons, therefor a single weapon should hold several attacks, but the cadence, cd, etc. is all on the weapon
typedef struct {
	Attack* attack;
	WeaponType type;
	float cooldown;
	float elapsed;
	bool attacking;
} Weapon;

typedef struct {
	float speed;
	int dashes;
} Boots;

typedef struct {
	int weaponSlot;
	int maxWeaps;
	int equippedWeaps;
	Weapon** weapons;
	Boots* boots;
} Gear;

typedef void RelicHitEvent(GameEntity*);

typedef struct {
	float damage;
	int dashes;
	float speed;
	RelicHitEvent* onHit;
} Relic;

Weapon* GetWeapon(int i);
Relic* GetRelic(int i);

#endif
