#include <stdlib.h>
#include "event.h"

size_t SubEvent(Observable* ob, EventType type, EventCallback* cb) {
	size_t id = ob->count;
	ob->count++;
	// Subscriptions happen only a few times per game execution,
	// so it should make sense to simply realloc a few times for each sub
	// since we don't really need to allocate extra memory, would be a waste.
	if (ob->count > ob->max) {
		ob->subs = realloc(ob->subs, ob->count);
		if (ob->subs == NULL) {
			return 0;
		}
	}
	ob->max = ob->count;
	ob->subs[id] = (EventSub){ .cb = cb, .type = type, .active = true };

	return id;
}

void UnsubEvent(Observable* ob, size_t sub) {
	if (sub >= ob->count) {
		return;
	}
	// Would be better to dealloc probably if this is needed.
	// Thing is same function pointer can be used to sub an event
	// so we need a unique identifier for each sub
	// No plans to use unsub yet tho, just feels like it should exist.
	ob->subs[sub].active = false;
}

void EmitEvent(Observable* ob, Event ev) {
	for (int i = 0; i < ob->count; i++) {
		if (ob->subs[i].type == ev.type) {
			ob->subs[i].cb(&ev);
		}
	}
}

Observable CreateEventEmitter(size_t expectedSubs) {
	return (Observable){
		.max = expectedSubs,
		.count = 0,
		.subs = expectedSubs == 0 ? NULL : malloc(sizeof(EventSub) * expectedSubs)
	};
}

void DestroyEmitter(Observable* emitter) {
	if (emitter->subs != NULL) {
		free(emitter->subs);
		emitter->max = 0;
		emitter->count = 0;
	}
}
