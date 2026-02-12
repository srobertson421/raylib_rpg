#include "event.h"
#include <stdlib.h>
#include <string.h>

#define EVENT_QUEUE_CAPACITY 256
#define MAX_LISTENERS_PER_TYPE 16

typedef struct Listener {
    EventCallback callback;
    void *userdata;
} Listener;

struct EventBus {
    // Ring buffer event queue
    Event queue[EVENT_QUEUE_CAPACITY];
    int head;
    int count;

    // Listener registry: up to MAX_LISTENERS_PER_TYPE per event type
    Listener listeners[EVT_COUNT][MAX_LISTENERS_PER_TYPE];
    int listener_count[EVT_COUNT];
};

EventBus *event_bus_create(void) {
    EventBus *bus = calloc(1, sizeof(EventBus));
    return bus;
}

void event_bus_destroy(EventBus *bus) {
    free(bus);
}

void event_subscribe(EventBus *bus, EventType type, EventCallback callback, void *userdata) {
    if (!bus || type <= EVT_NONE || type >= EVT_COUNT) return;
    int *count = &bus->listener_count[type];
    if (*count >= MAX_LISTENERS_PER_TYPE) return;

    bus->listeners[type][*count] = (Listener){ callback, userdata };
    (*count)++;
}

void event_unsubscribe(EventBus *bus, EventType type, EventCallback callback) {
    if (!bus || type <= EVT_NONE || type >= EVT_COUNT) return;
    int *count = &bus->listener_count[type];

    for (int i = 0; i < *count; i++) {
        if (bus->listeners[type][i].callback == callback) {
            // Shift remaining listeners down
            for (int j = i; j < *count - 1; j++) {
                bus->listeners[type][j] = bus->listeners[type][j + 1];
            }
            (*count)--;
            return;
        }
    }
}

void event_emit(EventBus *bus, Event event) {
    if (!bus || event.type <= EVT_NONE || event.type >= EVT_COUNT) return;
    if (bus->count >= EVENT_QUEUE_CAPACITY) return; // queue full, drop event

    int idx = (bus->head + bus->count) % EVENT_QUEUE_CAPACITY;
    bus->queue[idx] = event;
    bus->count++;
}

void event_flush(EventBus *bus) {
    if (!bus) return;

    // Snapshot count so events emitted by callbacks during flush
    // are deferred to the next flush (prevents infinite loops)
    int to_process = bus->count;

    for (int i = 0; i < to_process; i++) {
        Event evt = bus->queue[bus->head];
        bus->head = (bus->head + 1) % EVENT_QUEUE_CAPACITY;
        bus->count--;

        if (evt.type <= EVT_NONE || evt.type >= EVT_COUNT) continue;

        int lcount = bus->listener_count[evt.type];
        for (int j = 0; j < lcount; j++) {
            Listener *l = &bus->listeners[evt.type][j];
            if (l->callback) {
                l->callback(evt, l->userdata);
            }
        }
    }
}

void event_clear(EventBus *bus) {
    if (!bus) return;
    bus->head = 0;
    bus->count = 0;
}
