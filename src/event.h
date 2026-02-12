#ifndef EVENT_H
#define EVENT_H

typedef enum EventType {
    EVT_NONE = 0,
    EVT_COLLISION_ENTER,
    EVT_COLLISION_EXIT,
    EVT_ZONE_ENTER,
    EVT_ZONE_EXIT,
    EVT_INTERACT,
    EVT_DIALOG_START,
    EVT_DIALOG_END,
    EVT_COUNT
} EventType;

typedef struct Event {
    EventType type;
    int entity_id;
    int target_id;
    float x, y;
    void *data;
} Event;

typedef void (*EventCallback)(Event event, void *userdata);

typedef struct EventBus EventBus;

EventBus *event_bus_create(void);
void event_bus_destroy(EventBus *bus);

void event_subscribe(EventBus *bus, EventType type, EventCallback callback, void *userdata);
void event_unsubscribe(EventBus *bus, EventType type, EventCallback callback);

void event_emit(EventBus *bus, Event event);
void event_flush(EventBus *bus);
void event_clear(EventBus *bus);

#endif
