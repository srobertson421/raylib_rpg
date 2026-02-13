#ifndef SETTINGS_H
#define SETTINGS_H

typedef struct Game Game;

typedef struct Resolution {
    int width;
    int height;
    const char *label;
} Resolution;

#define RESOLUTION_COUNT 6
extern const Resolution RESOLUTIONS[RESOLUTION_COUNT];

typedef struct Settings {
    int screen_width;
    int screen_height;
    int resolution_index;
} Settings;

void settings_load(Settings *settings);
void settings_save(const Settings *settings);
void settings_apply_resolution(Game *game);
int settings_find_resolution_index(int width, int height);

#endif
