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
	ApplyStatus(entity, POISON, 5.0f);
}

static Relic relics[TOTAL_RELICS] = {
	{
		.damage = 0,
		.dashes = 0,
		.speed = 0,
		.onHit = &AddPoisonOnHit
	}
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
