/************************************************************************
 *                          Event system                                *
 * Game subsystems can set up their own events and subs.                *
 * When events are triggered, a callback can be executed synchronously, *
 * or a queue for async execution can be used.                          *
 ************************************************************************/

#ifndef EVENT_H
#define EVENT_H

#include "game.h"
#include "attack.h"

typedef enum { E_DMG, E_PLAYER_HIT } EventType;

typedef struct {
	GameEntity* entity;
	int amount;
	DamageType type;
} DamageEvent;

typedef struct {
	GameEntity* target;
} PlayerHitEvent;

typedef struct {
	EventType type;
	union {
		DamageEvent dmg;
		PlayerHitEvent phit;
	} params;
} Event;

typedef void EventCallback(Event* ev);

typedef struct {
	EventType type;
	EventCallback* cb;
	bool active;
} EventSub;

typedef struct {
	size_t max;
	size_t count;
	EventSub* subs;
} Observable;

size_t SubEvent(Observable* ob, EventType type, EventCallback* cb);
void UnsubEvent(Observable* ob, size_t sub);
void EmitEvent(Observable* ob, Event ev);
Observable CreateEventEmitter(size_t expectedSubs);
void DestroyEmitter(Observable* emitter);

#endif
