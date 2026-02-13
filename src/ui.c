#include "ui.h"
#include "game.h"
#include "settings.h"
#include "audio.h"
#include <stdio.h>

#define UI_OPEN_DURATION  0.25f
#define UI_CLOSE_DURATION 0.20f

#define PAUSE_ITEM_COUNT 3
#define SETTINGS_ROW_COUNT 3

static float ease_out_cubic(float t) {
    t -= 1.0f;
    return t * t * t + 1.0f;
}

void ui_init(UIOverlay *ui) {
    ui->state = UI_CLOSED;
    ui->anim_timer = 0;
    ui->page = UI_PAGE_PAUSE;
    ui->pause_cursor = 0;
    ui->settings_row = 0;
    ui->resolution_cursor = 0;
    ui->volume_value = 0.5f;
    ui->saved_volume = 0.5f;
}

void ui_open(UIOverlay *ui) {
    if (ui->state != UI_CLOSED) return;
    ui->state = UI_OPENING;
    ui->anim_timer = 0;
    ui->page = UI_PAGE_PAUSE;
    ui->pause_cursor = 0;
}

void ui_close(UIOverlay *ui) {
    if (ui->state != UI_OPEN) return;
    ui->state = UI_CLOSING;
    ui->anim_timer = 0;
}

bool ui_is_active(UIOverlay *ui) {
    return ui->state != UI_CLOSED;
}

// --- Pause page input ---

static void update_pause_page(UIOverlay *ui, Game *game) {
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        ui->pause_cursor--;
        if (ui->pause_cursor < 0) ui->pause_cursor = PAUSE_ITEM_COUNT - 1;
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        ui->pause_cursor++;
        if (ui->pause_cursor >= PAUSE_ITEM_COUNT) ui->pause_cursor = 0;
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        switch (ui->pause_cursor) {
        case 0: // Resume
            ui_close(ui);
            break;
        case 1: // Settings
            ui->page = UI_PAGE_SETTINGS;
            ui->settings_row = 0;
            ui->resolution_cursor = game->settings.resolution_index;
            ui->volume_value = game->settings.music_volume;
            ui->saved_volume = game->settings.music_volume;
            break;
        case 2: // Quit to Menu
            ui->state = UI_CLOSED;
            ui->anim_timer = 0;
            game->next_scene = SCENE_MENU;
            break;
        }
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        ui_close(ui);
    }
}

// --- Settings page input ---

static void update_settings_page(UIOverlay *ui, Game *game) {
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        ui->settings_row--;
        if (ui->settings_row < 0) ui->settings_row = SETTINGS_ROW_COUNT - 1;
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        ui->settings_row++;
        if (ui->settings_row >= SETTINGS_ROW_COUNT) ui->settings_row = 0;
    }

    if (ui->settings_row == 0) {
        // Volume
        if (IsKeyDown(KEY_LEFT)) {
            ui->volume_value -= 0.05f * (60.0f * GetFrameTime());
            if (ui->volume_value < 0.0f) ui->volume_value = 0.0f;
            if (game->audio) audio_set_music_volume(game->audio, ui->volume_value);
        }
        if (IsKeyDown(KEY_RIGHT)) {
            ui->volume_value += 0.05f * (60.0f * GetFrameTime());
            if (ui->volume_value > 1.0f) ui->volume_value = 1.0f;
            if (game->audio) audio_set_music_volume(game->audio, ui->volume_value);
        }
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            game->settings.music_volume = ui->volume_value;
            ui->saved_volume = ui->volume_value;
            settings_save(&game->settings);
        }
    } else if (ui->settings_row == 1) {
        // Resolution
        if (IsKeyPressed(KEY_LEFT)) {
            ui->resolution_cursor--;
            if (ui->resolution_cursor < 0) ui->resolution_cursor = RESOLUTION_COUNT - 1;
        }
        if (IsKeyPressed(KEY_RIGHT)) {
            ui->resolution_cursor++;
            if (ui->resolution_cursor >= RESOLUTION_COUNT) ui->resolution_cursor = 0;
        }
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            game->settings.screen_width = RESOLUTIONS[ui->resolution_cursor].width;
            game->settings.screen_height = RESOLUTIONS[ui->resolution_cursor].height;
            game->settings.resolution_index = ui->resolution_cursor;
            settings_apply_resolution(game);
            settings_save(&game->settings);
        }
    } else {
        // Back
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            // Revert unsaved volume
            if (game->audio) audio_set_music_volume(game->audio, ui->saved_volume);
            game->settings.music_volume = ui->saved_volume;
            ui->page = UI_PAGE_PAUSE;
        }
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        // Revert unsaved volume, return to pause page
        if (game->audio) audio_set_music_volume(game->audio, ui->saved_volume);
        game->settings.music_volume = ui->saved_volume;
        ui->page = UI_PAGE_PAUSE;
    }
}

// --- Update (returns true if overlay is active / scene should be paused) ---

bool ui_update(UIOverlay *ui, Game *game) {
    float dt = GetFrameTime();

    switch (ui->state) {
    case UI_CLOSED:
        return false;

    case UI_OPENING:
        ui->anim_timer += dt;
        if (ui->anim_timer >= UI_OPEN_DURATION) {
            ui->anim_timer = UI_OPEN_DURATION;
            ui->state = UI_OPEN;
        }
        return true;

    case UI_OPEN:
        if (ui->page == UI_PAGE_PAUSE) {
            update_pause_page(ui, game);
        } else {
            update_settings_page(ui, game);
        }
        return ui->state != UI_CLOSED; // Quit to Menu sets CLOSED instantly

    case UI_CLOSING:
        ui->anim_timer += dt;
        if (ui->anim_timer >= UI_CLOSE_DURATION) {
            ui->state = UI_CLOSED;
            return false;
        }
        return true;
    }

    return false;
}

// --- Draw helpers ---

static float get_anim_t(UIOverlay *ui) {
    switch (ui->state) {
    case UI_OPENING:
        return ease_out_cubic(ui->anim_timer / UI_OPEN_DURATION);
    case UI_OPEN:
        return 1.0f;
    case UI_CLOSING:
        return 1.0f - ease_out_cubic(ui->anim_timer / UI_CLOSE_DURATION);
    default:
        return 0.0f;
    }
}

static void draw_pause_page(UIOverlay *ui, float panel_x, float panel_y,
                             float panel_w, float panel_h, float content_alpha) {
    (void)panel_h;
    unsigned char ca = (unsigned char)(255 * content_alpha);

    const char *title = "PAUSED";
    int title_w = MeasureText(title, 36);
    DrawText(title, (int)(panel_x + panel_w / 2 - title_w / 2),
             (int)(panel_y + 24), 36, (Color){ 255, 255, 255, ca });

    const char *items[PAUSE_ITEM_COUNT] = { "Resume", "Settings", "Quit to Menu" };
    float item_start_y = panel_y + 80;
    float item_spacing = 44;

    for (int i = 0; i < PAUSE_ITEM_COUNT; i++) {
        float iy = item_start_y + i * item_spacing;
        Color c;
        if (i == ui->pause_cursor) {
            c = (Color){ 255, 255, 100, ca };
        } else {
            c = (Color){ 200, 200, 200, ca };
        }

        const char *prefix = (i == ui->pause_cursor) ? "> " : "  ";
        char line[64];
        snprintf(line, sizeof(line), "%s%s", prefix, items[i]);
        int line_w = MeasureText(line, 26);
        DrawText(line, (int)(panel_x + panel_w / 2 - line_w / 2),
                 (int)iy, 26, c);
    }

    // Help text
    const char *help = "UP/DOWN: Navigate   ENTER: Select   ESC: Resume";
    int help_w = MeasureText(help, 14);
    DrawText(help, (int)(panel_x + panel_w / 2 - help_w / 2),
             (int)(panel_y + panel_h - 30), 14, (Color){ 150, 150, 150, ca });
}

static void draw_settings_page(UIOverlay *ui, Game *game,
                                float panel_x, float panel_y,
                                float panel_w, float panel_h,
                                float content_alpha) {
    unsigned char ca = (unsigned char)(255 * content_alpha);

    const char *title = "SETTINGS";
    int title_w = MeasureText(title, 36);
    DrawText(title, (int)(panel_x + panel_w / 2 - title_w / 2),
             (int)(panel_y + 24), 36, (Color){ 255, 255, 255, ca });

    float row_y = panel_y + 80;
    float row_spacing = 60;

    // Row 0: Volume
    {
        Color label_c = (ui->settings_row == 0)
            ? (Color){ 255, 255, 100, ca }
            : (Color){ 200, 200, 200, ca };

        const char *prefix = (ui->settings_row == 0) ? "> " : "  ";
        char label[64];
        snprintf(label, sizeof(label), "%sMusic Volume", prefix);
        DrawText(label, (int)(panel_x + 30), (int)row_y, 22, label_c);

        // Volume bar
        float bar_w = 180.0f;
        float bar_h = 14.0f;
        float bar_x = panel_x + panel_w - bar_w - 60;
        float bar_y_pos = row_y + 4;

        DrawRectangle((int)bar_x, (int)bar_y_pos, (int)bar_w, (int)bar_h,
                      (Color){ 60, 60, 60, ca });
        DrawRectangle((int)bar_x, (int)bar_y_pos,
                      (int)(bar_w * ui->volume_value), (int)bar_h, label_c);
        DrawRectangleLines((int)bar_x, (int)bar_y_pos, (int)bar_w, (int)bar_h,
                           (Color){ 255, 255, 255, ca });

        char pct[16];
        snprintf(pct, sizeof(pct), "%d%%", (int)(ui->volume_value * 100));
        DrawText(pct, (int)(bar_x + bar_w + 8), (int)bar_y_pos, 14, label_c);
    }

    // Row 1: Resolution
    {
        Color label_c = (ui->settings_row == 1)
            ? (Color){ 255, 255, 100, ca }
            : (Color){ 200, 200, 200, ca };

        float ry = row_y + row_spacing;
        const char *prefix = (ui->settings_row == 1) ? "> " : "  ";
        char label[64];
        snprintf(label, sizeof(label), "%sResolution", prefix);
        DrawText(label, (int)(panel_x + 30), (int)ry, 22, label_c);

        // Current selection
        const char *res_label = RESOLUTIONS[ui->resolution_cursor].label;
        int res_w = MeasureText(res_label, 22);
        float res_x = panel_x + panel_w - 60 - res_w;
        DrawText(res_label, (int)res_x, (int)ry, 22, label_c);

        // Active marker
        if (ui->resolution_cursor == game->settings.resolution_index) {
            DrawText("*", (int)(res_x + res_w + 6), (int)ry, 22,
                     (Color){ 100, 255, 100, ca });
        }

        // Arrow hints
        if (ui->settings_row == 1) {
            DrawText("<", (int)(res_x - 20), (int)ry, 22, label_c);
            DrawText(">", (int)(res_x + res_w + (ui->resolution_cursor == game->settings.resolution_index ? 24 : 6)),
                     (int)ry, 22, label_c);
        }
    }

    // Row 2: Back
    {
        Color label_c = (ui->settings_row == 2)
            ? (Color){ 255, 255, 100, ca }
            : (Color){ 200, 200, 200, ca };

        float ry = row_y + row_spacing * 2;
        const char *prefix = (ui->settings_row == 2) ? "> " : "  ";
        char label[64];
        snprintf(label, sizeof(label), "%sBack", prefix);
        int label_w = MeasureText(label, 22);
        DrawText(label, (int)(panel_x + panel_w / 2 - label_w / 2),
                 (int)ry, 22, label_c);
    }

    // Help text
    const char *help = "UP/DOWN: Row   LEFT/RIGHT: Adjust   ENTER: Apply   ESC: Back";
    int help_w = MeasureText(help, 14);
    DrawText(help, (int)(panel_x + panel_w / 2 - help_w / 2),
             (int)(panel_y + panel_h - 30), 14, (Color){ 150, 150, 150, ca });
}

// --- Main draw ---

void ui_draw(UIOverlay *ui, Game *game) {
    if (ui->state == UI_CLOSED) return;

    float anim_t = get_anim_t(ui);
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();

    // Backdrop
    unsigned char backdrop_alpha = (unsigned char)(150.0f * anim_t);
    DrawRectangle(0, 0, (int)sw, (int)sh, (Color){ 0, 0, 0, backdrop_alpha });

    // Panel dimensions (scale from center)
    float full_w = 420.0f;
    float full_h = 320.0f;
    float panel_w = full_w * anim_t;
    float panel_h = full_h * anim_t;
    float panel_x = (sw - panel_w) / 2.0f;
    float panel_y = (sh - panel_h) / 2.0f;

    // Panel background
    Rectangle panel_rect = { panel_x, panel_y, panel_w, panel_h };
    DrawRectangleRounded(panel_rect, 0.05f, 8, (Color){ 30, 20, 50, 230 });

    // Panel border
    DrawRectangleRoundedLinesEx(panel_rect, 0.05f, 8, 2.0f,
                                 (Color){ 120, 80, 160, 200 });

    // Content fades in during last 15% of open animation
    float content_alpha;
    if (ui->state == UI_OPENING) {
        float threshold = 0.85f;
        float raw_t = ui->anim_timer / UI_OPEN_DURATION;
        if (raw_t < threshold) {
            content_alpha = 0.0f;
        } else {
            content_alpha = (raw_t - threshold) / (1.0f - threshold);
        }
    } else if (ui->state == UI_CLOSING) {
        content_alpha = 0.0f;
    } else {
        content_alpha = 1.0f;
    }

    if (content_alpha <= 0.01f) return;

    if (ui->page == UI_PAGE_PAUSE) {
        draw_pause_page(ui, panel_x, panel_y, panel_w, panel_h, content_alpha);
    } else {
        draw_settings_page(ui, game, panel_x, panel_y, panel_w, panel_h, content_alpha);
    }
}
