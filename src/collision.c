#include "collision.h"
#include "tilemap.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

CollisionWorld *collision_create(void) {
    CollisionWorld *world = calloc(1, sizeof(CollisionWorld));
    return world;
}

void collision_destroy(CollisionWorld *world) {
    if (world) free(world);
}

int collision_add_body(CollisionWorld *world, Rectangle rect, BodyType type, BodyTag tag, int elevation, void *user_data) {
    if (world->body_count >= COLLISION_MAX_BODIES) return -1;
    int index = world->body_count++;
    world->bodies[index].rect = rect;
    world->bodies[index].type = type;
    world->bodies[index].tag = tag;
    world->bodies[index].active = true;
    world->bodies[index].elevation = elevation;
    world->bodies[index].user_data = user_data;
    return index;
}

void collision_remove_body(CollisionWorld *world, int index) {
    if (index >= 0 && index < world->body_count) {
        world->bodies[index].active = false;
    }
}

int collision_load_from_tilemap(CollisionWorld *world, TileMap *tilemap, const char *layer_name) {
    if (!tilemap || !tilemap->loaded) return 0;

    int count = 0;
    for (int l = 0; l < tilemap->object_layer_count; l++) {
        ObjectLayer *layer = &tilemap->object_layers[l];
        if (strcmp(layer->name, layer_name) != 0) continue;

        for (int i = 0; i < layer->object_count; i++) {
            MapObject *obj = &layer->objects[i];

            // Skip ramp objects — they are loaded separately
            if (strcmp(obj->type, "elevation_ramp") == 0) continue;

            float ox = (float)obj->x;
            float oy = (float)obj->y;
            float ow = (float)obj->width;
            float oh = (float)obj->height;

            if (obj->rotation != 0.0) {
                /* Tiled rotates around top-left corner. Compute AABB of rotated rect. */
                float rad = (float)(obj->rotation * (3.14159265358979323846 / 180.0));
                float cos_r = cosf(rad);
                float sin_r = sinf(rad);

                /* Four corners relative to top-left (rotation pivot) */
                float cx[4] = { 0, ow, ow, 0 };
                float cy[4] = { 0, 0, oh, oh };

                float min_x = ox, max_x = ox;
                float min_y = oy, max_y = oy;
                for (int c = 0; c < 4; c++) {
                    float rx = ox + cx[c] * cos_r - cy[c] * sin_r;
                    float ry = oy + cx[c] * sin_r + cy[c] * cos_r;
                    if (rx < min_x) min_x = rx;
                    if (rx > max_x) max_x = rx;
                    if (ry < min_y) min_y = ry;
                    if (ry > max_y) max_y = ry;
                }

                Rectangle rect = { min_x, min_y, max_x - min_x, max_y - min_y };
                collision_add_body(world, rect, BODY_STATIC, TAG_WALL, obj->elevation, NULL);
            } else {
                Rectangle rect = { ox, oy, ow, oh };
                collision_add_body(world, rect, BODY_STATIC, TAG_WALL, obj->elevation, NULL);
            }
            count++;
        }
    }
    return count;
}

Vector2 collision_move_and_slide(CollisionWorld *world, int body_index, float dx, float dy) {
    if (body_index < 0 || body_index >= world->body_count) return (Vector2){0, 0};
    CollisionBody *body = &world->bodies[body_index];
    if (!body->active) return (Vector2){body->rect.x, body->rect.y};

    /* Step 1: Move X */
    body->rect.x += dx;
    for (int i = 0; i < world->body_count; i++) {
        if (i == body_index) continue;
        CollisionBody *other = &world->bodies[i];
        if (!other->active || other->type != BODY_STATIC) continue;
        if (other->elevation != body->elevation) continue;
        if (CheckCollisionRecs(body->rect, other->rect)) {
            if (dx > 0) {
                body->rect.x = other->rect.x - body->rect.width;
            } else if (dx < 0) {
                body->rect.x = other->rect.x + other->rect.width;
            }
        }
    }

    /* Step 2: Move Y */
    body->rect.y += dy;
    for (int i = 0; i < world->body_count; i++) {
        if (i == body_index) continue;
        CollisionBody *other = &world->bodies[i];
        if (!other->active || other->type != BODY_STATIC) continue;
        if (other->elevation != body->elevation) continue;
        if (CheckCollisionRecs(body->rect, other->rect)) {
            if (dy > 0) {
                body->rect.y = other->rect.y - body->rect.height;
            } else if (dy < 0) {
                body->rect.y = other->rect.y + other->rect.height;
            }
        }
    }

    return (Vector2){body->rect.x, body->rect.y};
}

int collision_load_ramps_from_tilemap(ElevationRampSet *ramps, TileMap *tilemap, const char *layer_name) {
    if (!tilemap || !tilemap->loaded || !ramps) return 0;

    ramps->count = 0;
    for (int l = 0; l < tilemap->object_layer_count; l++) {
        ObjectLayer *layer = &tilemap->object_layers[l];
        if (strcmp(layer->name, layer_name) != 0) continue;

        for (int i = 0; i < layer->object_count; i++) {
            MapObject *obj = &layer->objects[i];
            if (strcmp(obj->type, "elevation_ramp") != 0) continue;
            if (ramps->count >= ELEVATION_MAX_RAMPS) break;

            ElevationRamp *r = &ramps->ramps[ramps->count++];
            r->rect = (Rectangle){ (float)obj->x, (float)obj->y, (float)obj->width, (float)obj->height };
            r->from_elevation = obj->from_elevation;
            r->to_elevation = obj->to_elevation;
        }
    }
    return ramps->count;
}

void collision_debug_draw(CollisionWorld *world) {
    if (!world || !world->debug_draw) return;
    for (int i = 0; i < world->body_count; i++) {
        CollisionBody *b = &world->bodies[i];
        if (!b->active) continue;
        Color c;
        if (b->elevation == 0) {
            c = (b->type == BODY_STATIC) ? RED : GREEN;
        } else {
            c = (b->type == BODY_STATIC) ? BLUE : SKYBLUE;
        }
        DrawRectangleLinesEx(b->rect, 1.0f, c);
    }
}
