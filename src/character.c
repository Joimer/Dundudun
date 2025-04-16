#include <raylib.h>
#include <raymath.h>
#include <math.h>
#include "character.h"
#include "control.h"
#include "frame.h"
#include "game.h"

#define PLAYER_SPEED 200.0f
#define PLAYER_SPEED_DIAGONAL 132.0f
#define DASH_SPEED_MULT 4.0f
#define DASH_DURATION 0.25f
#define DASH_LENGTH 150.0f
#define DEG_360 PI * 2
#define DEG_45 PI / 4
#define DEG_90 PI / 2
#define DEG_135 3 * PI / 4
#define DEG_225 5 * PI / 4
#define DEG_270 3 * PI / 2
#define DEG_315 7 * PI / 4

Player CreatePlayer(Texture2D* characterTexture) {
	float halfWidth = (float) characterTexture->width / 2.0f;
	float halfHeight = (float) characterTexture->height / 2.0f;
	return (Player){
		.entity = (GameEntity){
			.health = 50,
			.position = (Vector2){ initialPos.x, initialPos.y },
			.sprite = (Sprite){
				.texture = characterTexture,
				.position = { halfWidth, halfHeight },
				.rect = { 0.0f, 0.0f, characterTexture->width, characterTexture->height },
			},
			.hitbox = (Rectangle){
				.x = -(halfWidth / 2.0f),
				.y = -(halfHeight / 2.0f),
				.width = halfWidth,
				.height = halfHeight,
			}
		}
	};
}

void UpdatePlayer(GameContext* context, Player* player, float delta) {
	// Player is mid dash, no control on actions until it is finished.
	if (player->dash.dashing) {
		player->dash.elapsed += delta;
		// When we reach max duration, clamp to max time and set status to not dashing.
		float trueDelta = delta;
		if (player->dash.elapsed >= DASH_DURATION) {
			float diff = player->dash.elapsed - DASH_DURATION;
			trueDelta -= diff;
			player->dash.dashing = false;
		}
		Vector2 moveVector = (Vector2){ .x = player->dash.direction.x * trueDelta, .y = player->dash.direction.y * trueDelta };
		player->entity.position = Vector2Add(player->entity.position, moveVector);
		return;
	}

	// TODO: This, but better.
	bool isLeft = IsActionPressed(LEFT);
	bool isRight = IsActionPressed(RIGHT);
	bool isUp = IsActionPressed(UP);
	bool isDown = IsActionPressed(DOWN);

	// Movement actions.
	if (isLeft) {
		player->entity.position.x -= (isUp || isDown ? PLAYER_SPEED_DIAGONAL : PLAYER_SPEED) * delta;
	}
	if (isRight) {
		player->entity.position.x += (isUp || isDown ? PLAYER_SPEED_DIAGONAL : PLAYER_SPEED) * delta;
	}
	if (isUp) {
		player->entity.position.y -= (isLeft || isRight ? PLAYER_SPEED_DIAGONAL : PLAYER_SPEED) * delta;
	}
	if (isDown) {
		player->entity.position.y += (isLeft || isRight ? PLAYER_SPEED_DIAGONAL : PLAYER_SPEED) * delta;
	}

	// Execute dash.
	if (IsActionPressed(ACTION_D)) {
		player->dash.dashing = true;
		float dashSpeed = PLAYER_SPEED * DASH_SPEED_MULT;
		float angle;
		if (context->options->dashMode == MOUSE) {
			Vector2 mpos = GetWorldMousePos(context);
			angle = Vector2LineAngle(player->entity.position, mpos);
		} else {
			// TODO: Set default dash direction to last direction when stopped.
			if (isUp) {
				if (isLeft) {
					angle = DEG_135;
				} else if (isRight) {
					angle = DEG_45;
				} else {
					angle = DEG_90;
				}
			}
			if (isDown) {
				if (isLeft) {
					angle = DEG_225;
				} else if (isRight) {
					angle = DEG_315;
				} else {
					angle = DEG_270;
				}
			}
			if (isLeft && !isUp && !isDown) {
				angle = PI;
			}
			if (isRight && !isUp && !isDown) {
				angle = DEG_360;
			}
		}
		player->dash.direction = (Vector2){ .x = cosf(angle) * dashSpeed, .y = -(sinf(angle) * dashSpeed) };
		player->dash.elapsed = 0.0f;
		return;
	}

	// Attack.
	if (IsActionPressed(ACTION_A)) {

	}
}
