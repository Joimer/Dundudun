/***********************************
 * Game items to be used by player *
 ***********************************/

#ifndef ITEM_H
#define ITEM_H

#include <raylib.h>
#include "game.h"
#include "attack.h"

#define TOTAL_WEAPONS 100
#define TOTAL_RELICS 4

typedef enum { MELEE, SHOOTING } WeaponType;

typedef struct {
	Vector2 pos;
	Vector2 direction;
	float force;
} Bullet;

typedef enum {
	LETTER_OPENER, CLIP_BOX,
} WeaponName;

// A Weapon holds a type of attack.
// Attacks have a windup and duration, but cooldown for usage is dependent on weapon.
// TODO FIXME uuuh maybe attack.windup, duration, should be on the weapon and attack just be a collection of hitboxes with damage and associated sprite?
// Later on I must add attack strings for melee weapons, therefor a single weapon should hold several attacks, but the cadence, cd, etc. is all on the weapon
typedef struct {
	WeaponName id;
	Attack* attack;
	WeaponType type;
	float cooldown;
	float elapsed;
	bool attacking;
	int cost;
} Weapon;

typedef struct {
	float speed;
	int dashes;
	int cost;
} Boots;

typedef struct {
	int weaponSlot;
	int maxWeaps;
	int equippedWeaps;
	Weapon** weapons;
	Boots* boots;
} Gear;

typedef void RelicHitEvent(GameEntity*);

typedef enum {
	LEFTOVER_LUNCH, MACHINE_COFFEE, HR_HEART, SHOCKING_PIC,
} RelicName;

typedef struct {
	RelicName id;
	float damage;
	int dashes;
	float speed;
	RelicHitEvent* onHit;
	float cooldown;
	int cost;
	// Values for dropping the relic.
	int weight;
	int minLevel;
} Relic;

typedef enum { KEFIR_DRINK } ConsumableName;

typedef struct {
	int heal;
	bool statusHeal[4];
	int cost;
} Consumable;

typedef enum { I_NONE, I_KEY, I_EXP, I_BOMB, I_RELIC, I_CONSUMABLE, I_GEAR } ItemType;

// Representation of an item in the game world.
typedef struct {
	Vector2 pos;
	ItemType type;
	int amount;
	bool active;
	int cost;
} Item;

Weapon* GetWeapon(int i);
Relic* GetRelic(int i);
bool HasRelic(RelicName id, Relic** relics, int count);
Relic* GetRelicDrop(GameContext* context, Relic** playerRelics, int relicCount, int level);

#endif
