#include "settings.h"
#include "game.h"
#include "audio.h"
#include "cJSON.h"
#include "raylib.h"
#include <stdio.h>
#include <string.h>

const Resolution RESOLUTIONS[RESOLUTION_COUNT] = {
    {  800,  600, "800 x 600"   },
    { 1024,  768, "1024 x 768"  },
    { 1280,  720, "1280 x 720"  },
    { 1366,  768, "1366 x 768"  },
    { 1600,  900, "1600 x 900"  },
    { 1920, 1080, "1920 x 1080" },
};

static const char *SETTINGS_PATH = "settings.json";

int settings_find_resolution_index(int width, int height) {
    for (int i = 0; i < RESOLUTION_COUNT; i++) {
        if (RESOLUTIONS[i].width == width && RESOLUTIONS[i].height == height) {
            return i;
        }
    }
    return 0;
}

void settings_load(Settings *settings) {
    // Defaults
    settings->screen_width = 800;
    settings->screen_height = 600;
    settings->resolution_index = 0;
    settings->music_volume = 0.5f;

    if (!FileExists(SETTINGS_PATH)) return;

    char *text = LoadFileText(SETTINGS_PATH);
    if (!text) return;

    cJSON *root = cJSON_Parse(text);
    UnloadFileText(text);
    if (!root) return;

    cJSON *w = cJSON_GetObjectItem(root, "screen_width");
    cJSON *h = cJSON_GetObjectItem(root, "screen_height");
    if (cJSON_IsNumber(w) && cJSON_IsNumber(h)) {
        settings->screen_width = w->valueint;
        settings->screen_height = h->valueint;
        settings->resolution_index = settings_find_resolution_index(
            settings->screen_width, settings->screen_height);
    }

    cJSON *vol = cJSON_GetObjectItem(root, "music_volume");
    if (cJSON_IsNumber(vol)) {
        settings->music_volume = (float)vol->valuedouble;
        if (settings->music_volume < 0.0f) settings->music_volume = 0.0f;
        if (settings->music_volume > 1.0f) settings->music_volume = 1.0f;
    }

    cJSON_Delete(root);
}

void settings_save(const Settings *settings) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "screen_width", settings->screen_width);
    cJSON_AddNumberToObject(root, "screen_height", settings->screen_height);
    cJSON_AddNumberToObject(root, "music_volume", settings->music_volume);

    char *json_str = cJSON_Print(root);
    cJSON_Delete(root);

    if (json_str) {
        FILE *f = fopen(SETTINGS_PATH, "w");
        if (f) {
            fputs(json_str, f);
            fclose(f);
        }
        cJSON_free(json_str);
    }
}

void settings_apply_resolution(Game *game) {
    int w = game->settings.screen_width;
    int h = game->settings.screen_height;
    SetWindowSize(w, h);
    game->camera.offset = (Vector2){ w / 2.0f, h / 2.0f };
}

void settings_apply_volume(Game *game) {
    if (game->audio) {
        audio_set_music_volume(game->audio, game->settings.music_volume);
    }
}
