#include <raylib.h>
#include "control.h"

static bool DoActionCheck(GameAction action, bool fn(int), bool altFn(int)) {
	switch (action) {
		case GO_LEFT: return fn(KEY_LEFT) || fn(KEY_A);
		case GO_RIGHT: return fn(KEY_RIGHT) || fn(KEY_D);
		case GO_UP: return fn(KEY_UP) || fn(KEY_W);
		case GO_DOWN: return fn(KEY_DOWN) || fn(KEY_S);
		case ACCEPT: return fn(KEY_ENTER);
		case CANCEL: return fn(KEY_X);
		case ACTION_A: return fn(KEY_Z) || altFn(MOUSE_BUTTON_LEFT);
		case ACTION_B: return fn(KEY_X) || altFn(MOUSE_BUTTON_RIGHT);
		case ACTION_C: return fn(KEY_C);
		case ACTION_D: return fn(KEY_SPACE);
		default: return false;
	}
}

bool IsActionActive(GameAction action) {
	return DoActionCheck(action, IsKeyDown, IsMouseButtonDown);
}

bool IsActionOnce(GameAction action) {
	return DoActionCheck(action, IsKeyPressed, IsMouseButtonPressed);
}
