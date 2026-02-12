#include "tilemap.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Fix unescaped backslashes in JSON text (Tiled Windows path quirk)
static void fix_json_backslashes(char *text) {
    for (char *p = text; *p; p++) {
        if (*p == '\\') {
            char next = *(p + 1);
            if (next != '"' && next != '\\' && next != '/' &&
                next != 'b' && next != 'f' && next != 'n' &&
                next != 'r' && next != 't' && next != 'u') {
                *p = '/';
            }
        }
    }
}

static void strncpy_safe(char *dst, const char *src, size_t n) {
    if (!src) { dst[0] = '\0'; return; }
    strncpy(dst, src, n - 1);
    dst[n - 1] = '\0';
}

// Extract directory from a file path (result includes trailing slash)
static void get_directory(const char *path, char *dir, size_t dir_size) {
    strncpy_safe(dir, path, dir_size);
    // Normalize backslashes
    for (char *p = dir; *p; p++) {
        if (*p == '\\') *p = '/';
    }
    char *last_slash = strrchr(dir, '/');
    if (last_slash) {
        last_slash[1] = '\0';
    } else {
        dir[0] = '\0';
    }
}

// Extract just the filename from a path
static const char *get_filename(const char *path) {
    const char *slash = strrchr(path, '/');
    const char *bslash = strrchr(path, '\\');
    const char *last = slash > bslash ? slash : bslash;
    return last ? last + 1 : path;
}

// Try to load a texture, with fallback to just the filename in base_dir
static Texture2D load_texture_with_fallback(const char *image_path, const char *base_dir) {
    // First try: resolve relative to base_dir
    char resolved[512];
    snprintf(resolved, sizeof(resolved), "%s%s", base_dir, image_path);
    if (FileExists(resolved)) {
        printf("[tilemap] Loading texture: %s\n", resolved);
        return LoadTexture(resolved);
    }

    // Second try: just the filename in base_dir
    const char *fname = get_filename(image_path);
    snprintf(resolved, sizeof(resolved), "%s%s", base_dir, fname);
    if (FileExists(resolved)) {
        printf("[tilemap] Loading texture (fallback): %s\n", resolved);
        return LoadTexture(resolved);
    }

    // Third try: lowercase filename in base_dir
    char lower_fname[256];
    strncpy_safe(lower_fname, fname, sizeof(lower_fname));
    for (char *p = lower_fname; *p; p++) {
        if (*p >= 'A' && *p <= 'Z') *p += 32;
    }
    snprintf(resolved, sizeof(resolved), "%s%s", base_dir, lower_fname);
    if (FileExists(resolved)) {
        printf("[tilemap] Loading texture (lowercase fallback): %s\n", resolved);
        return LoadTexture(resolved);
    }

    printf("[tilemap] WARNING: Could not find texture: %s\n", image_path);
    return (Texture2D){0};
}

static bool parse_tileset_json(cJSON *ts_json, TilesetInfo *ts, const char *base_dir) {
    cJSON *item;

    item = cJSON_GetObjectItem(ts_json, "firstgid");
    if (item) ts->firstgid = item->valueint;

    item = cJSON_GetObjectItem(ts_json, "tilewidth");
    if (item) ts->tilewidth = item->valueint;

    item = cJSON_GetObjectItem(ts_json, "tileheight");
    if (item) ts->tileheight = item->valueint;

    item = cJSON_GetObjectItem(ts_json, "columns");
    if (item) ts->columns = item->valueint;

    item = cJSON_GetObjectItem(ts_json, "tilecount");
    if (item) ts->tilecount = item->valueint;

    item = cJSON_GetObjectItem(ts_json, "margin");
    if (item) ts->margin = item->valueint;

    item = cJSON_GetObjectItem(ts_json, "spacing");
    if (item) ts->spacing = item->valueint;

    item = cJSON_GetObjectItem(ts_json, "imagewidth");
    if (item) ts->imagewidth = item->valueint;

    item = cJSON_GetObjectItem(ts_json, "imageheight");
    if (item) ts->imageheight = item->valueint;

    item = cJSON_GetObjectItem(ts_json, "image");
    if (item && item->valuestring) {
        ts->texture = load_texture_with_fallback(item->valuestring, base_dir);
    }

    return ts->columns > 0 && ts->tilecount > 0;
}

static void parse_tile_layer(cJSON *layer_json, TileLayer *layer) {
    cJSON *item;

    item = cJSON_GetObjectItem(layer_json, "name");
    strncpy_safe(layer->name, item ? item->valuestring : "", sizeof(layer->name));

    item = cJSON_GetObjectItem(layer_json, "width");
    if (item) layer->width = item->valueint;

    item = cJSON_GetObjectItem(layer_json, "height");
    if (item) layer->height = item->valueint;

    item = cJSON_GetObjectItem(layer_json, "visible");
    layer->visible = item ? cJSON_IsTrue(item) : true;

    item = cJSON_GetObjectItem(layer_json, "opacity");
    layer->opacity = item ? (float)item->valuedouble : 1.0f;

    // Parse render_layer from Tiled custom properties
    layer->render_layer[0] = '\0';
    cJSON *props = cJSON_GetObjectItem(layer_json, "properties");
    if (props && cJSON_IsArray(props)) {
        cJSON *prop;
        cJSON_ArrayForEach(prop, props) {
            cJSON *pname = cJSON_GetObjectItem(prop, "name");
            if (pname && pname->valuestring && strcmp(pname->valuestring, "render_layer") == 0) {
                cJSON *pval = cJSON_GetObjectItem(prop, "value");
                if (pval && pval->valuestring) {
                    strncpy_safe(layer->render_layer, pval->valuestring, sizeof(layer->render_layer));
                }
                break;
            }
        }
    }

    cJSON *data = cJSON_GetObjectItem(layer_json, "data");
    if (data && cJSON_IsArray(data)) {
        int count = cJSON_GetArraySize(data);
        layer->data = (uint32_t *)malloc(count * sizeof(uint32_t));
        if (layer->data) {
            int i = 0;
            cJSON *gid;
            cJSON_ArrayForEach(gid, data) {
                layer->data[i++] = (uint32_t)gid->valuedouble;
            }
        }
    }
}

static void parse_object_layer(cJSON *layer_json, ObjectLayer *layer) {
    cJSON *item;

    item = cJSON_GetObjectItem(layer_json, "name");
    strncpy_safe(layer->name, item ? item->valuestring : "", sizeof(layer->name));

    item = cJSON_GetObjectItem(layer_json, "visible");
    layer->visible = item ? cJSON_IsTrue(item) : true;

    cJSON *objects = cJSON_GetObjectItem(layer_json, "objects");
    if (objects && cJSON_IsArray(objects)) {
        layer->object_count = cJSON_GetArraySize(objects);
        layer->objects = (MapObject *)calloc(layer->object_count, sizeof(MapObject));
        if (layer->objects) {
            int i = 0;
            cJSON *obj;
            cJSON_ArrayForEach(obj, objects) {
                MapObject *mo = &layer->objects[i++];

                item = cJSON_GetObjectItem(obj, "id");
                if (item) mo->id = item->valueint;

                item = cJSON_GetObjectItem(obj, "name");
                strncpy_safe(mo->name, item ? item->valuestring : "", sizeof(mo->name));

                item = cJSON_GetObjectItem(obj, "type");
                if (!item) item = cJSON_GetObjectItem(obj, "class");
                strncpy_safe(mo->type, item ? item->valuestring : "", sizeof(mo->type));

                item = cJSON_GetObjectItem(obj, "x");
                if (item) mo->x = item->valuedouble;

                item = cJSON_GetObjectItem(obj, "y");
                if (item) mo->y = item->valuedouble;

                item = cJSON_GetObjectItem(obj, "width");
                if (item) mo->width = item->valuedouble;

                item = cJSON_GetObjectItem(obj, "height");
                if (item) mo->height = item->valuedouble;

                item = cJSON_GetObjectItem(obj, "rotation");
                if (item) mo->rotation = item->valuedouble;

                item = cJSON_GetObjectItem(obj, "visible");
                mo->visible = item ? cJSON_IsTrue(item) : true;
            }
        }
    }
}

TileMap *tilemap_load(const char *path) {
    char *text = LoadFileText(path);
    if (!text) {
        printf("[tilemap] ERROR: Could not read file: %s\n", path);
        return NULL;
    }

    fix_json_backslashes(text);
    cJSON *root = cJSON_Parse(text);
    UnloadFileText(text);
    if (!root) {
        printf("[tilemap] ERROR: JSON parse failed for: %s\n", path);
        return NULL;
    }

    TileMap *map = (TileMap *)calloc(1, sizeof(TileMap));
    if (!map) {
        cJSON_Delete(root);
        return NULL;
    }

    // Get base directory for resolving relative paths
    char base_dir[512];
    get_directory(path, base_dir, sizeof(base_dir));

    // Map dimensions
    cJSON *item;
    item = cJSON_GetObjectItem(root, "width");
    if (item) map->width = item->valueint;

    item = cJSON_GetObjectItem(root, "height");
    if (item) map->height = item->valueint;

    item = cJSON_GetObjectItem(root, "tilewidth");
    if (item) map->tilewidth = item->valueint;

    item = cJSON_GetObjectItem(root, "tileheight");
    if (item) map->tileheight = item->valueint;

    printf("[tilemap] Map: %dx%d tiles, %dx%d px/tile\n",
           map->width, map->height, map->tilewidth, map->tileheight);

    // Parse tilesets
    cJSON *tilesets = cJSON_GetObjectItem(root, "tilesets");
    if (tilesets && cJSON_IsArray(tilesets)) {
        map->tileset_count = cJSON_GetArraySize(tilesets);
        map->tilesets = (TilesetInfo *)calloc(map->tileset_count, sizeof(TilesetInfo));

        int i = 0;
        cJSON *ts_entry;
        cJSON_ArrayForEach(ts_entry, tilesets) {
            TilesetInfo *ts = &map->tilesets[i++];

            // Get firstgid from the map entry (always present)
            item = cJSON_GetObjectItem(ts_entry, "firstgid");
            if (item) ts->firstgid = item->valueint;

            // Check if this is an external tileset reference
            item = cJSON_GetObjectItem(ts_entry, "source");
            if (item && item->valuestring) {
                // External tileset — load from file
                char ts_path[512] = {0};
                char source[256];
                strncpy_safe(source, item->valuestring, sizeof(source));

                // Normalize backslashes and strip leading ./
                for (char *p = source; *p; p++) {
                    if (*p == '\\') *p = '/';
                }
                char *src = source;
                if (src[0] == '.' && src[1] == '/') src += 2;

                // Try the source path as-is first
                snprintf(ts_path, sizeof(ts_path), "%s%s", base_dir, src);

                // If it's a .tsx reference, try .tsj instead
                char *ext = strrchr(ts_path, '.');
                if (ext && strcmp(ext, ".tsx") == 0) {
                    strcpy(ext, ".tsj");
                }

                printf("[tilemap] Loading external tileset: %s\n", ts_path);
                char *ts_text = LoadFileText(ts_path);
                if (ts_text) {
                    fix_json_backslashes(ts_text);
                    cJSON *ts_root = cJSON_Parse(ts_text);
                    UnloadFileText(ts_text);
                    if (ts_root) {
                        int saved_firstgid = ts->firstgid;
                        // Get tileset base dir for resolving image path
                        char ts_base_dir[512];
                        get_directory(ts_path, ts_base_dir, sizeof(ts_base_dir));
                        parse_tileset_json(ts_root, ts, ts_base_dir);
                        ts->firstgid = saved_firstgid;
                        cJSON_Delete(ts_root);
                    } else {
                        printf("[tilemap] WARNING: Failed to parse tileset JSON: %s\n", ts_path);
                    }
                } else {
                    printf("[tilemap] WARNING: Could not read tileset file: %s\n", ts_path);
                }
            } else {
                // Embedded tileset
                parse_tileset_json(ts_entry, ts, base_dir);
            }

            printf("[tilemap] Tileset: firstgid=%d, %dx%d tiles, %d columns, texture=%dx%d\n",
                   ts->firstgid, ts->tilewidth, ts->tileheight, ts->columns,
                   ts->texture.width, ts->texture.height);
        }
    }

    // Count layers by type
    cJSON *layers = cJSON_GetObjectItem(root, "layers");
    if (layers && cJSON_IsArray(layers)) {
        int tile_count = 0, obj_count = 0;
        cJSON *layer;
        cJSON_ArrayForEach(layer, layers) {
            item = cJSON_GetObjectItem(layer, "type");
            if (item && item->valuestring) {
                if (strcmp(item->valuestring, "tilelayer") == 0) tile_count++;
                else if (strcmp(item->valuestring, "objectgroup") == 0) obj_count++;
            }
        }

        map->tile_layer_count = tile_count;
        map->object_layer_count = obj_count;
        if (tile_count > 0)
            map->tile_layers = (TileLayer *)calloc(tile_count, sizeof(TileLayer));
        if (obj_count > 0)
            map->object_layers = (ObjectLayer *)calloc(obj_count, sizeof(ObjectLayer));

        int ti = 0, oi = 0;
        cJSON_ArrayForEach(layer, layers) {
            item = cJSON_GetObjectItem(layer, "type");
            if (!item || !item->valuestring) continue;

            if (strcmp(item->valuestring, "tilelayer") == 0 && ti < tile_count) {
                parse_tile_layer(layer, &map->tile_layers[ti]);
                printf("[tilemap] Tile layer %d: \"%s\" (%dx%d)\n",
                       ti, map->tile_layers[ti].name,
                       map->tile_layers[ti].width, map->tile_layers[ti].height);
                ti++;
            } else if (strcmp(item->valuestring, "objectgroup") == 0 && oi < obj_count) {
                parse_object_layer(layer, &map->object_layers[oi]);
                printf("[tilemap] Object layer %d: \"%s\" (%d objects)\n",
                       oi, map->object_layers[oi].name,
                       map->object_layers[oi].object_count);
                oi++;
            }
        }
    }

    map->loaded = true;
    cJSON_Delete(root);
    printf("[tilemap] Loaded successfully: %d tile layers, %d object layers\n",
           map->tile_layer_count, map->object_layer_count);
    return map;
}

void tilemap_unload(TileMap *map) {
    if (!map) return;

    for (int i = 0; i < map->tileset_count; i++) {
        if (map->tilesets[i].texture.id > 0) {
            UnloadTexture(map->tilesets[i].texture);
        }
    }
    free(map->tilesets);

    for (int i = 0; i < map->tile_layer_count; i++) {
        free(map->tile_layers[i].data);
    }
    free(map->tile_layers);

    for (int i = 0; i < map->object_layer_count; i++) {
        free(map->object_layers[i].objects);
    }
    free(map->object_layers);

    free(map);
}

void tilemap_draw_layer(TileMap *map, int layer_index, Camera2D camera) {
    if (!map || !map->loaded) return;
    if (layer_index < 0 || layer_index >= map->tile_layer_count) return;

    TileLayer *layer = &map->tile_layers[layer_index];
    if (!layer->visible || !layer->data) return;

    // Calculate visible tile range from camera
    float cam_x = camera.target.x - camera.offset.x / camera.zoom;
    float cam_y = camera.target.y - camera.offset.y / camera.zoom;
    float view_w = GetScreenWidth() / camera.zoom;
    float view_h = GetScreenHeight() / camera.zoom;

    int start_x = (int)(cam_x / map->tilewidth) - 1;
    int start_y = (int)(cam_y / map->tileheight) - 1;
    int end_x = (int)((cam_x + view_w) / map->tilewidth) + 2;
    int end_y = (int)((cam_y + view_h) / map->tileheight) + 2;

    if (start_x < 0) start_x = 0;
    if (start_y < 0) start_y = 0;
    if (end_x > layer->width) end_x = layer->width;
    if (end_y > layer->height) end_y = layer->height;

    Color tint = WHITE;
    if (layer->opacity < 1.0f) {
        tint.a = (unsigned char)(layer->opacity * 255.0f);
    }

    for (int y = start_y; y < end_y; y++) {
        for (int x = start_x; x < end_x; x++) {
            uint32_t raw_gid = layer->data[y * layer->width + x];
            uint32_t gid = raw_gid & GID_MASK;
            if (gid == 0) continue;

            // Find owning tileset (largest firstgid <= gid)
            TilesetInfo *ts = NULL;
            for (int t = map->tileset_count - 1; t >= 0; t--) {
                if ((uint32_t)map->tilesets[t].firstgid <= gid) {
                    ts = &map->tilesets[t];
                    break;
                }
            }
            if (!ts || ts->texture.id == 0) continue;

            int local_id = (int)gid - ts->firstgid;
            int col = local_id % ts->columns;
            int row = local_id / ts->columns;

            Rectangle src = {
                (float)(ts->margin + col * (ts->tilewidth + ts->spacing)),
                (float)(ts->margin + row * (ts->tileheight + ts->spacing)),
                (float)ts->tilewidth,
                (float)ts->tileheight
            };

            // Handle flip/rotation flags
            bool flipH = (raw_gid & FLIPPED_H_FLAG) != 0;
            bool flipV = (raw_gid & FLIPPED_V_FLAG) != 0;
            bool flipD = (raw_gid & FLIPPED_D_FLAG) != 0;

            Rectangle dst = {
                (float)(x * map->tilewidth),
                (float)(y * map->tileheight),
                (float)map->tilewidth,
                (float)map->tileheight
            };

            float rotation = 0;
            if (flipD) {
                if (flipH && flipV) {
                    rotation = 90.0f;
                    dst.x += map->tilewidth;
                    src.width = -src.width;
                } else if (flipH) {
                    rotation = 90.0f;
                    dst.x += map->tilewidth;
                } else if (flipV) {
                    rotation = 270.0f;
                    dst.y += map->tileheight;
                } else {
                    rotation = 270.0f;
                    dst.y += map->tileheight;
                    src.width = -src.width;
                }
            } else {
                if (flipH) src.width = -src.width;
                if (flipV) src.height = -src.height;
            }

            DrawTexturePro(ts->texture, src, dst, (Vector2){0, 0}, rotation, tint);
        }
    }
}

void tilemap_draw_all(TileMap *map, Camera2D camera) {
    if (!map) return;
    for (int i = 0; i < map->tile_layer_count; i++) {
        tilemap_draw_layer(map, i, camera);
    }
}
