#include "game.h"
#include "scene.h"
#include <stdlib.h>

typedef enum MenuItem {
    MENU_START_GAME,
    MENU_SETTINGS,
    MENU_ITEM_COUNT,
} MenuItem;

static const char *MENU_LABELS[MENU_ITEM_COUNT] = {
    "Start Game",
    "Settings",
};

typedef struct MenuData {
    int selected;
} MenuData;

static void menu_init(Game *game) {
    MenuData *data = calloc(1, sizeof(MenuData));
    data->selected = 0;
    game->scene_data[SCENE_MENU] = data;
}

static void menu_cleanup(Game *game) {
    free(game->scene_data[SCENE_MENU]);
    game->scene_data[SCENE_MENU] = NULL;
}

static void menu_update(Game *game) {
    MenuData *data = game->scene_data[SCENE_MENU];

    if (IsKeyPressed(KEY_UP)) {
        data->selected--;
        if (data->selected < 0) data->selected = MENU_ITEM_COUNT - 1;
    }
    if (IsKeyPressed(KEY_DOWN)) {
        data->selected++;
        if (data->selected >= MENU_ITEM_COUNT) data->selected = 0;
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        switch (data->selected) {
            case MENU_START_GAME:
                game->next_scene = SCENE_OVERWORLD;
                break;
            case MENU_SETTINGS:
                game->next_scene = SCENE_SETTINGS;
                break;
        }
    }
}

static void menu_draw(Game *game) {
    MenuData *data = game->scene_data[SCENE_MENU];
    ClearBackground((Color){ 20, 20, 40, 255 });

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    const char *title = "RAYLIB ADVENTURE";
    int title_width = MeasureText(title, 40);
    DrawText(title, (sw - title_width) / 2, sh / 2 - 80, 40, WHITE);

    int start_y = sh / 2 + 10;
    int line_height = 35;

    for (int i = 0; i < MENU_ITEM_COUNT; i++) {
        int y = start_y + i * line_height;
        const char *label = MENU_LABELS[i];
        int label_width = MeasureText(label, 24);
        int x = (sw - label_width) / 2;

        Color color = LIGHTGRAY;
        if (i == data->selected) {
            color = YELLOW;
            DrawText(">", x - 30, y, 24, YELLOW);
        }

        DrawText(label, x, y, 24, color);
    }
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
