#include <stdlib.h>
#include <raylib.h>
#include "game.h"
#include "character.h"
#include "frame.h"
#include "resource.h"
#include "level.h"
#include "lib.h"

const Vector2 initialPos = { 100.0f, 100.0f };

inline unsigned long GenerateGameSeed() {
	// Seeds are 48 bits long.
	// 6 bits per character, all numbers and caps ASCII alphabet
	// From 000000000000000000000000000000000000000000000000
	// To   111111111111111111111111111111111111111111111111
	// We assume 64 bits system and let's go.
	unsigned long seed = 0;
	for (int i = 0; i < SEED_LENGTH; i++) {
		unsigned long part = GetRandomValue(0, 35);
		seed |= part << BITS_PER_SEED_CHAR * i;
	}
	return seed;
}

inline static char SeedIntToChar(int val) {
	if (val > 35) {
		val = val % 36;
	}
	if (val < 10) {
		return (char)(48 + val);
	}
	return (char)(55 + val);
}

const char* SeedToString(unsigned long seed) {
	char* seedString = malloc(SEED_LENGTH + 1);
	for (int i = 0; i < SEED_LENGTH; i++) {
		// Each 6 bits on the number defines a character.
		// Create a mask where only the bits corresponding to the character position are gotten.
		unsigned long mask = (((1 << BITS_PER_SEED_CHAR) - 1) << (i * BITS_PER_SEED_CHAR));
		// We only need the first bits here, so can use int for this.
		int charBits = (int)(seed & mask);
		seedString[i] = SeedIntToChar(charBits);
	}
	// string termination char.
	seedString[SEED_LENGTH] = '\0';

	// Note: This is used once currently, if used more, you would need to free() the result.
	return seedString;
}

void SetupGamePRNG(GameContext* context) {
	context->state->seed = GenerateGameSeed();
	context->state->seedStr = SeedToString(context->state->seed);
	// We want to players with the same seed to experience the same maps and drops and bosses.
	// Therefor, we use the MT Rand only for these, always in the same step.
	context->state->mtrand = SeedMTRand(context->state->seed);
}

void Update(GameContext* context, Player* player, Level* level) {
	float dt = GetFrameTime();
	if (player != NULL) {
		UpdatePlayer(context, player, dt);
		context->state->camera.target = player->entity.position;
	}
	if (IsKeyPressed(KEY_H)) {
		context->options->showGizmos = !context->options->showGizmos;
	}

	// Update all active entities.
	if (player != NULL && level != NULL && level->entityCount > 0) {
		for (int i = 0; i < level->entityCount; i++) {
			if (!level->entities[i].active) {
				continue;
			}
			// TODO: Own functions for entities for movement/action and state machine for those.
			// Check if player is within the entity's active area.
			if (IsPointInCircle(
				player->entity.position,
				(Circle){ level->entities[i].entity.position, level->entities[i].activeRadius }
			)) {
				//LogDebug("Enemy %d: Player inside entity active area!", i);
				if (level->entities[i].behaviour == APPROACH) {
					// Set direction towards player.
					// Min distance is entity hitbox in front of player hitbox.
					float xDiff = fabs(player->entity.position.x - level->entities[i].entity.position.x);
					float yDiff = fabs(player->entity.position.y - level->entities[i].entity.position.y);
					float xThreshold = player->entity.hitbox.width + level->entities[i].entity.hitbox.width;
					float yThreshold = player->entity.hitbox.height + level->entities[i].entity.hitbox.height;

					// Entity is close enough to player, ignore movement.
					if (xDiff < xThreshold && yDiff < yThreshold) {
						continue;
					}

					// Get the closest player hitbox corner to the enemy position.
					Vector2 closestCorner = ClosestRectCorner(
						(Rectangle){
							.x = player->entity.position.x + player->entity.hitbox.x,
							.y = player->entity.position.y + player->entity.hitbox.y,
							.width = player->entity.hitbox.width,
							.height = player->entity.hitbox.height
						},
						level->entities[i].entity.position
					);
					Direction dir = GetPointDirThreshold(
						level->entities[i].entity.position,
						closestCorner,
						level->entities[i].entity.hitbox.width,
						level->entities[i].entity.hitbox.height
					);

					// Hitbox is close enough to player, ignore movement.
					if (dir == NO_DIRECTION) {
						continue;
					}
					level->entities[i].entity.dir = (Direction) dir;
					bool isUp = IsBitSet(dir, 4);
					bool isDown = IsBitSet(dir, 3);
					bool isLeft = IsBitSet(dir, 2);
					bool isRight = IsBitSet(dir, 1);

					// Have to check if there's another hitbox in the desired direction.
					float maxSpeed = level->entities[i].speed * dt;

					// Entity hitbox rectangle on its own with the future movement thresholds.
					float neighbourXThres = isRight ? maxSpeed : (isLeft ? -maxSpeed : 0);
					float neighbourYThres = isUp ? -maxSpeed : (isDown ? maxSpeed : 0);
					Rectangle hitBoxArea = {
						.x = level->entities[i].entity.position.x + level->entities[i].entity.hitbox.x,
						.y = level->entities[i].entity.position.y + level->entities[i].entity.hitbox.y,
						.width = level->entities[i].entity.hitbox.width,
						.height = level->entities[i].entity.hitbox.height
					};
					Rectangle movedRect;

					for (int j = 0; j < level->entityCount; j++) {
						if (!isLeft && !isRight && !isUp && !isDown) {
							// We found out that the entity cannot move, no need for more calculations.
							break;
						}
						if (j == i) {
							// Ignore self.
							continue;
						}
						if (!level->entities[j].active) {
							// Ignore inactive entities.
							continue;
						}
						Rectangle entityWorldHitbox = {
							.x = level->entities[j].entity.position.x + level->entities[j].entity.hitbox.x,
							.y = level->entities[j].entity.position.y + level->entities[j].entity.hitbox.y,
							.width = level->entities[j].entity.hitbox.width,
							.height = level->entities[j].entity.hitbox.height
						};
						if (isLeft) {
							movedRect = hitBoxArea;
							movedRect.x += neighbourXThres;
							if (DoesRectCollideRect(movedRect, entityWorldHitbox)) {
								isLeft = false;
								continue;
							}
						}
						if (isRight) {
							movedRect = hitBoxArea;
							movedRect.x += neighbourXThres;
							if (DoesRectCollideRect(movedRect, entityWorldHitbox)) {
								isRight = false;
								continue;
							}
						}
						if (isUp) {
							movedRect = hitBoxArea;
							if (isLeft || isRight) {
								movedRect.x += neighbourXThres;
							}
							movedRect.y += neighbourYThres;
							if (DoesRectCollideRect(movedRect, entityWorldHitbox)) {
								isUp = false;
								continue;
							}
						}
						if (isDown) {
							movedRect = hitBoxArea;
							if (isLeft || isRight) {
								movedRect.x += neighbourXThres;
							}
							movedRect.y += neighbourYThres;
							if (DoesRectCollideRect(movedRect, entityWorldHitbox)) {
								isDown = false;
								continue;
							}
						}
					}

					// If the entity was completely stopped, we can check on next one already.
					if (!isLeft && !isRight && !isUp && !isDown) {
						continue;
					}

					// Final movement.
					float diagSpeed = maxSpeed * 0.7f;
					if (isLeft) {
						level->entities[i].entity.position.x -= (isUp || isDown ? diagSpeed : maxSpeed);
					}
					if (isRight) {
						level->entities[i].entity.position.x += (isUp || isDown ? diagSpeed : maxSpeed);
					}
					if (isUp) {
						level->entities[i].entity.position.y -= (isLeft || isRight ? diagSpeed : maxSpeed);
					}
					if (isDown) {
						level->entities[i].entity.position.y += (isLeft || isRight ? diagSpeed : maxSpeed);
					}
				}
			} else {
				// Inactive status.
				// If can be seen in screen or close by, idle behaviour.
				// Otherwise, completely ignore.
			}
		}
	}
}

int RunGame(GameContext* context) {
	// Game cursor configuration.
	if (!context->options->systemCursor) {
		HideCursor();
		LoadCustomCursor(context->options);
	}

	// Load necessary textures.
	RenderTexture2D worldRender = LoadRenderTexture(worldSize.width, worldSize.height);
	SetTextureFilter(worldRender.texture, TEXTURE_FILTER_POINT);

	// Load player character (TODO: Only on necessary screens)
	Texture2D* characterTexture = GetTexture(PLAYER_TEXTURE);
	Player player = CreatePlayer(characterTexture);

	// Generate initial seed (can be set by player later, too).
	SetupGamePRNG(context);

	// For development and testing, generate when appropriate and manage within context.
	Level level = GenerateLevel(context, 1);

	// Main game loop
	while (!WindowShouldClose()) {
		// First run the logic updates.
		Update(context, &player, &level);

		// Run draw frame logic.
		// TODO: Only playing level render needs player and level.
		Render(
			context,
			&worldRender,
			&player,
			&level
		);
	}

	// Unload resources before exit.
	UnloadTextures();

	// Free used memory.
	if (context->state->seedStr != NULL) {
		free((char*)context->state->seedStr);
	}
	CloseWindow();

	return GAME_CLOSE_SUCCESS;
}
