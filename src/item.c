#include "game.h"
#include "lib.h"
#include "item.h"
#include "attack.h"
#include "entity.h"

static Weapon weapons[TOTAL_WEAPONS] = {
	// Base melee weapon: Letter Opener
	{
		.attack = &attacks[2],
		.type = MELEE,
		.cooldown = 0.35f,
		.attacking = false,
		.elapsed = 0.0f
	},
	// Base shooting weapon: Clip box
	{
		.attack = &attacks[3],
		.type = SHOOTING,
		.cooldown = 0.4f,
		.attacking = false,
		.elapsed = 0.0f
	}
};

void AddPoisonOnHit(GameEntity* entity) {
	ApplyStatus(entity, POISON, 2.0f);
}

void AddBurnOnHit(GameEntity* entity) {
	ApplyStatus(entity, BURN, 1.0f);
}

void AddFreezeOnHit(GameEntity* entity) {
	ApplyStatus(entity, FROZEN, 1.0f);
}

void AddParalyseOnHit(GameEntity* entity) {
	ApplyStatus(entity, PARALYSED, 1.0f);
}

static Relic relics[TOTAL_RELICS] = {
	[LEFTOVER_LUNCH] = {
		.id = LEFTOVER_LUNCH,
		.damage = 0,
		.dashes = 0,
		.speed = 0,
		.onHit = &AddPoisonOnHit,
		.cooldown = 0,
	},
	[MACHINE_COFFEE] = {
		.id = MACHINE_COFFEE,
		.damage = 0,
		.dashes = 0,
		.speed = 0,
		.onHit = &AddBurnOnHit,
		.cooldown = 0,
	},
	[HR_HEART] = {
		.id = HR_HEART,
		.damage = 0,
		.dashes = 0,
		.speed = 0,
		.onHit = &AddFreezeOnHit,
		.cooldown = 2.5f,
	},
	[SHOCKING_PIC] = {
		.id = SHOCKING_PIC,
		.damage = 0,
		.dashes = 0,
		.speed = 0,
		.onHit = &AddParalyseOnHit,
		.cooldown = 2.5f,
	},
};

Weapon* GetWeapon(int i) {
	if (i > TOTAL_WEAPONS - 1) {
		LogDebug("Attempting to get invalid weapon %d", i);
		return NULL;
	}

	return &weapons[i];
}

Relic* GetRelic(int i) {
	if (i > TOTAL_RELICS - 1) {
		LogDebug("Attempting to get invalid relic %d", i);
		return NULL;
	}

	return &relics[i];
}
