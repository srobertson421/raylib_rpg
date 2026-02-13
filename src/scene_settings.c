#include "game.h"
#include "scene.h"
#include "settings.h"
#include <stdlib.h>

typedef struct SettingsData {
    int selected_index;
} SettingsData;

static void settings_init(Game *game) {
    SettingsData *data = calloc(1, sizeof(SettingsData));
    data->selected_index = game->settings.resolution_index;
    game->scene_data[SCENE_SETTINGS] = data;
}

static void settings_cleanup(Game *game) {
    free(game->scene_data[SCENE_SETTINGS]);
    game->scene_data[SCENE_SETTINGS] = NULL;
}

static void settings_update(Game *game) {
    SettingsData *data = game->scene_data[SCENE_SETTINGS];

    if (IsKeyPressed(KEY_UP)) {
        data->selected_index--;
        if (data->selected_index < 0) data->selected_index = RESOLUTION_COUNT - 1;
    }
    if (IsKeyPressed(KEY_DOWN)) {
        data->selected_index++;
        if (data->selected_index >= RESOLUTION_COUNT) data->selected_index = 0;
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        game->settings.screen_width = RESOLUTIONS[data->selected_index].width;
        game->settings.screen_height = RESOLUTIONS[data->selected_index].height;
        game->settings.resolution_index = data->selected_index;
        settings_apply_resolution(game);
        settings_save(&game->settings);
        game->next_scene = SCENE_MENU;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        game->next_scene = SCENE_MENU;
    }
}

static void settings_draw(Game *game) {
    SettingsData *data = game->scene_data[SCENE_SETTINGS];
    ClearBackground((Color){ 20, 20, 40, 255 });

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    const char *title = "SETTINGS";
    int title_width = MeasureText(title, 40);
    DrawText(title, (sw - title_width) / 2, sh / 2 - 160, 40, WHITE);

    const char *subtitle = "Resolution";
    int subtitle_width = MeasureText(subtitle, 20);
    DrawText(subtitle, (sw - subtitle_width) / 2, sh / 2 - 100, 20, LIGHTGRAY);

    int start_y = sh / 2 - 60;
    int line_height = 30;
    int active_index = game->settings.resolution_index;

    for (int i = 0; i < RESOLUTION_COUNT; i++) {
        int y = start_y + i * line_height;
        const char *label = RESOLUTIONS[i].label;
        int label_width = MeasureText(label, 20);
        int x = (sw - label_width) / 2;

        Color color = LIGHTGRAY;
        if (i == data->selected_index) color = YELLOW;
        if (i == active_index && i != data->selected_index) color = GREEN;

        if (i == data->selected_index) {
            DrawText(">", x - 25, y, 20, YELLOW);
        }
        if (i == active_index) {
            int star_x = x + label_width + 10;
            DrawText("*", star_x, y, 20, GREEN);
        }

        DrawText(label, x, y, 20, color);
    }

    int help_y = start_y + RESOLUTION_COUNT * line_height + 30;
    const char *help = "UP/DOWN: Select   ENTER: Apply   ESC: Cancel";
    int help_width = MeasureText(help, 16);
    DrawText(help, (sw - help_width) / 2, help_y, 16, GRAY);
}

SceneFuncs scene_settings_funcs(void) {
    return (SceneFuncs){
        .init    = settings_init,
        .cleanup = settings_cleanup,
        .update  = settings_update,
        .draw    = settings_draw,
        .persistent = false,
    };
}
