#include <raylib.h>
#include <raymath.h>
#include <math.h>
#include <stdlib.h>
#include "character.h"
#include "control.h"
#include "frame.h"
#include "game.h"
#include "item.h"

Player CreatePlayer(Texture2D* characterTexture) {
	float halfWidth = (float) characterTexture->width / 2.0f;
	float halfHeight = (float) characterTexture->height / 2.0f;
	Weapon** playerWeapons = malloc(sizeof(Weapon*) * PLAYER_MAX_WEAPONS);
	// Pre-assign initial weapons.
	// This will not be done here and initial room will have them so player can learn how to interact with items.
	playerWeapons[0] = GetWeapon(0);
	playerWeapons[1] = GetWeapon(1);
	// TODO: What if NULL? Crash game gracefully? Reset it and count failures?
	return (Player){
		.speed = PLAYER_SPEED,
		.entity = (GameEntity){
			.health = 50,
			.maxHealth = 50,
			.position = (Vector2){ initialPos.x, initialPos.y },
			.dir = SOUTH,
			.sprite = (Sprite){
				.texture = characterTexture,
				.position = { halfWidth, halfHeight },
				.rect = { 0.0f, 0.0f, characterTexture->width, characterTexture->height },
				.visible = true,
				.layer = 5
			},
			.hitbox = (Rectangle){
				.x = -(halfWidth / 2.0f),
				.y = -(halfHeight / 2.0f),
				.width = halfWidth,
				.height = halfHeight
			},
			.invuln = (Invulnerability){ .duration = 1.0f }
		},
		.dash = (Dash){
			.max = 1,
			.cooldown = 0.5f
		},
		.gear = (Gear){
			.weaponSlot = 0,
			.maxWeaps = PLAYER_MAX_WEAPONS,
			.weapons = playerWeapons
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
	float dashSpeed = player->speed * DASH_SPEED_MULT;
	float angle = DEG_270;
	Vector2 dir;
	if (context->options->dashMode == MOUSE) {
		Vector2 mpos = GetWorldMousePos(context);
		angle = Vector2LineAngle(player->entity.position, mpos);
		player->entity.anglev = (Vector2){ .x = cosf(angle), .y = -(sinf(angle)) };
	} else {
		player->entity.anglev = DirectionToVector(player->entity.dir);
	}

	// TODO: Should just set the direction vector and manage speed on dash object or elsewhere, probably.
	player->dash.direction = (Vector2){ .x = dir.x * dashSpeed, .y = dir.y * dashSpeed };
	player->dash.elapsed = 0.0f;
}

Direction PlayerUpdateDirection(Player* player) {
	// Movement actions being pressed to pick current direction.
	bool isLeft = IsActionPressed(GO_LEFT);
	bool isRight = IsActionPressed(GO_RIGHT);
	bool isUp = IsActionPressed(GO_UP);
	bool isDown = IsActionPressed(GO_DOWN);

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
