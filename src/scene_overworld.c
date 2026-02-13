#include "game.h"
#include "scene.h"
#include "tilemap.h"
#include "collision.h"
#include "sprite.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

typedef struct OverworldData {
    TileMap *tilemap;
    CollisionWorld *collision_world;
    int player_body;
    float pos_x, pos_y;
    ElevationRampSet ramps;
    int player_elevation;
    int last_ramp;          // index of ramp that last fired (-1 = none)
} OverworldData;

static void overworld_init(Game *game) {
    OverworldData *data = calloc(1, sizeof(OverworldData));
    game->scene_data[SCENE_OVERWORLD] = data;

    // Reset player HP to full when entering overworld
    game->player_hp = game->player_max_hp;

    data->tilemap = tilemap_load("../assets/overworld.tmj");

    // Player start position (from Tiled marker, fallback to center of map)
    data->pos_x = 400.0f;
    data->pos_y = 300.0f;
    if (data->tilemap && data->tilemap->loaded) {
        MapObject *spawn = tilemap_find_object(data->tilemap, "objects_markers", "player_start");
        if (spawn) {
            data->pos_x = (float)spawn->x;
            data->pos_y = (float)spawn->y;
        } else {
            data->pos_x = (data->tilemap->width * data->tilemap->tilewidth) / 2.0f;
            data->pos_y = (data->tilemap->height * data->tilemap->tileheight) / 2.0f;
        }
    }

    // Collision setup
    data->collision_world = collision_create();
    if (data->tilemap && data->tilemap->loaded) {
        collision_load_from_tilemap(data->collision_world, data->tilemap, "objects_collision");
    }
    data->player_elevation = 0;
    data->last_ramp = -1;
    data->player_body = collision_add_body(
        data->collision_world,
        (Rectangle){ data->pos_x, data->pos_y, 16, 16 },
        BODY_KINEMATIC, TAG_PLAYER, 0, NULL
    );
    if (data->tilemap && data->tilemap->loaded) {
        collision_load_ramps_from_tilemap(&data->ramps, data->tilemap, "objects_collision");
    }

    // Point camera at player
    game->camera.target = (Vector2){ data->pos_x, data->pos_y };
    game->camera.offset = (Vector2){ GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
    // BGM is now triggered by EVT_SCENE_ENTER event
}

static void overworld_cleanup(Game *game) {
    OverworldData *data = game->scene_data[SCENE_OVERWORLD];
    if (!data) return;

    if (data->tilemap) {
        tilemap_unload(data->tilemap);
    }
    if (data->collision_world) {
        collision_destroy(data->collision_world);
    }
    free(data);
    game->scene_data[SCENE_OVERWORLD] = NULL;
}

static void overworld_update(Game *game) {
    OverworldData *data = game->scene_data[SCENE_OVERWORLD];
    if (!data) return;

    // Advance tile animations
    tilemap_update(data->tilemap, GetFrameTime());

    // Toggle debug draw
    if (IsKeyPressed(KEY_F3)) {
        data->collision_world->debug_draw = !data->collision_world->debug_draw;
    }

    // Scene transition: press 1 to enter dungeon
    if (IsKeyPressed(KEY_ONE)) {
        game->next_scene = SCENE_DUNGEON_1;
        return;
    }

    // Scene transition: press 2 to enter battle
    if (IsKeyPressed(KEY_TWO)) {
        game->next_scene = SCENE_BATTLE;
        return;
    }

    // Player movement
    float dx = 0, dy = 0;
    if (IsKeyDown(KEY_RIGHT)) dx += game->speed;
    if (IsKeyDown(KEY_LEFT))  dx -= game->speed;
    if (IsKeyDown(KEY_DOWN))  dy += game->speed;
    if (IsKeyDown(KEY_UP))    dy -= game->speed;

    // Normalize diagonal movement
    if (dx != 0 && dy != 0) {
        float inv_len = game->speed / sqrtf(dx * dx + dy * dy);
        dx *= inv_len;
        dy *= inv_len;
    }

    // Animation direction
    bool moving = (dx != 0 || dy != 0);
    if (moving) {
        if (fabsf(dx) >= fabsf(dy)) {
            game->facing = (dx > 0) ? 1 : 3;
        } else {
            game->facing = (dy > 0) ? 0 : 2;
        }
        sprite_play(game->player_sprite, game->facing);
    } else {
        sprite_stop(game->player_sprite);
    }
    sprite_update(game->player_sprite, GetFrameTime());

    // Move with collision
    collision_move_and_slide(data->collision_world, data->player_body, dx, dy);
    CollisionBody *pbody = &data->collision_world->bodies[data->player_body];
    data->pos_x = pbody->rect.x;
    data->pos_y = pbody->rect.y;

    // Clamp player to map bounds
    if (data->tilemap && data->tilemap->loaded) {
        float map_w = (float)(data->tilemap->width * data->tilemap->tilewidth);
        float map_h = (float)(data->tilemap->height * data->tilemap->tileheight);
        if (data->pos_x < 0) data->pos_x = 0;
        if (data->pos_y < 0) data->pos_y = 0;
        if (data->pos_x > map_w - 16) data->pos_x = map_w - 16;
        if (data->pos_y > map_h - 16) data->pos_y = map_h - 16;
        pbody->rect.x = data->pos_x;
        pbody->rect.y = data->pos_y;
    }

    // Check ramp overlaps for elevation transitions
    // Suppress re-triggering until the player leaves the ramp that last fired
    if (data->last_ramp >= 0) {
        ElevationRamp *lr = &data->ramps.ramps[data->last_ramp];
        if (!CheckCollisionRecs(pbody->rect, lr->rect)) {
            data->last_ramp = -1;
        }
    }
    if (data->last_ramp < 0) {
        for (int i = 0; i < data->ramps.count; i++) {
            ElevationRamp *ramp = &data->ramps.ramps[i];
            if (data->player_elevation != ramp->from_elevation) continue;
            if (CheckCollisionRecs(pbody->rect, ramp->rect)) {
                data->player_elevation = ramp->to_elevation;
                pbody->elevation = ramp->to_elevation;
                data->last_ramp = i;
                break;
            }
        }
    }

    // Camera follows player
    game->camera.target = (Vector2){ data->pos_x + 8, data->pos_y + 8 };
    game->camera.offset = (Vector2){ GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
}

static void overworld_draw(Game *game) {
    OverworldData *data = game->scene_data[SCENE_OVERWORLD];
    if (!data) return;

    ClearBackground(BLACK);
    BeginMode2D(game->camera);

    Color elevated_tint = (Color){ 255, 255, 255, 160 };

    for (int rl = 0; rl < RENDER_LAYER_COUNT; rl++) {
        // Draw tile layers with elevation-aware layering
        if (data->tilemap && data->tilemap->loaded) {
            for (int i = 0; i < data->tilemap->tile_layer_count; i++) {
                TileLayer *layer = &data->tilemap->tile_layers[i];
                RenderLayer layer_rl = render_layer_from_name(layer->render_layer);

                // Determine if this layer draws in the current render pass
                bool should_draw = false;
                bool use_elevated_tint = false;
                if (layer->elevation > data->player_elevation) {
                    if (rl == RENDER_LAYER_ABOVE_PLAYER) {
                        should_draw = true;
                        use_elevated_tint = true;
                    }
                } else if ((int)layer_rl == rl) {
                    should_draw = true;
                }
                if (!should_draw) continue;

                // Activate per-layer shader if tagged
                bool shader_active = false;
                if (layer->shader_name[0] && strcmp(layer->shader_name, "water") == 0) {
                    float t = (float)GetTime();
                    SetShaderValue(game->water_shader, game->water_time_loc, &t, SHADER_UNIFORM_FLOAT);
                    SetShaderValue(game->water_shader, game->water_cam_target_loc, &game->camera.target, SHADER_UNIFORM_VEC2);
                    SetShaderValue(game->water_shader, game->water_cam_offset_loc, &game->camera.offset, SHADER_UNIFORM_VEC2);
                    SetShaderValue(game->water_shader, game->water_cam_zoom_loc, &game->camera.zoom, SHADER_UNIFORM_FLOAT);
                    BeginShaderMode(game->water_shader);
                    shader_active = true;
                }

                if (use_elevated_tint) {
                    tilemap_draw_layer_tinted(data->tilemap, i, game->camera, elevated_tint);
                } else {
                    tilemap_draw_layer(data->tilemap, i, game->camera);
                }

                if (shader_active) EndShaderMode();
            }
        }

        // Draw player sprite at the correct render layer
        if (game->player_sprite && game->player_sprite->render_layer == rl) {
            sprite_draw(game->player_sprite, data->pos_x, data->pos_y - 16, WHITE);
        }
    }

    // Debug collision wireframes
    collision_debug_draw(data->collision_world);

    EndMode2D();

    // HUD
    DrawText("Arrows: move | 1: dungeon | 2: battle | F3: collisions | F4: +1hr | F5: torch | F6: reinit", 10, 10, 20, WHITE);
    DrawFPS(10, 40);

    char elev_buf[32];
    snprintf(elev_buf, sizeof(elev_buf), "Elevation: %d", data->player_elevation);
    DrawText(elev_buf, 10, 60, 20, YELLOW);

    // Time of day clock
    int hour = (int)game->time_of_day;
    int minute = (int)((game->time_of_day - hour) * 60.0f);
    char time_buf[16];
    snprintf(time_buf, sizeof(time_buf), "%02d:%02d", hour, minute);
    DrawText(time_buf, GetScreenWidth() - 80, 10, 20, WHITE);
}

SceneFuncs scene_overworld_funcs(void) {
    return (SceneFuncs){
        .init    = overworld_init,
        .cleanup = overworld_cleanup,
        .update  = overworld_update,
        .draw    = overworld_draw,
        .persistent = true,
    };
}
