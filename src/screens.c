#include "game.h"
#include "screens.h"
#include "level.h"

void LoadNextScreen(GameContext* context, GameScreen next) {
	if (next == GAMEPLAY) {
		SetupLevel(context);
	}
	context->state->screen = next;
}
