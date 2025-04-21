#include "item.h"

static Weapon weapons[2] = {
	{
		.damage = 7,
		.type = MELEE,
		.cooldown = 0.25f,
		.hitbox = { .rect = { 32.0f, 32.0f } },
		.centerDist = 16.0f
	},
	{
		.damage = 5,
		.type = SHOOTING,
		.cooldown = 0.2f,
		.hitbox = { .radius = 0.1f },
		.centerDist = 0.0f
	}
};
