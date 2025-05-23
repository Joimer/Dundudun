/********************************************
* Level boss behaviours, attacks and so on *
********************************************/

#ifndef BOSS_H
#define BOSS_H

#include "game.h"
#include "player.h"

#define TOTAL_BOSSES 10

struct ActiveBoss;

typedef int BossRunPhase(struct ActiveBoss* boss, Player* player);

typedef struct {
	int level;
	int phases;
	int maxhp;
	float baseSpeed;
	BossRunPhase* behaviour;
	float weight;
} Boss;

typedef struct ActiveBoss {
	const Boss* boss;
	int hp;
	// Generic counter for each boss to keep track of their own behaviour steps.
	int stateCount;
	float elapsed;
	GameEntity entity;
	bool active;
} ActiveBoss;

const Boss* GetBoss(GameContext* context, int level);
void UpdateBoss(ActiveBoss* boss, float dt);
ActiveBoss InstantiateBoss(const Boss* boss, Vector2 pos);

#endif
