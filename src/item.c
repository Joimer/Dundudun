#include "lib.h"
#include "item.h"
#include "attack.h"

static Weapon weapons[TOTAL_WEAPONS] = {
	// Base melee weapon: Letter Opener
	{
		.attack = &attacks[0],
		.type = MELEE,
		.cooldown = 0.35f,
	},
	// Base shooting weapon: Clip box
	{
		.attack = &attacks[1],
		.type = SHOOTING,
		.cooldown = 0.25f,
	}
};

Weapon* GetWeapon(int i) {
	if (i > TOTAL_WEAPONS - 1) {
		LogDebug("Attempting to get invalid weapon %d", i);
		return NULL;
	}

	return &weapons[i];
}
