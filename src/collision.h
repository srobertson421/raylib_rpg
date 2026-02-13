#ifndef COLLISION_H
#define COLLISION_H

#include "raylib.h"
#include <stdbool.h>

#define COLLISION_MAX_BODIES 256

typedef enum BodyType {
    BODY_STATIC,
    BODY_KINEMATIC
} BodyType;

typedef enum BodyTag {
    TAG_NONE,
    TAG_WALL,
    TAG_PLAYER,
    TAG_NPC,
    TAG_DOOR
} BodyTag;

typedef struct CollisionBody {
    Rectangle rect;
    BodyType type;
    BodyTag tag;
    bool active;
    int elevation;
    void *user_data;
} CollisionBody;

typedef struct CollisionWorld {
    CollisionBody bodies[COLLISION_MAX_BODIES];
    int body_count;
    bool debug_draw;
} CollisionWorld;

typedef struct TileMap TileMap;

typedef struct ElevationRamp {
    Rectangle rect;
    int from_elevation;
    int to_elevation;
} ElevationRamp;

#define ELEVATION_MAX_RAMPS 64

typedef struct ElevationRampSet {
    ElevationRamp ramps[ELEVATION_MAX_RAMPS];
    int count;
} ElevationRampSet;

CollisionWorld *collision_create(void);
void collision_destroy(CollisionWorld *world);
int collision_add_body(CollisionWorld *world, Rectangle rect, BodyType type, BodyTag tag, int elevation, void *user_data);
void collision_remove_body(CollisionWorld *world, int index);
int collision_load_from_tilemap(CollisionWorld *world, TileMap *tilemap, const char *layer_name);
int collision_load_ramps_from_tilemap(ElevationRampSet *ramps, TileMap *tilemap, const char *layer_name);
Vector2 collision_move_and_slide(CollisionWorld *world, int body_index, float dx, float dy);
void collision_debug_draw(CollisionWorld *world);

#endif
