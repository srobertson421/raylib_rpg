#ifndef BUILD_GAME_DLL
#define BUILD_GAME_DLL
#endif
#include "game.h"
#include "tilemap.h"
#include "collision.h"
#include "sprite.h"
#include <stddef.h>
#include <math.h>

static void game_init(GameState *state) {
    // Clean up existing tilemap on reinit (F6)
    if (state->tilemap) {
        tilemap_unload(state->tilemap);
        state->tilemap = NULL;
    }

    // Clean up existing collision world on reinit (F6)
    if (state->collision_world) {
        collision_destroy(state->collision_world);
        state->collision_world = NULL;
    }

    // Clean up existing sprite on reinit (F6)
    if (state->player_sprite) {
        sprite_destroy(state->player_sprite);
        state->player_sprite = NULL;
    }

    state->tilemap = tilemap_load("../assets/overworld.tmj");

    // Player start position (center of map, in pixels)
    if (state->tilemap && state->tilemap->loaded) {
        state->pos_x = (state->tilemap->width * state->tilemap->tilewidth) / 2.0f;
        state->pos_y = (state->tilemap->height * state->tilemap->tileheight) / 2.0f;
    } else {
        state->pos_x = 400.0f;
        state->pos_y = 300.0f;
    }

    state->speed = 3.0f;
    state->color = (Color){ 230, 41, 55, 255 };

    // Collision setup
    state->collision_world = collision_create();
    if (state->tilemap && state->tilemap->loaded) {
        collision_load_from_tilemap(state->collision_world, state->tilemap, "objects_collision");
    }
    state->player_body = collision_add_body(
        state->collision_world,
        (Rectangle){ state->pos_x, state->pos_y, 16, 16 },
        BODY_KINEMATIC, TAG_PLAYER, NULL
    );

    // Camera setup
    state->camera.target = (Vector2){ state->pos_x, state->pos_y };
    state->camera.offset = (Vector2){ GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
    state->camera.rotation = 0.0f;
    state->camera.zoom = 2.0f;

    // Player sprite setup
    state->player_sprite = sprite_create("../assets/player.png", 16, 32);
    sprite_add_animation(state->player_sprite, "walk_down",  0, 4, 0, 8.0f, true);
    sprite_add_animation(state->player_sprite, "walk_right", 4, 4, 0, 8.0f, true);
    sprite_add_animation(state->player_sprite, "walk_up",    8, 4, 0, 8.0f, true);
    sprite_add_animation(state->player_sprite, "walk_left", 12, 4, 0, 8.0f, true);
    state->facing = 0;
    sprite_play(state->player_sprite, 0);
    sprite_stop(state->player_sprite);

    state->initialized = true;
}

static void game_update(GameState *state) {
    // Toggle debug draw
    if (IsKeyPressed(KEY_F3)) {
        state->collision_world->debug_draw = !state->collision_world->debug_draw;
    }

    // Player movement
    float dx = 0, dy = 0;
    if (IsKeyDown(KEY_RIGHT)) dx += state->speed;
    if (IsKeyDown(KEY_LEFT))  dx -= state->speed;
    if (IsKeyDown(KEY_DOWN))  dy += state->speed;
    if (IsKeyDown(KEY_UP))    dy -= state->speed;

    // Normalize diagonal movement
    if (dx != 0 && dy != 0) {
        float inv_len = state->speed / sqrtf(dx * dx + dy * dy);
        dx *= inv_len;
        dy *= inv_len;
    }

    // Animation direction
    bool moving = (dx != 0 || dy != 0);
    if (moving) {
        if (fabsf(dx) >= fabsf(dy)) {
            state->facing = (dx > 0) ? 1 : 3;
        } else {
            state->facing = (dy > 0) ? 0 : 2;
        }
        sprite_play(state->player_sprite, state->facing);
    } else {
        sprite_stop(state->player_sprite);
    }
    sprite_update(state->player_sprite, GetFrameTime());

    // Move with collision
    collision_move_and_slide(state->collision_world, state->player_body, dx, dy);
    CollisionBody *pbody = &state->collision_world->bodies[state->player_body];
    state->pos_x = pbody->rect.x;
    state->pos_y = pbody->rect.y;

    // Clamp player to map bounds
    if (state->tilemap && state->tilemap->loaded) {
        float map_w = (float)(state->tilemap->width * state->tilemap->tilewidth);
        float map_h = (float)(state->tilemap->height * state->tilemap->tileheight);
        if (state->pos_x < 0) state->pos_x = 0;
        if (state->pos_y < 0) state->pos_y = 0;
        if (state->pos_x > map_w - 16) state->pos_x = map_w - 16;
        if (state->pos_y > map_h - 16) state->pos_y = map_h - 16;
        // Sync clamped position back to body
        pbody->rect.x = state->pos_x;
        pbody->rect.y = state->pos_y;
    }

    // Camera follows player
    state->camera.target = (Vector2){ state->pos_x + 8, state->pos_y + 8 };
    state->camera.offset = (Vector2){ GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
}

static void game_draw(GameState *state) {
    ClearBackground(BLACK);

    BeginMode2D(state->camera);

    if (state->tilemap && state->tilemap->loaded) {
        // Draw layers below player: grass, water, paths, lower_objects
        for (int i = 0; i < state->tilemap->tile_layer_count && i < 4; i++) {
            tilemap_draw_layer(state->tilemap, i, state->camera);
        }

        // Draw player (sprite is 16x32, offset up so feet align with 16x16 collision rect)
        sprite_draw(state->player_sprite, state->pos_x, state->pos_y - 16, WHITE);

        // Draw layers above player: upper_objects
        for (int i = 4; i < state->tilemap->tile_layer_count; i++) {
            tilemap_draw_layer(state->tilemap, i, state->camera);
        }
    } else {
        sprite_draw(state->player_sprite, state->pos_x, state->pos_y - 16, WHITE);
    }

    // Debug collision wireframes
    collision_debug_draw(state->collision_world);

    EndMode2D();

    // HUD (screen space, not affected by camera)
    DrawText("Arrows: move | F3: collisions | F5: reload | F6: reinit", 10, 10, 20, WHITE);
    DrawFPS(10, 40);
}

GAME_EXPORT GameAPI get_game_api(void) {
    GameAPI api = {0};
    api.init = game_init;
    api.update = game_update;
    api.draw = game_draw;
    return api;
}
