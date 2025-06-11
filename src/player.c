#include <raylib.h>
#include <raymath.h>
#include <math.h>
#include <stdlib.h>
#include "player.h"
#include "control.h"
#include "game.h"
#include "item.h"
#include "level.h"
#include "lib.h"
#include "object-pool.h"
#include "entity.h"

static Observable playerEvents;

Observable* GetPlayerEvents() {
	return &playerEvents;
}

void SetupPlayerEvents() {
	playerEvents = CreateEventEmitter(0);
}

Player CreatePlayer(Texture2D* characterTexture) {
	float halfWidth = (float) characterTexture->width / 2.0f;
	float halfHeight = (float) characterTexture->height / 2.0f;
	// TODO: realloc if limits are increased in-game? Always alloc to max possible weapons?
	Weapon** playerWeapons = calloc(PLAYER_INIT_WEAPONS, sizeof(Weapon*));
	Consumable** consumables = calloc(PLAYER_INIT_CONSUMABLES, sizeof(Consumable*));

	// Pre-assign initial weapons.
	// This will not be done here and initial room will have them so player can learn how to interact with items.
	playerWeapons[0] = GetWeapon(0);
	playerWeapons[1] = GetWeapon(1);

	return (Player){
		.speed = PLAYER_SPEED,
		.relicCount = 0,
		.relics = calloc(100, sizeof(Relic*)),
		.strength = 0,
		.keys = 0,
		.exp = 0,
		.bombs = 0,
		.bombElapsed = 0,
		.nextAction = NONE,
		.entity = CreateEntity(
			50,
			(Vector2){ 100.0f, 100.0f },
			(Sprite){
				.texture = characterTexture,
				.position = { halfWidth, halfHeight },
				.rect = { 0.0f, 0.0f, characterTexture->width, characterTexture->height },
				.visible = true,
				.layer = 5
			},
			(Rectangle){
				.x = -(halfWidth / 2.0f),
				.y = -(halfHeight / 2.0f),
				.width = halfWidth,
				.height = halfHeight
			},
			1.0f
		),
		.dash = (Dash){
			.max = 1,
			.cooldown = 0.5f
		},
		.gear = (Gear){
			.weaponSlot = 0,
			.maxWeaps = PLAYER_INIT_WEAPONS,
			.weapons = playerWeapons,
			.equippedWeaps = PLAYER_INIT_WEAPONS,
			.consumableSlot = 0,
			.maxConsumables = PLAYER_INIT_CONSUMABLES,
			.equippedConsumables = 0,
			.consumable = consumables,
		}
	};
}

void PlayerDashUpdate(Player* player, float dt) {
	player->dash.elapsed += dt;
	// When we reach max duration, clamp to max time and set status to not dashing.
	float trueDelta = dt;
	if (player->dash.elapsed >= DASH_DURATION) {
		float diff = player->dash.elapsed - DASH_DURATION;
		trueDelta -= diff;
		player->dash.dashing = false;
		player->entity.speed = 0.0f;

		// Set Dash cooldown only after all available consecutive dashes are used.
		if (player->dash.consecutive >= player->dash.max) {
			player->dash.cdLeft = player->dash.cooldown - diff;
			player->dash.consecutive = 0;
		}
	}
}

void PlayerStartDash(GameContext* context, Player* player) {
	SetStance(&player->entity, DASHING);
	player->dash.dashing = true;
	player->dash.consecutive++;
	player->entity.speed = player->speed * DASH_SPEED_MULT;
	float angle = DEG_270;
	if (context->options->dashMode == MOUSE) {
		Vector2 mpos = GetWorldMousePos(context);
		angle = Vector2LineAngle(player->entity.position, mpos);
		player->entity.anglev = (Vector2){ .x = cosf(angle), .y = -(sinf(angle)) };
	} else {
		player->entity.anglev = DirectionToVector(player->entity.dir);
	}
	player->dash.elapsed = 0.0f;
}

Direction PlayerUpdateDirection(Player* player) {
	if (player->entity.stun.active) {
		return NO_DIRECTION;
	}

	// Movement actions being pressed to pick current direction.
	bool isLeft = IsActionActive(GO_LEFT);
	bool isRight = IsActionActive(GO_RIGHT);
	bool isUp = IsActionActive(GO_UP);
	bool isDown = IsActionActive(GO_DOWN);

	// The player direction is set only when there are no conflicting inputs.
	// When there's a conflicting input, the direction will not be updated.
	// Thus, active direction is always the last valid one.
	// TODO: When player is going diagonally, there's often an update in between stopping pressing one key and the other.
	// FIX this with an action buffer or similar.
	char newDir = isRight ? 1 : 0;
	if (isLeft) {
		newDir = IsBitSet(newDir, 1) ? 0 : 2;
	}
	if (isUp) {
		newDir ^= 1 << 3;
	}
	if (isDown) {
		newDir = IsBitSet(newDir, 4) ? newDir ^ (1 << 3) : newDir ^ (1 << 2);
	}
	if (newDir != 0) {
		player->entity.speed = player->speed;
		player->entity.dir = (Direction) newDir;
		player->entity.anglev = DirectionToVector(player->entity.dir);
		SetStance(&player->entity, WALKING);
	} else {
		player->entity.speed = 0.0f;
		SetStance(&player->entity, STANDING);
	}

	return (Direction) newDir;
}

int EquipWeapon(Player* player, Weapon* weapon) {
	if (player->gear.weapons[0] == NULL) {
		player->gear.weapons[0] = weapon;
		return 0;
	}
	if (player->gear.weapons[1] == NULL) {
		player->gear.weapons[1] = weapon;
		return 1;
	}
	// TODO: Drop current weapon
	player->gear.weapons[player->gear.weaponSlot] = weapon;

	return player->gear.weaponSlot;
}

int SwapWeapon(Player* player) {
	player->gear.weaponSlot++;
	if (player->gear.weaponSlot >= player->gear.maxWeaps) {
		player->gear.weaponSlot = 0;
	}

	return player->gear.weaponSlot;
}

bool CanPlayerBeHit(Player* player) {
	return !player->dash.dashing && !player->entity.invuln.active;
}

void UpdateWeaponStatus(Player* player, float delta) {
	// Update weapon timers.
	for (int i = 0; i < player->gear.maxWeaps; i++) {
		Weapon* weapon = player->gear.weapons[i];
		if (weapon != NULL && weapon->attacking) {
			weapon->elapsed += delta;
			if (weapon->elapsed >= weapon->cooldown) {
				weapon->attacking = false;
			}
		}
	}

	// Check weapon swap.
	if (IsActionOnce(ACTION_SWAP) && player->gear.equippedWeaps > 1) {
		Weapon* weapon = player->gear.weapons[player->gear.weaponSlot];
		if (weapon == NULL || !weapon->attacking) {
			int nextSlot = player->gear.weaponSlot + 1;
			for (int i = 0; i < player->gear.equippedWeaps; i++) {
				if (nextSlot == player->gear.equippedWeaps) {
					nextSlot = 0;
				}
				if (player->gear.weapons[nextSlot] != NULL) {
					LogDebug("Swapped to weapon slot %d", nextSlot);
					player->gear.weaponSlot = nextSlot;
					break;
				}
				nextSlot++;
			}
		}
	}
}

void PlayerAttackAction(GameContext* context, Player* player, ObjectPool* attPool, Direction attackDir) {
	Weapon* usedWeapon = player->gear.weapons[player->gear.weaponSlot];
	if (usedWeapon != NULL && !usedWeapon->attacking) {
		if (usedWeapon->attack == NULL) {
			LogDebug("NULL attack on player weapon! %d %f", usedWeapon->type, usedWeapon->cooldown);
			return;
		}
		SetStance(&player->entity, ATTACKING);

		// Direction depends on the type of input.
		Vector2 attackPos;
		switch (attackDir) {
			case NORTH:
			case SOUTH:
			case EAST:
			case WEST:
			case NORTHEAST:
			case NORTHWEST:
			case SOUTHEAST:
			case SOUTHWEST:
				attackPos = AdvancePointByVector(player->entity.position, DirectionToVector(attackDir), TILE_SIZE);
				break;
			default: attackPos = GetWorldMousePos(context); break;
		}

		// Create attack instance.
		ActiveAttack att = InitiateAttack(&player->entity, &attackPos, usedWeapon->attack, T_ENEMY, true);
		void* result = AddToPool(attPool, &att);
		if (result == NULL) {
			LogDebug("Failed to allocate character attack on object pool: %d/%d", attPool->activeItems, attPool->length);
		} else {
			usedWeapon->attacking = true;
			usedWeapon->elapsed = 0.0f;
		}
	}
}

void AddRelic(Player* player, Relic* relic) {
	player->relics[player->relicCount++] = relic;
	player->dash.consecutive += relic->dashes;
	// TODO: Use percentage increments?
	player->speed += relic->speed;
	player->strength += relic->damage;
}

// Event to apply onHit effects from relics acquired during the run.
void onPlayerHit(Event* ev) {
	// TODO: This breaks potential local multiplayer and it is overall not good
	// Think out a better way to organise code that does not rely on global getters
	Player* player = GetPlayer();
	GameEntity* target = ev->params.phit.target;
	for (int i = 0; i < player->relicCount; i++) {
		if (player->relics[i]->onHit != NULL) {
			player->relics[i]->onHit(target);
		}
	}
}

void EmitPlayerHitEvent(GameEntity* target) {
	EmitEvent(&playerEvents, (Event){
		.type = E_PLAYER_HIT,
		.params = { .phit = (PlayerHitEvent){
			.target = target
		}}
	});
}

void PlayerCollideItem(Player* player, Item* item) {
	if (!item->active) {
		return;
	}
	switch (item->type) {
		case I_KEY: player->keys += item->amount; break;
		case I_EXP: player->exp += item->amount;break;
		case I_BOMB: player->bombs += item->amount; break;
		case I_RELIC:
			// For now can use amount to mark ID on relics. We'll see later.
			AddRelic(player, GetRelic(item->amount));
			break;
		case I_CONSUMABLE: /*TODO XD*/ break;
		case I_GEAR: /*TODO XD*/ break;
		default: break;
	}
	item->active = false;
}
