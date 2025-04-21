#include <raylib.h>
#include <raymath.h>
#include <math.h>
#include "character.h"
#include "control.h"
#include "frame.h"
#include "game.h"
#include "item.h"

#define PLAYER_SPEED 200.0f
#define PLAYER_SPEED_DIAGONAL 140.0f
#define DASH_SPEED_MULT 2.5f
#define DASH_DURATION 0.25f
#define DASH_LENGTH 150.0f
#define DEG_360 PI * 2.0f
#define DEG_45 PI / 4.0f
#define DEG_90 PI / 2.0f
#define DEG_135 3.0f * PI / 4.0f
#define DEG_225 5.0f * PI / 4.0f
#define DEG_270 3.0f * PI / 2.0f
#define DEG_315 7.0f * PI / 4.0f

Player CreatePlayer(Texture2D* characterTexture) {
	float halfWidth = (float) characterTexture->width / 2.0f;
	float halfHeight = (float) characterTexture->height / 2.0f;
	return (Player){
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
		}
	};
}

void UpdatePlayer(GameContext* context, Player* player, float delta) {
	// Check invulnerability status.
	UpdateInvuln(&player->entity, delta);

	// Player is mid dash, no control on actions until it is finished.
	if (player->dash.dashing) {
		player->dash.elapsed += delta;
		// When we reach max duration, clamp to max time and set status to not dashing.
		float trueDelta = delta;
		if (player->dash.elapsed >= DASH_DURATION) {
			float diff = player->dash.elapsed - DASH_DURATION;
			trueDelta -= diff;
			player->dash.dashing = false;
			if (player->dash.consecutive >= player->dash.max) {
				player->dash.cdLeft = player->dash.cooldown - diff;
				player->dash.consecutive = 0;
			}
		}
		Vector2 moveVector = (Vector2){ .x = player->dash.direction.x * trueDelta, .y = player->dash.direction.y * trueDelta };
		player->entity.position = Vector2Add(player->entity.position, moveVector);
		return;
	}

	// Update physic state counters.
	if (player->dash.cdLeft > 0) {
		player->dash.cdLeft -= delta > player->dash.cdLeft ? player->dash.cdLeft : delta;
	}

	// TODO: Add pushback here.
	// Player cannot move or act during a pushback action.

	// Movement actions being pressed.
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
		player->entity.dir = (Direction) newDir;
	}

	// Execute movement.
	if (IsBitSet(newDir, 1)) {
		// East
		player->entity.position.x += (IsBitSet(newDir, 3) || IsBitSet(newDir, 4) ? PLAYER_SPEED_DIAGONAL : PLAYER_SPEED) * delta;
	}
	if (IsBitSet(newDir, 2)) {
		// West
		player->entity.position.x -= (IsBitSet(newDir, 3) || IsBitSet(newDir, 4) ? PLAYER_SPEED_DIAGONAL : PLAYER_SPEED) * delta;
	}
	if (IsBitSet(newDir, 3)) {
		// South
		player->entity.position.y += (IsBitSet(newDir, 1) || IsBitSet(newDir, 2) ? PLAYER_SPEED_DIAGONAL : PLAYER_SPEED) * delta;
	}
	if (IsBitSet(newDir, 4)) {
		// North
		player->entity.position.y -= (IsBitSet(newDir, 1) || IsBitSet(newDir, 2) ? PLAYER_SPEED_DIAGONAL : PLAYER_SPEED) * delta;
	}

	// Execute dash.
	if (IsActionPressed(ACTION_D) && player->dash.cdLeft == 0.0f) {
		player->dash.dashing = true;
		player->dash.consecutive++;
		float dashSpeed = PLAYER_SPEED * DASH_SPEED_MULT;
		float angle = DEG_270;
		if (context->options->dashMode == MOUSE) {
			Vector2 mpos = GetWorldMousePos(context);
			angle = Vector2LineAngle(player->entity.position, mpos);
		} else {
			switch (player->entity.dir) {
				case NORTH: angle = DEG_90; break;
				case SOUTH: angle = DEG_270; break;
				case EAST: angle = DEG_360; break;
				case WEST: angle = PI; break;
				case NORTHEAST: angle = DEG_45; break;
				case NORTHWEST: angle = DEG_135; break;
				case SOUTHEAST: angle = DEG_315; break;
				case SOUTHWEST: angle = DEG_225; break;
				case NO_DIRECTION: break;
			}
		}
		player->dash.direction = (Vector2){ .x = cosf(angle) * dashSpeed, .y = -(sinf(angle) * dashSpeed) };
		player->dash.elapsed = 0.0f;
		return;
	}

	// Attack.
	if (IsActionPressed(ACTION_A)) {
		LogDebug("Atttaaaaack");
	}
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
