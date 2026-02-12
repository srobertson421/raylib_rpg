#ifndef BUILD_GAME_DLL
#define BUILD_GAME_DLL
#endif
#include "game.h"
#include "tilemap.h"
#include <stddef.h>

static void game_init(GameState *state) {
    // Clean up existing tilemap on reinit (F6)
    if (state->tilemap) {
        tilemap_unload(state->tilemap);
        state->tilemap = NULL;
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

    // Camera setup
    state->camera.target = (Vector2){ state->pos_x, state->pos_y };
    state->camera.offset = (Vector2){ GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
    state->camera.rotation = 0.0f;
    state->camera.zoom = 2.0f;

    state->initialized = true;
}

static void game_update(GameState *state) {
    // Player movement
    if (IsKeyDown(KEY_RIGHT)) state->pos_x += state->speed;
    if (IsKeyDown(KEY_LEFT))  state->pos_x -= state->speed;
    if (IsKeyDown(KEY_DOWN))  state->pos_y += state->speed;
    if (IsKeyDown(KEY_UP))    state->pos_y -= state->speed;

    // Clamp player to map bounds
    if (state->tilemap && state->tilemap->loaded) {
        float map_w = (float)(state->tilemap->width * state->tilemap->tilewidth);
        float map_h = (float)(state->tilemap->height * state->tilemap->tileheight);
        if (state->pos_x < 0) state->pos_x = 0;
        if (state->pos_y < 0) state->pos_y = 0;
        if (state->pos_x > map_w - 16) state->pos_x = map_w - 16;
        if (state->pos_y > map_h - 16) state->pos_y = map_h - 16;
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

        // Draw player
        DrawRectangle((int)state->pos_x, (int)state->pos_y, 16, 16, state->color);

        // Draw layers above player: upper_objects
        for (int i = 4; i < state->tilemap->tile_layer_count; i++) {
            tilemap_draw_layer(state->tilemap, i, state->camera);
        }
    } else {
        DrawRectangle((int)state->pos_x, (int)state->pos_y, 16, 16, state->color);
    }

    EndMode2D();

    // HUD (screen space, not affected by camera)
    DrawText("Arrows: move | F5: reload | F6: reload+reinit", 10, 10, 20, WHITE);
    DrawFPS(10, 40);
}

GAME_EXPORT GameAPI get_game_api(void) {
    GameAPI api = {0};
    api.init = game_init;
    api.update = game_update;
    api.draw = game_draw;
    return api;
}
