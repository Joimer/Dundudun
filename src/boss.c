#include "boss.h"
#include "entity.h"
#include "lib.h"
#include "player.h"

void UpdateBoss(ActiveBoss* boss, float dt) {
	if (boss->active) {
		UpdateEntity(&boss->entity, dt);
		boss->elapsed += dt;
	}
}

void Forklift(ActiveBoss* boss, Player* player) {
	// Boss is not doing an action and ready to stammrt.
	if (boss->stateCount == 0 && boss->elapsed >= 0.15f) {
		boss->stateCount = 1;
		boss->elapsed = 0;
		// Attack: Fix objective towards player and go there in a rushing movement until hitting the wall.
		float angle = Vector2LineAngle(boss->entity.position, player->entity.position);
		//Direction dir = AngleToDirection(angle, false);
		boss->entity.anglev = AngleToVector(angle);
		boss->entity.dir = AngleToDirection(angle, false);
		boss->entity.speed = boss->boss->baseSpeed;
		boss->entity.unstoppable = true;
		return;
	}

	// Executing movement attack.
	if (boss->stateCount == 1) {
		// Was stopped upon hitting an obstacle.
		if (boss->entity.speed == 0.0f) {
			boss->entity.unstoppable = false;
			// Change to charging next attack.
			boss->stateCount = 2;
			boss->elapsed = 0;
		}
	}

	// Next attack windup. When time's passed resets to starting attack.
	if (boss->stateCount == 2 && boss->elapsed >= 0.35f) {
		boss->stateCount = 0;
		boss->elapsed = 0;
	}
}

const Boss bosses[TOTAL_BOSSES] = {
	{
		.level = 1,
		.phases = 1,
		.maxhp = 100,
		.baseSpeed = 400,
		.behaviour = &Forklift
	},
};

const Boss* GetBoss(GameContext* context, int level) {
	for (int i = 0; i < TOTAL_BOSSES; i++) {
		if (bosses[i].level == level) {
			// TODO: Random between bosses of same level.
			return &bosses[i];
		}
	}

	return NULL;
}

ActiveBoss InstantiateBoss(const Boss* boss, Vector2 pos) {
	return (ActiveBoss){
		.boss = boss,
		.active = true,
		.hp = boss->maxhp,
		.stateCount = 0,
		.elapsed = 0.0f,
		.entity = CreateEntity(
			boss->maxhp,
			pos,
			(Sprite){
				.rect = (Rectangle){ 0, 0, 48, 48 },
				.position = (Vector2){ -24, -24 },
				.visible = true,
				.layer = 4
			},
			(Rectangle){ -12, -12, 24, 24 },
			0.5f
		)
	};
}
