/*********************************
 * Game input control and config *
 *********************************/

#ifndef CONTROL_H
#define CONTROL_H

typedef enum {
	NONE,
	GO_LEFT,
	GO_RIGHT,
	GO_UP,
	GO_DOWN,
	ACCEPT,
	CANCEL,
	ACTION_ATT,
	ACTION_SWAP,
	ACTION_BOMB,
	ACTION_DASH,
	ACTION_TAB,
	ACTION_ATT_DRIGHT,
	ACTION_ATT_DLEFT,
	ACTION_ATT_DUP,
	ACTION_ATT_DDOWN,
} GameAction;

bool IsActionActive(GameAction action);
bool IsActionOnce(GameAction action);

#endif
