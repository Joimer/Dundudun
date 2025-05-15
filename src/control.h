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
} GameAction;

bool IsActionActive(GameAction action);
bool IsActionOnce(GameAction action);

#endif
