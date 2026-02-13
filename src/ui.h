#ifndef UI_H
#define UI_H

#include "raylib.h"
#include <stdbool.h>

typedef enum UIOverlayState {
    UI_CLOSED,
    UI_OPENING,
    UI_OPEN,
    UI_CLOSING,
} UIOverlayState;

typedef enum UIPage {
    UI_PAGE_PAUSE,
    UI_PAGE_SETTINGS,
} UIPage;

typedef struct UIOverlay {
    UIOverlayState state;
    float anim_timer;

    UIPage page;

    // Pause page
    int pause_cursor;       // 0=Resume, 1=Settings, 2=Quit to Menu

    // Settings page
    int settings_row;       // 0=Volume, 1=Resolution, 2=Back
    int resolution_cursor;
    float volume_value;     // working copy (live preview)
    float saved_volume;     // snapshot for revert on ESC/Back
} UIOverlay;

typedef struct Game Game;

void ui_init(UIOverlay *ui);
void ui_open(UIOverlay *ui);
void ui_close(UIOverlay *ui);
bool ui_update(UIOverlay *ui, Game *game);
void ui_draw(UIOverlay *ui, Game *game);
bool ui_is_active(UIOverlay *ui);

#endif
