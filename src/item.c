#include "game.h"
#include "lib.h"
#include "item.h"
#include "attack.h"
#include "entity.h"

static Weapon weapons[TOTAL_WEAPONS] = {
	// Base melee weapon: Letter Opener
	[LETTER_OPENER] = {
		.id = LETTER_OPENER,
		.attack = &attacks[2],
		.type = MELEE,
		.cooldown = 0.35f,
		.attacking = false,
		.elapsed = 0.0f
	},
	// Base shooting weapon: Clip box
	[CLIP_BOX] = {
		.id = CLIP_BOX,
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
	ApplyStatus(entity, BURN, 5.0f);
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
		.cost = 25,
		.weight = 25,
		.minLevel = 1,
	},
	[MACHINE_COFFEE] = {
		.id = MACHINE_COFFEE,
		.damage = 0,
		.dashes = 0,
		.speed = 0,
		.onHit = &AddBurnOnHit,
		.cooldown = 0,
		.cost = 25,
		.weight = 25,
		.minLevel = 1,
	},
	[HR_HEART] = {
		.id = HR_HEART,
		.damage = 0,
		.dashes = 0,
		.speed = 0,
		.onHit = &AddFreezeOnHit,
		.cooldown = 2.5f,
		.cost = 25,
		.weight = 25,
		.minLevel = 1,
	},
	[SHOCKING_PIC] = {
		.id = SHOCKING_PIC,
		.damage = 0,
		.dashes = 0,
		.speed = 0,
		.onHit = &AddParalyseOnHit,
		.cooldown = 2.5f,
		.cost = 25,
		.weight = 25,
		.minLevel = 1,
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

bool HasRelic(RelicName id, Relic** relics, int count) {
	if (count > 0) {
		for (int j = 0; j < count; j++) {
			if (id == relics[j]->id) {
				return true;
			}
		}
	}
	return false;
}

// Decides a relic drop from a drop table generated on the fly.
Relic* GetRelicDrop(GameContext* context, Relic** playerRelics, int relicCount, int level) {
	int totalChance = 0;
	int relicWeights[TOTAL_RELICS] = { 0 };
	for (int i = 0; i < TOTAL_RELICS; i++) {
		if (relics[i].minLevel > level) {
			continue;
		}
		if (relicCount > 0) {
			if (HasRelic(relics[i].id, playerRelics, relicCount)) {
				continue;
			}
		}
		totalChance += relics[i].weight;
		relicWeights[i] = relics[i].weight;
	}
	// Somehow all available relics are unavailable...
	if (totalChance == 0) {
		return NULL;
	}
	int dice = GetRandomMTValue(&context->state->mtrand) % totalChance;
	int acc = 0;
	int relicId = -1;
	for (int i = 0; i < TOTAL_RELICS; i++) {
		if (relicWeights[i] == 0) {
			continue;
		}
		acc += relicWeights[i];
		if (acc >= dice) {
			relicId = i;
			break;
		}
	}
	if (relicId == -1) {
		return NULL;
	}

	return &relics[relicId];
}
