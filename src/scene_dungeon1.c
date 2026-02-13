#include "game.h"
#include "scene.h"
#include "sprite.h"
#include <stdlib.h>
#include <math.h>

typedef struct Dungeon1Data {
    float pos_x, pos_y;
} Dungeon1Data;

static void dungeon1_init(Game *game) {
    Dungeon1Data *data = calloc(1, sizeof(Dungeon1Data));
    game->scene_data[SCENE_DUNGEON_1] = data;

    data->pos_x = GetScreenWidth() / 4.0f;
    data->pos_y = GetScreenHeight() / 4.0f;

    game->camera.target = (Vector2){ data->pos_x, data->pos_y };
    game->camera.offset = (Vector2){ GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
}

static void dungeon1_cleanup(Game *game) {
    Dungeon1Data *data = game->scene_data[SCENE_DUNGEON_1];
    if (!data) return;
    free(data);
    game->scene_data[SCENE_DUNGEON_1] = NULL;
}

static void dungeon1_update(Game *game) {
    Dungeon1Data *data = game->scene_data[SCENE_DUNGEON_1];
    if (!data) return;

    // Return to overworld
    if (IsKeyPressed(KEY_ONE)) {
        game->next_scene = SCENE_OVERWORLD;
        return;
    }

    // Simple movement without collision
    float dx = 0, dy = 0;
    if (IsKeyDown(KEY_RIGHT)) dx += game->speed;
    if (IsKeyDown(KEY_LEFT))  dx -= game->speed;
    if (IsKeyDown(KEY_DOWN))  dy += game->speed;
    if (IsKeyDown(KEY_UP))    dy -= game->speed;

    if (dx != 0 && dy != 0) {
        float inv_len = game->speed / sqrtf(dx * dx + dy * dy);
        dx *= inv_len;
        dy *= inv_len;
    }

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

    data->pos_x += dx;
    data->pos_y += dy;

    game->camera.target = (Vector2){ data->pos_x + 8, data->pos_y + 8 };
    game->camera.offset = (Vector2){ GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
}

static void dungeon1_draw(Game *game) {
    Dungeon1Data *data = game->scene_data[SCENE_DUNGEON_1];
    if (!data) return;

    ClearBackground((Color){ 10, 10, 15, 255 });
    BeginMode2D(game->camera);

    // Draw player sprite
    if (game->player_sprite) {
        sprite_draw(game->player_sprite, data->pos_x, data->pos_y - 16, WHITE);
    }

    EndMode2D();

    // HUD
    DrawText("DUNGEON 1 | 1: return to overworld | F6: reinit", 10, 10, 20, WHITE);
    DrawFPS(10, 40);
}

SceneFuncs scene_dungeon1_funcs(void) {
    return (SceneFuncs){
        .init    = dungeon1_init,
        .cleanup = dungeon1_cleanup,
        .update  = dungeon1_update,
        .draw    = dungeon1_draw,
        .persistent = false,
    };
}
