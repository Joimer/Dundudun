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
	defaultKeys[ACTION_ATT] = KEY_Z;
	defaultKeys[ACTION_SWAP] = KEY_X;
	defaultKeys[ACTION_BOMB] = KEY_C;
	defaultKeys[ACTION_DASH] = KEY_SPACE;
}
*/

// TODO: use this for key config menu
// const char *GetKeyName(int key);

static bool DoActionCheckGamepad(GameAction action, int pad, bool padFn(int, int)) {
	// TODO: float GetGamepadAxisMovement(int gamepad, int axis);
	// Axis will give a direction to assert any of the GO actions.
	switch (action) {
		case GO_LEFT: return padFn(pad, GAMEPAD_BUTTON_LEFT_FACE_LEFT);
		case GO_RIGHT: return padFn(pad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT);
		case GO_UP: return padFn(pad, GAMEPAD_BUTTON_LEFT_FACE_UP);
		case GO_DOWN: return padFn(pad, GAMEPAD_BUTTON_LEFT_FACE_DOWN);
		case ACCEPT: return padFn(pad, KEY_A);
		case CANCEL: return padFn(pad, KEY_A);
		case ACTION_ATT: return padFn(pad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
		case ACTION_SWAP: return padFn(pad, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
		case ACTION_BOMB: return padFn(pad, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
		case ACTION_DASH: return padFn(pad, GAMEPAD_BUTTON_RIGHT_FACE_UP);
		case ACTION_TAB: return padFn(pad, GAMEPAD_BUTTON_MIDDLE);
		default: return false;
	}
}

static bool DoActionCheck(GameAction action, bool fn(int), bool altFn(int)) {
	switch (action) {
		case GO_LEFT: return fn(KEY_A);
		case GO_RIGHT: return fn(KEY_D);
		case GO_UP: return fn(KEY_W);
		case GO_DOWN: return fn(KEY_S);
		case ACCEPT: return fn(KEY_ENTER);
		case CANCEL: return fn(KEY_X);
		case ACTION_ATT: return fn(KEY_Z) || altFn(MOUSE_BUTTON_LEFT);
		case ACTION_SWAP: return fn(KEY_X) || altFn(MOUSE_BUTTON_RIGHT);
		case ACTION_BOMB: return fn(KEY_Q);
		case ACTION_DASH: return fn(KEY_SPACE);
		case ACTION_TAB: return fn(KEY_TAB);
		default: return false;
	}
}

bool IsActionActive(GameAction action) {
	bool result = false;
	// TODO: actual gamepad detection
	for (int i = 0; i < 5; i++) {
		if (IsGamepadAvailable(i)) {
			result |= DoActionCheckGamepad(action, i, IsGamepadButtonDown);
		}
	}
	return result || DoActionCheck(action, IsKeyDown, IsMouseButtonDown);
}

bool IsActionOnce(GameAction action) {
	bool result = false;
	for (int i = 0; i < 5; i++) {
		if (IsGamepadAvailable(i)) {
			result |= DoActionCheckGamepad(action, i, IsGamepadButtonPressed);
		}
	}
	return result || DoActionCheck(action, IsKeyPressed, IsMouseButtonPressed);
}
