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
	ACTION_A,
	ACTION_B,
	ACTION_C,
	ACTION_D
} GameAction;

bool IsActionActive(GameAction action);
bool IsActionOnce(GameAction action);

#endif
