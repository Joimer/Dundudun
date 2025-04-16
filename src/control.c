#include <raylib.h>
#include "control.h"

bool IsActionPressed(GameAction action) {
	switch (action) {
		case LEFT: return IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A);
		case RIGHT: return IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);
		case UP: return IsKeyDown(KEY_UP) || IsKeyDown(KEY_W);
		case DOWN: return IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S);
		case ACCEPT: return IsKeyDown(KEY_ENTER);
		case CANCEL: return IsKeyDown(KEY_X);
		case ACTION_A: return IsKeyDown(KEY_Z);
		case ACTION_B: return IsKeyDown(KEY_X);
		case ACTION_C: return IsKeyDown(KEY_C);
		case ACTION_D: return IsKeyDown(KEY_SPACE);
		default: return false;
	}
}
