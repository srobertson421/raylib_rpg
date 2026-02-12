#include "raylib.h"
#include "game.h"
#include <stdio.h>

int main(void) {
    InitWindow(800, 600, "raylib game");
    SetTargetFPS(60);

    Game state = {0};
    game_init(&state);

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_F6)) {
            printf("[main] Reinit (F6).\n");
            game_init(&state);
        }

        game_update(&state);

        BeginDrawing();
        game_draw(&state);
        EndDrawing();
    }

    game_cleanup(&state);
    CloseWindow();
    return 0;
}
