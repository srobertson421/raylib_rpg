#include "game.h"
#include "scene.h"
#include "tilemap.h"
#include "collision.h"
#include "sprite.h"
#include <stdlib.h>
#include <math.h>

typedef struct OverworldData {
    TileMap *tilemap;
    CollisionWorld *collision_world;
    int player_body;
    float pos_x, pos_y;
} OverworldData;

static void overworld_init(Game *game) {
    OverworldData *data = calloc(1, sizeof(OverworldData));
    game->scene_data[SCENE_OVERWORLD] = data;

    // Reset player HP to full when entering overworld
    game->player_hp = game->player_max_hp;

    data->tilemap = tilemap_load("../assets/overworld.tmj");

    // Player start position (center of map)
    if (data->tilemap && data->tilemap->loaded) {
        data->pos_x = (data->tilemap->width * data->tilemap->tilewidth) / 2.0f;
        data->pos_y = (data->tilemap->height * data->tilemap->tileheight) / 2.0f;
    } else {
        data->pos_x = 400.0f;
        data->pos_y = 300.0f;
    }

    // Collision setup
    data->collision_world = collision_create();
    if (data->tilemap && data->tilemap->loaded) {
        collision_load_from_tilemap(data->collision_world, data->tilemap, "objects_collision");
    }
    data->player_body = collision_add_body(
        data->collision_world,
        (Rectangle){ data->pos_x, data->pos_y, 16, 16 },
        BODY_KINEMATIC, TAG_PLAYER, NULL
    );

    // Point camera at player
    game->camera.target = (Vector2){ data->pos_x, data->pos_y };
    game->camera.offset = (Vector2){ GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
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

    // Camera follows player
    game->camera.target = (Vector2){ data->pos_x + 8, data->pos_y + 8 };
    game->camera.offset = (Vector2){ GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
}

static void overworld_draw(Game *game) {
    OverworldData *data = game->scene_data[SCENE_OVERWORLD];
    if (!data) return;

    ClearBackground(BLACK);
    BeginMode2D(game->camera);

    for (int rl = 0; rl < RENDER_LAYER_COUNT; rl++) {
        // Draw tile layers matching this render layer
        if (data->tilemap && data->tilemap->loaded) {
            for (int i = 0; i < data->tilemap->tile_layer_count; i++) {
                RenderLayer layer_rl = render_layer_from_name(data->tilemap->tile_layers[i].render_layer);
                if ((int)layer_rl == rl) {
                    tilemap_draw_layer(data->tilemap, i, game->camera);
                }
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
    DrawText("Arrows: move | 1: dungeon | 2: battle | F3: collisions | F6: reinit", 10, 10, 20, WHITE);
    DrawFPS(10, 40);
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
