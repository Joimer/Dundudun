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

bool IsActionPressed(GameAction action);

#endif
