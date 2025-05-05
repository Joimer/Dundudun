#ifndef EVENT_H
#define EVENT_H

#include "game.h"
#include "attack.h"

typedef enum { E_DMG } EventType;

typedef struct {
	GameEntity* entity;
	int amount;
	DamageType type;
} DamageEvent;

typedef struct {
	EventType type;
	union {
		DamageEvent dmg;
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
