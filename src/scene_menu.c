#include "game.h"
#include "scene.h"

static void menu_init(Game *game) {
    (void)game;
}

static void menu_cleanup(Game *game) {
    (void)game;
}

static void menu_update(Game *game) {
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        game->next_scene = SCENE_OVERWORLD;
    }
}

static void menu_draw(Game *game) {
    (void)game;
    ClearBackground((Color){ 20, 20, 40, 255 });

    const char *title = "RAYLIB ADVENTURE";
    int title_width = MeasureText(title, 40);
    DrawText(title, (GetScreenWidth() - title_width) / 2, GetScreenHeight() / 2 - 60, 40, WHITE);

    const char *prompt = "Press ENTER to start";
    int prompt_width = MeasureText(prompt, 20);
    DrawText(prompt, (GetScreenWidth() - prompt_width) / 2, GetScreenHeight() / 2 + 20, 20, LIGHTGRAY);
}

SceneFuncs scene_menu_funcs(void) {
    return (SceneFuncs){
        .init    = menu_init,
        .cleanup = menu_cleanup,
        .update  = menu_update,
        .draw    = menu_draw,
        .persistent = false,
    };
}
