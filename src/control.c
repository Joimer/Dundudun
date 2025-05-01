#include <raylib.h>
#include "control.h"

/* TODO with more fine grained controls
static int defaultKeys[11];

static void SetDefaultKeys() {
	defaultKeys[GO_LEFT] = KEY_A;
	defaultKeys[GO_RIGHT] = KEY_D;
	defaultKeys[GO_UP] = KEY_W;
	defaultKeys[GO_DOWN] = KEY_S;
	defaultKeys[ACCEPT] = KEY_ENTER;
	defaultKeys[CANCEL] = KEY_X;
	defaultKeys[ACTION_A] = KEY_Z;
	defaultKeys[ACTION_B] = KEY_X;
	defaultKeys[ACTION_C] = KEY_C;
	defaultKeys[ACTION_D] = KEY_SPACE;
}
*/

static bool DoActionCheck(GameAction action, bool fn(int), bool altFn(int)) {
	switch (action) {
		case GO_LEFT: return fn(KEY_A);
		case GO_RIGHT: return fn(KEY_D);
		case GO_UP: return fn(KEY_W);
		case GO_DOWN: return fn(KEY_S);
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
