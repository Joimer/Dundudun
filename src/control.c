#include <raylib.h>
#include "control.h"
#include "lib.h"

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

static bool IsPadAxisDirPressed(int gamepad, Direction dir) {
	// All of this shit will have to be configured hashahsahdshdhahdwqh	SOUL
	const float deadZone = 0.1f;
	float stickX = 0.0f;
	float stickY = 0.0f;
	if (stickX > -deadZone && stickX < deadZone) stickX = 0.0f;
    if (stickY > -deadZone && stickY < deadZone) stickY = 0.0f;
	switch (dir) {
		case NORTH: return stickY < 0.0f;
		case SOUTH: return stickY > 0.0f;
		case EAST: return stickX > 0.0f;
		case WEST: return stickX < 0.0f;
		case NORTHEAST: return stickY < 0.0f && stickX > 0.0f;
		case NORTHWEST: return stickY < 0.0f && stickX < 0.0f;
		case SOUTHEAST: return stickY > 0.0f && stickX > 0.0f;
		case SOUTHWEST: return stickY > 0.0f && stickX < 0.0f;
		default: break;
	}
	return false;
}

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
		case ACTION_ATT_DRIGHT: return padFn(pad, EAST);
		case ACTION_ATT_DLEFT: return padFn(pad, WEST);
		case ACTION_ATT_DUP: return padFn(pad, NORTH);
		case ACTION_ATT_DDOWN: return padFn(pad, SOUTH);
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
		case ACTION_ATT_DRIGHT: return fn(KEY_RIGHT);
		case ACTION_ATT_DLEFT: return fn(KEY_LEFT);
		case ACTION_ATT_DUP: return fn(KEY_UP);
		case ACTION_ATT_DDOWN: return fn(KEY_DOWN);
		default: return false;
	}
}

bool IsActionActive(GameAction action) {
	bool result = false;
	// TODO: actual gamepad detection
	for (int i = 0; i < 5; i++) {
		if (IsGamepadAvailable(i)) {
			switch (action) {
				case ACTION_ATT_DRIGHT: result |= IsPadAxisDirPressed(i, EAST); break;
				case ACTION_ATT_DLEFT: result |= IsPadAxisDirPressed(i, WEST); break;
				case ACTION_ATT_DUP: result |= IsPadAxisDirPressed(i, NORTH); break;
				case ACTION_ATT_DDOWN: result |= IsPadAxisDirPressed(i, SOUTH); break;
				default: result |= DoActionCheckGamepad(action, i, IsGamepadButtonDown);
			}
		}
	}
	return result || DoActionCheck(action, IsKeyDown, IsMouseButtonDown);
}

bool IsActionOnce(GameAction action) {
	bool result = false;
	for (int i = 0; i < 5; i++) {
		if (IsGamepadAvailable(i)) {
			switch (action) {
				case ACTION_ATT_DRIGHT: result |= IsPadAxisDirPressed(i, EAST); break;
				case ACTION_ATT_DLEFT: result |= IsPadAxisDirPressed(i, WEST); break;
				case ACTION_ATT_DUP: result |= IsPadAxisDirPressed(i, NORTH); break;
				case ACTION_ATT_DDOWN: result |= IsPadAxisDirPressed(i, SOUTH); break;
				default: result |= DoActionCheckGamepad(action, i, IsGamepadButtonPressed);
			}
		}
	}
	return result || DoActionCheck(action, IsKeyPressed, IsMouseButtonPressed);
}
