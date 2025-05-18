/******************************
 * Player character functions *
 ******************************/

#ifndef CHARACTER_H
#define CHARACTER_H

#include <raylib.h>
#include "game.h"
#include "item.h"
#include "object-pool.h"
#include "event.h"

#define PLAYER_SPEED 200.0f
#define PLAYER_INIT_WEAPONS 2
#define DASH_SPEED_MULT 2.5f
#define DASH_DURATION 0.25f
#define DASH_LENGTH 150.0f

typedef struct {
	bool dashing;
	float elapsed;
	Vector2 direction;
	int max;
	int consecutive;
	// Total cooldown after a dash sequence
	float cooldown;
	// Time to wait until player can dash again.
	float cdLeft;
} Dash;

typedef struct {
	GameEntity entity;
	float speed;
	Dash dash;
	Gear gear;
	int relicCount;
	Relic** relics;
	float strength;
	// Keys to open doors and locked chests.
	int keys;
	// Exposure is the game currency.
	int exp;
	// Regular bombs to blow up stuff.
	int bombs;
	// TODO: Spell cards as bomb alternative
	float bombElapsed;
} Player;

Player CreatePlayer(Texture2D* characterTexture);
int EquipWeapon(Player* player, Weapon* weapon);
int SwapWeapon(Player* player);
bool CanPlayerBeHit(Player* player);
void PlayerDashUpdate(Player* player, float dt);
void PlayerStartDash(GameContext* context, Player* player);
Direction PlayerUpdateDirection(Player* player);
void UpdateWeaponStatus(Player* player, float delta);
void PlayerAttackAction(GameContext* context, Player* player, ObjectPool* attPool);
void AddRelic(Player* player, Relic* relic);
Observable* GetPlayerEvents();
void SetupPlayerEvents();
void onPlayerHit(Event* ev);
void EmitPlayerHitEvent(GameEntity* target);
void PlayerCollideItem(Player* player, Item* item);

#endif
