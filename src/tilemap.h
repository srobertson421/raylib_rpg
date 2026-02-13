#ifndef TILEMAP_H
#define TILEMAP_H

#include "raylib.h"
#include <stdint.h>
#include <stdbool.h>

#define FLIPPED_H_FLAG  0x80000000u
#define FLIPPED_V_FLAG  0x40000000u
#define FLIPPED_D_FLAG  0x20000000u
#define GID_MASK        0x0FFFFFFFu

typedef struct TileAnimFrame {
    int tileid;       // local tile ID to display
    int duration;     // milliseconds
} TileAnimFrame;

typedef struct TileAnim {
    TileAnimFrame *frames;
    int frame_count;
    int total_duration;  // precomputed sum of all frame durations (ms)
} TileAnim;

typedef struct TilesetInfo {
    int firstgid;
    int tilewidth;
    int tileheight;
    int columns;
    int tilecount;
    int margin;
    int spacing;
    int imagewidth;
    int imageheight;
    Texture2D texture;
    TileAnim *anim_lookup;   // array of size tilecount, indexed by local tile ID
                              // .frame_count == 0 means not animated
} TilesetInfo;

typedef struct TileLayer {
    char name[64];
    int width;
    int height;
    uint32_t *data;
    bool visible;
    float opacity;
    char render_layer[32];
    int elevation;
    char shader_name[32];   // Tiled custom property "shader" (e.g., "water")
} TileLayer;

typedef struct MapObject {
    int id;
    char name[64];
    char type[64];
    double x, y;
    double width, height;
    double rotation;
    bool visible;
    int elevation;
    int from_elevation;
    int to_elevation;
} MapObject;

typedef struct ObjectLayer {
    char name[64];
    MapObject *objects;
    int object_count;
    bool visible;
} ObjectLayer;

typedef struct TileMap {
    int width;
    int height;
    int tilewidth;
    int tileheight;

    TilesetInfo *tilesets;
    int tileset_count;

    TileLayer *tile_layers;
    int tile_layer_count;

    ObjectLayer *object_layers;
    int object_layer_count;

    bool loaded;
    double anim_time;         // global animation clock in milliseconds
} TileMap;

TileMap *tilemap_load(const char *path);
void tilemap_unload(TileMap *map);
void tilemap_update(TileMap *map, float dt);
void tilemap_draw_layer(TileMap *map, int layer_index, Camera2D camera);
void tilemap_draw_layer_tinted(TileMap *map, int layer_index, Camera2D camera, Color tint);
void tilemap_draw_all(TileMap *map, Camera2D camera);
MapObject *tilemap_find_object(TileMap *map, const char *layer_name, const char *type);

#endif
