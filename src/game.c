#ifndef BUILD_GAME_DLL
#define BUILD_GAME_DLL
#endif
#include "game.h"

static void game_init(GameState *state) {
    state->pos_x = 375;
    state->pos_y = 275;
    state->speed = 4;
    state->color = (Color){ 255, 0, 0, 255 };
    state->initialized = true;
}

static void game_update(GameState *state) {
    if (IsKeyDown(KEY_RIGHT)) state->pos_x += state->speed;
    if (IsKeyDown(KEY_LEFT))  state->pos_x -= state->speed;
    if (IsKeyDown(KEY_DOWN))  state->pos_y += state->speed;
    if (IsKeyDown(KEY_UP))    state->pos_y -= state->speed;

    if (state->pos_x < 0)   state->pos_x = 0;
    if (state->pos_x > 750)  state->pos_x = 750;
    if (state->pos_y < 0)   state->pos_y = 0;
    if (state->pos_y > 550)  state->pos_y = 550;
}

static void game_draw(GameState *state) {
    ClearBackground(RAYWHITE);
    DrawRectangle(state->pos_x, state->pos_y, 50, 50, state->color);
    DrawText("Arrows: move | F5: reload | F6: reload+reinit", 10, 10, 20, DARKGRAY);
    DrawFPS(720, 10);
}

GAME_EXPORT GameAPI get_game_api(void) {
    GameAPI api = {0};
    api.init = game_init;
    api.update = game_update;
    api.draw = game_draw;
    return api;
}
