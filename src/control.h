#ifndef CONTROL_H
#define CONTROL_H

typedef enum {
	NONE,
	LEFT,
	RIGHT,
	UP,
	DOWN,
	ACCEPT,
	CANCEL,
	ACTION_A,
	ACTION_B,
	ACTION_C,
	ACTION_D
} GameAction;

bool IsActionPressed(GameAction action);

#endif
