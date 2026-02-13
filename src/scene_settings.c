#include "game.h"
#include "scene.h"
#include "settings.h"
#include "audio.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct SettingsData {
    int selected_index;   // resolution cursor
    float volume_value;   // working copy of music volume
    int active_row;       // 0 = resolution, 1 = volume
} SettingsData;

static void settings_init(Game *game) {
    SettingsData *data = calloc(1, sizeof(SettingsData));
    data->selected_index = game->settings.resolution_index;
    data->volume_value = game->settings.music_volume;
    data->active_row = 0;
    game->scene_data[SCENE_SETTINGS] = data;
}

static void settings_cleanup(Game *game) {
    SettingsData *data = game->scene_data[SCENE_SETTINGS];
    if (data) {
        // Revert live volume preview to saved value
        if (game->audio) {
            audio_set_music_volume(game->audio, game->settings.music_volume);
        }
    }
    free(game->scene_data[SCENE_SETTINGS]);
    game->scene_data[SCENE_SETTINGS] = NULL;
}

static void settings_update(Game *game) {
    SettingsData *data = game->scene_data[SCENE_SETTINGS];

    if (data->active_row == 0) {
        // Resolution row navigation
        if (IsKeyPressed(KEY_UP)) {
            data->selected_index--;
            if (data->selected_index < 0) data->selected_index = RESOLUTION_COUNT - 1;
        }
        if (IsKeyPressed(KEY_DOWN)) {
            data->selected_index++;
            if (data->selected_index >= RESOLUTION_COUNT) {
                // Wrap to volume row
                data->active_row = 1;
                data->selected_index = RESOLUTION_COUNT - 1;
            }
        }

        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            game->settings.screen_width = RESOLUTIONS[data->selected_index].width;
            game->settings.screen_height = RESOLUTIONS[data->selected_index].height;
            game->settings.resolution_index = data->selected_index;
            settings_apply_resolution(game);
            settings_save(&game->settings);
        }
    } else {
        // Volume row
        if (IsKeyPressed(KEY_UP)) {
            data->active_row = 0;
        }

        if (IsKeyDown(KEY_LEFT)) {
            data->volume_value -= 0.05f * (60.0f * GetFrameTime());
            if (data->volume_value < 0.0f) data->volume_value = 0.0f;
            // Live preview
            if (game->audio) audio_set_music_volume(game->audio, data->volume_value);
        }
        if (IsKeyDown(KEY_RIGHT)) {
            data->volume_value += 0.05f * (60.0f * GetFrameTime());
            if (data->volume_value > 1.0f) data->volume_value = 1.0f;
            // Live preview
            if (game->audio) audio_set_music_volume(game->audio, data->volume_value);
        }

        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            game->settings.music_volume = data->volume_value;
            settings_save(&game->settings);
        }
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        // Revert live preview to saved value
        if (game->audio) {
            audio_set_music_volume(game->audio, game->settings.music_volume);
        }
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
        if (data->active_row == 0 && i == data->selected_index) color = YELLOW;
        if (i == active_index && !(data->active_row == 0 && i == data->selected_index)) color = GREEN;

        if (data->active_row == 0 && i == data->selected_index) {
            DrawText(">", x - 25, y, 20, YELLOW);
        }
        if (i == active_index) {
            int star_x = x + label_width + 10;
            DrawText("*", star_x, y, 20, GREEN);
        }

        DrawText(label, x, y, 20, color);
    }

    // Volume row
    int vol_y = start_y + RESOLUTION_COUNT * line_height + 20;

    const char *vol_label = "Music Volume";
    int vol_label_w = MeasureText(vol_label, 20);
    DrawText(vol_label, (sw - vol_label_w) / 2, vol_y, 20,
             data->active_row == 1 ? YELLOW : LIGHTGRAY);

    // Volume bar
    float bar_w = 200.0f;
    float bar_h = 16.0f;
    float bar_x = (sw - bar_w) / 2.0f;
    float bar_y = (float)(vol_y + 28);
    DrawRectangle((int)bar_x, (int)bar_y, (int)bar_w, (int)bar_h, DARKGRAY);
    DrawRectangle((int)bar_x, (int)bar_y, (int)(bar_w * data->volume_value), (int)bar_h,
                  data->active_row == 1 ? YELLOW : LIGHTGRAY);
    DrawRectangleLines((int)bar_x, (int)bar_y, (int)bar_w, (int)bar_h, WHITE);

    // Percentage
    char pct_buf[16];
    snprintf(pct_buf, sizeof(pct_buf), "%d%%", (int)(data->volume_value * 100));
    DrawText(pct_buf, (int)(bar_x + bar_w + 10), (int)bar_y, 16,
             data->active_row == 1 ? YELLOW : LIGHTGRAY);

    if (data->active_row == 1) {
        DrawText(">", (int)(bar_x - 25), (int)bar_y, 16, YELLOW);
    }

    int help_y = (int)bar_y + 40;
    const char *help = "UP/DOWN: Select   LEFT/RIGHT: Volume   ENTER: Apply   ESC: Cancel";
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
