#include "game.h"
#include "scene.h"
#include "sprite.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// ---------- enums & constants ----------

typedef enum BattlePhase {
    PHASE_INTRO,
    PHASE_PLAYER_MENU,
    PHASE_PLAYER_ATTACK_ANIM,
    PHASE_RESOLVE_PLAYER_ATTACK,
    PHASE_ENEMY_ATTACK_ANIM,
    PHASE_RESOLVE_ENEMY_ATTACK,
    PHASE_WIN,
    PHASE_LOSE,
} BattlePhase;

typedef enum TimingResult {
    TIMING_NONE,
    TIMING_MISS,
    TIMING_GOOD,
    TIMING_EXCELLENT,
} TimingResult;

// Timing windows (normalized anim_t range)
#define TIMING_WINDOW_START 0.75f
#define TIMING_WINDOW_END   0.92f
#define TIMING_EXCELLENT_START 0.83f

#define INTRO_DURATION   1.5f
#define RESOLVE_DURATION 1.2f
#define ANIM_SPEED       1.8f   // anim_t units per second (~0.55s lunge)

#define PLAYER_BASE_HP     30
#define PLAYER_BASE_ATTACK  5
#define ENEMY_BASE_HP      20
#define ENEMY_BASE_ATTACK   4

#define SPRITE_SCALE 4.0f

// ---------- data ----------

typedef struct Combatant {
    char name[32];
    int hp;
    int max_hp;
    int base_attack;
    float rest_x, rest_y;   // home position (screen-space)
    float cur_x, cur_y;     // current draw position
} Combatant;

typedef struct FloatingText {
    char text[32];
    Color color;
    float x, y;
    float timer;     // counts down
    float lifetime;
} FloatingText;

#define MAX_FLOATS 8

typedef struct BattleData {
    BattlePhase phase;
    float phase_timer;

    Combatant player;
    Combatant enemy;

    // animation
    float anim_t;          // 0→1 lunge progress
    bool timing_pressed;
    TimingResult timing_result;
    bool show_hint;

    // menu
    int menu_cursor;       // 0 = Attack, 1 = Defend
    bool player_chose_defend;

    // resolve display
    float resolve_timer;

    // floating damage/text
    FloatingText floats[MAX_FLOATS];
    int float_count;

    // save/restore sprite state for battle drawing
    int saved_anim;
    int saved_frame;
    float saved_timer;
    bool saved_playing;
} BattleData;

// ---------- helpers ----------

static float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

static float clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static Color hp_color(int hp, int max_hp) {
    float ratio = (float)hp / (float)max_hp;
    int r = (int)(255 * (1.0f - ratio));
    int g = (int)(255 * ratio);
    return (Color){ (unsigned char)r, (unsigned char)g, 40, 255 };
}

static void add_float_text(BattleData *bd, const char *text, float x, float y, Color color) {
    if (bd->float_count >= MAX_FLOATS) {
        // shift out oldest
        for (int i = 0; i < MAX_FLOATS - 1; i++) bd->floats[i] = bd->floats[i + 1];
        bd->float_count = MAX_FLOATS - 1;
    }
    FloatingText *ft = &bd->floats[bd->float_count++];
    snprintf(ft->text, sizeof(ft->text), "%s", text);
    ft->color = color;
    ft->x = x;
    ft->y = y;
    ft->timer = 1.2f;
    ft->lifetime = 1.2f;
}

static void update_floats(BattleData *bd, float dt) {
    for (int i = 0; i < bd->float_count; ) {
        FloatingText *ft = &bd->floats[i];
        ft->timer -= dt;
        ft->y -= 40.0f * dt;  // drift up
        if (ft->timer <= 0) {
            bd->floats[i] = bd->floats[--bd->float_count];
        } else {
            i++;
        }
    }
}

static TimingResult evaluate_timing(float anim_t) {
    if (anim_t >= TIMING_EXCELLENT_START && anim_t <= TIMING_WINDOW_END) {
        return TIMING_EXCELLENT;
    }
    if (anim_t >= TIMING_WINDOW_START && anim_t < TIMING_EXCELLENT_START) {
        return TIMING_GOOD;
    }
    return TIMING_MISS;
}

static const char *timing_label(TimingResult r) {
    switch (r) {
        case TIMING_EXCELLENT: return "EXCELLENT!";
        case TIMING_GOOD:      return "GOOD!";
        case TIMING_MISS:      return "MISS";
        default:               return "";
    }
}

static Color timing_color(TimingResult r) {
    switch (r) {
        case TIMING_EXCELLENT: return GOLD;
        case TIMING_GOOD:      return GREEN;
        case TIMING_MISS:      return GRAY;
        default:               return WHITE;
    }
}

static int calc_damage(int base_attack, TimingResult r) {
    switch (r) {
        case TIMING_EXCELLENT: return base_attack * 2;
        case TIMING_GOOD:      return (int)(base_attack * 1.5f);
        default:               return base_attack;
    }
}

static int calc_retaliation_damage(int raw_dmg, TimingResult r) {
    switch (r) {
        case TIMING_EXCELLENT: return raw_dmg / 2;
        case TIMING_GOOD:      return raw_dmg * 3 / 4;
        default:               return raw_dmg;
    }
}

// ---------- phase transitions ----------

static void enter_phase(BattleData *bd, BattlePhase phase) {
    bd->phase = phase;
    bd->phase_timer = 0;
    bd->anim_t = 0;
    bd->timing_pressed = false;
    bd->timing_result = TIMING_NONE;
    bd->show_hint = false;
}

// ---------- init / cleanup ----------

static void battle_init(Game *game) {
    BattleData *bd = calloc(1, sizeof(BattleData));
    game->scene_data[SCENE_BATTLE] = bd;

    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    float ground_y = sh * 0.6f;

    // Player on left
    bd->player.rest_x = sw * 0.20f;
    bd->player.rest_y = ground_y;
    bd->player.cur_x = bd->player.rest_x;
    bd->player.cur_y = bd->player.rest_y;
    snprintf(bd->player.name, sizeof(bd->player.name), "Player");
    bd->player.hp = game->player_hp;
    bd->player.max_hp = game->player_max_hp;
    bd->player.base_attack = game->player_attack;

    // Enemy on right
    bd->enemy.rest_x = sw * 0.70f;
    bd->enemy.rest_y = ground_y;
    bd->enemy.cur_x = bd->enemy.rest_x;
    bd->enemy.cur_y = bd->enemy.rest_y;
    snprintf(bd->enemy.name, sizeof(bd->enemy.name), "Goblin");
    bd->enemy.hp = ENEMY_BASE_HP;
    bd->enemy.max_hp = ENEMY_BASE_HP;
    bd->enemy.base_attack = ENEMY_BASE_ATTACK;

    enter_phase(bd, PHASE_INTRO);
}

static void battle_cleanup(Game *game) {
    BattleData *bd = game->scene_data[SCENE_BATTLE];
    if (!bd) return;
    free(bd);
    game->scene_data[SCENE_BATTLE] = NULL;
}

// ---------- update ----------

static void battle_update(Game *game) {
    BattleData *bd = game->scene_data[SCENE_BATTLE];
    if (!bd) return;

    float dt = GetFrameTime();
    bd->phase_timer += dt;
    update_floats(bd, dt);

    switch (bd->phase) {

    case PHASE_INTRO:
        if (bd->phase_timer >= INTRO_DURATION) {
            enter_phase(bd, PHASE_PLAYER_MENU);
        }
        break;

    case PHASE_PLAYER_MENU:
        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))   bd->menu_cursor = 0;
        if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))  bd->menu_cursor = 1;

        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
            if (bd->menu_cursor == 0) {
                // Attack
                bd->player_chose_defend = false;
                enter_phase(bd, PHASE_PLAYER_ATTACK_ANIM);
            } else {
                // Defend — skip to enemy attack with defend opportunity
                bd->player_chose_defend = true;
                enter_phase(bd, PHASE_ENEMY_ATTACK_ANIM);
            }
        }

        // ESC to flee
        if (IsKeyPressed(KEY_ESCAPE)) {
            game->next_scene = SCENE_OVERWORLD;
        }
        break;

    case PHASE_PLAYER_ATTACK_ANIM: {
        bd->anim_t += ANIM_SPEED * dt;
        bd->anim_t = clampf(bd->anim_t, 0, 1.0f);

        // Lerp player toward enemy
        bd->player.cur_x = lerpf(bd->player.rest_x, bd->enemy.rest_x - 40.0f, bd->anim_t);

        // Show hint during timing window
        bd->show_hint = (bd->anim_t >= TIMING_WINDOW_START && bd->anim_t <= TIMING_WINDOW_END);

        // Check for timing press
        if (!bd->timing_pressed && IsKeyPressed(KEY_SPACE)) {
            bd->timing_pressed = true;
            bd->timing_result = evaluate_timing(bd->anim_t);
        }

        // Animation complete
        if (bd->anim_t >= 1.0f) {
            if (!bd->timing_pressed) {
                bd->timing_result = TIMING_MISS;
            }
            // Apply damage
            int dmg = calc_damage(bd->player.base_attack, bd->timing_result);
            bd->enemy.hp -= dmg;
            if (bd->enemy.hp < 0) bd->enemy.hp = 0;

            // Floating text
            char buf[32];
            snprintf(buf, sizeof(buf), "%d", dmg);
            add_float_text(bd, buf, bd->enemy.cur_x + 20, bd->enemy.cur_y - 30, timing_color(bd->timing_result));
            add_float_text(bd, timing_label(bd->timing_result), bd->enemy.cur_x - 10, bd->enemy.cur_y - 60, timing_color(bd->timing_result));

            // Slide player back
            bd->player.cur_x = bd->player.rest_x;

            enter_phase(bd, PHASE_RESOLVE_PLAYER_ATTACK);
        }
    } break;

    case PHASE_RESOLVE_PLAYER_ATTACK:
        if (bd->phase_timer >= RESOLVE_DURATION) {
            if (bd->enemy.hp <= 0) {
                enter_phase(bd, PHASE_WIN);
            } else {
                // Enemy retaliates — no defend opportunity
                bd->player_chose_defend = false;
                enter_phase(bd, PHASE_ENEMY_ATTACK_ANIM);
            }
        }
        break;

    case PHASE_ENEMY_ATTACK_ANIM: {
        bd->anim_t += ANIM_SPEED * dt;
        bd->anim_t = clampf(bd->anim_t, 0, 1.0f);

        // Lerp enemy toward player
        bd->enemy.cur_x = lerpf(bd->enemy.rest_x, bd->player.rest_x + 40.0f, bd->anim_t);

        // Show timing hint only during retaliation (not when defending)
        if (!bd->player_chose_defend) {
            bd->show_hint = (bd->anim_t >= TIMING_WINDOW_START && bd->anim_t <= TIMING_WINDOW_END);

            if (!bd->timing_pressed && IsKeyPressed(KEY_SPACE)) {
                bd->timing_pressed = true;
                bd->timing_result = evaluate_timing(bd->anim_t);
            }
        }

        // Animation complete
        if (bd->anim_t >= 1.0f) {
            int raw_dmg = bd->enemy.base_attack;
            int final_dmg;

            if (bd->player_chose_defend) {
                // Defend: flat 50% reduction, no timing
                final_dmg = raw_dmg / 2;
                bd->timing_result = TIMING_NONE;
            } else {
                // Retaliation: timing-based reduction
                if (!bd->timing_pressed) bd->timing_result = TIMING_MISS;
                final_dmg = calc_retaliation_damage(raw_dmg, bd->timing_result);
            }

            bd->player.hp -= final_dmg;
            if (bd->player.hp < 0) bd->player.hp = 0;

            // Floating text
            char buf[32];
            snprintf(buf, sizeof(buf), "%d", final_dmg);
            if (bd->player_chose_defend) {
                add_float_text(bd, buf, bd->player.cur_x + 20, bd->player.cur_y - 30, GREEN);
                add_float_text(bd, "BLOCKED", bd->player.cur_x - 10, bd->player.cur_y - 60, GREEN);
            } else {
                add_float_text(bd, buf, bd->player.cur_x + 20, bd->player.cur_y - 30, timing_color(bd->timing_result));
                add_float_text(bd, timing_label(bd->timing_result), bd->player.cur_x - 10, bd->player.cur_y - 60, timing_color(bd->timing_result));
            }

            // Slide enemy back
            bd->enemy.cur_x = bd->enemy.rest_x;

            enter_phase(bd, PHASE_RESOLVE_ENEMY_ATTACK);
        }
    } break;

    case PHASE_RESOLVE_ENEMY_ATTACK:
        if (bd->phase_timer >= RESOLVE_DURATION) {
            if (bd->player.hp <= 0) {
                enter_phase(bd, PHASE_LOSE);
            } else {
                enter_phase(bd, PHASE_PLAYER_MENU);
            }
        }
        break;

    case PHASE_WIN:
    case PHASE_LOSE:
        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) {
            game->player_hp = bd->player.hp;
            game->next_scene = SCENE_OVERWORLD;
        }
        break;
    }
}

// ---------- draw ----------

static void draw_hp_bar(float x, float y, float width, float height, int hp, int max_hp) {
    float ratio = (float)hp / (float)max_hp;
    DrawRectangle((int)x, (int)y, (int)width, (int)height, DARKGRAY);
    DrawRectangle((int)x, (int)y, (int)(width * ratio), (int)height, hp_color(hp, max_hp));
    DrawRectangleLines((int)x, (int)y, (int)width, (int)height, WHITE);

    char buf[32];
    snprintf(buf, sizeof(buf), "%d/%d", hp, max_hp);
    int text_w = MeasureText(buf, 16);
    DrawText(buf, (int)(x + width / 2 - text_w / 2), (int)(y + 2), 16, WHITE);
}

static void draw_combatant_sprite(AnimatedSprite *sprite, BattleData *bd,
                                  float x, float y, int anim_index) {
    // Save current sprite state
    bd->saved_anim = sprite->current_animation;
    bd->saved_frame = sprite->current_frame;
    bd->saved_timer = sprite->frame_timer;
    bd->saved_playing = sprite->playing;

    // Set to desired animation for drawing
    sprite->current_animation = anim_index;
    sprite->current_frame = 0;
    sprite->playing = false;

    sprite_draw_ex(sprite, x, y, SPRITE_SCALE, WHITE);

    // Restore
    sprite->current_animation = bd->saved_anim;
    sprite->current_frame = bd->saved_frame;
    sprite->frame_timer = bd->saved_timer;
    sprite->playing = bd->saved_playing;
}

static void battle_draw(Game *game) {
    BattleData *bd = game->scene_data[SCENE_BATTLE];
    if (!bd) return;

    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    float ground_y = sh * 0.6f;

    ClearBackground((Color){ 15, 10, 25, 255 });

    // Ground line
    DrawRectangle(0, (int)(ground_y + 32 * SPRITE_SCALE), (int)sw, (int)(sh - ground_y), (Color){ 30, 25, 40, 255 });
    DrawLine(0, (int)(ground_y + 32 * SPRITE_SCALE), (int)sw, (int)(ground_y + 32 * SPRITE_SCALE), (Color){ 60, 50, 80, 255 });

    // Draw combatant sprites
    if (game->player_sprite) {
        // Player faces right (anim index 1 = walk_right)
        draw_combatant_sprite(game->player_sprite, bd,
                              bd->player.cur_x, bd->player.cur_y, 1);

        // Enemy faces left (anim index 3 = walk_left)
        draw_combatant_sprite(game->player_sprite, bd,
                              bd->enemy.cur_x, bd->enemy.cur_y, 3);
    }

    // Names
    int player_name_w = MeasureText(bd->player.name, 20);
    DrawText(bd->player.name,
             (int)(bd->player.rest_x + 16 * SPRITE_SCALE / 2 - player_name_w / 2),
             (int)(bd->player.rest_y - 30), 20, WHITE);
    int enemy_name_w = MeasureText(bd->enemy.name, 20);
    DrawText(bd->enemy.name,
             (int)(bd->enemy.rest_x + 16 * SPRITE_SCALE / 2 - enemy_name_w / 2),
             (int)(bd->enemy.rest_y - 30), 20, RED);

    // HP bars (below each combatant)
    float bar_w = 120.0f;
    float bar_h = 18.0f;
    float bar_offset_y = 32 * SPRITE_SCALE + 10;
    draw_hp_bar(bd->player.rest_x + 16 * SPRITE_SCALE / 2 - bar_w / 2,
                bd->player.rest_y + bar_offset_y,
                bar_w, bar_h, bd->player.hp, bd->player.max_hp);
    draw_hp_bar(bd->enemy.rest_x + 16 * SPRITE_SCALE / 2 - bar_w / 2,
                bd->enemy.rest_y + bar_offset_y,
                bar_w, bar_h, bd->enemy.hp, bd->enemy.max_hp);

    // Timing hint "!" above the relevant combatant
    if (bd->show_hint) {
        float hint_x, hint_y;
        if (bd->phase == PHASE_PLAYER_ATTACK_ANIM) {
            hint_x = bd->player.cur_x + 16 * SPRITE_SCALE / 2;
            hint_y = bd->player.cur_y - 50;
        } else {
            // Defend: hint appears above player (they're pressing)
            hint_x = bd->player.rest_x + 16 * SPRITE_SCALE / 2;
            hint_y = bd->player.rest_y - 50;
        }
        int bang_w = MeasureText("!", 36);
        DrawText("!", (int)(hint_x - bang_w / 2), (int)hint_y, 36, YELLOW);
    }

    // Floating texts
    for (int i = 0; i < bd->float_count; i++) {
        FloatingText *ft = &bd->floats[i];
        float alpha = ft->timer / ft->lifetime;
        Color c = ft->color;
        c.a = (unsigned char)(255 * alpha);
        int tw = MeasureText(ft->text, 24);
        DrawText(ft->text, (int)(ft->x - tw / 2), (int)ft->y, 24, c);
    }

    // Phase-specific UI
    switch (bd->phase) {
    case PHASE_INTRO: {
        const char *msg = "BATTLE START!";
        int msg_w = MeasureText(msg, 40);
        float flash = sinf(bd->phase_timer * 8.0f) * 0.5f + 0.5f;
        Color col = (Color){ 255, 255, 100, (unsigned char)(200 + 55 * flash) };
        DrawText(msg, (int)(sw / 2 - msg_w / 2), (int)(sh * 0.3f), 40, col);
    } break;

    case PHASE_PLAYER_MENU: {
        // Menu box at bottom-left
        float box_x = 30;
        float box_y = sh - 140;
        float box_w = 220;
        float box_h = 110;
        DrawRectangle((int)box_x, (int)box_y, (int)box_w, (int)box_h, (Color){ 0, 0, 0, 200 });
        DrawRectangleLines((int)box_x, (int)box_y, (int)box_w, (int)box_h, WHITE);

        const char *options[] = { "Attack", "Defend" };
        for (int i = 0; i < 2; i++) {
            Color c = (i == bd->menu_cursor) ? YELLOW : WHITE;
            const char *prefix = (i == bd->menu_cursor) ? "> " : "  ";
            char line[64];
            snprintf(line, sizeof(line), "%s%s", prefix, options[i]);
            DrawText(line, (int)(box_x + 15), (int)(box_y + 15 + i * 40), 28, c);
        }

        DrawText("UP/DOWN: select | SPACE: confirm | ESC: flee",
                 (int)(box_x), (int)(sh - 22), 16, GRAY);
    } break;

    case PHASE_PLAYER_ATTACK_ANIM:
    case PHASE_ENEMY_ATTACK_ANIM: {
        // Show "SPACE" prompt during timing window
        if (bd->show_hint && !bd->timing_pressed) {
            const char *prompt = "[SPACE]";
            int pw = MeasureText(prompt, 20);
            DrawText(prompt, (int)(sw / 2 - pw / 2), (int)(sh - 50), 20, YELLOW);
        }
        // Show result if already pressed
        if (bd->timing_pressed) {
            const char *label = timing_label(bd->timing_result);
            int lw = MeasureText(label, 28);
            DrawText(label, (int)(sw / 2 - lw / 2), (int)(sh * 0.35f), 28, timing_color(bd->timing_result));
        }
    } break;

    case PHASE_RESOLVE_PLAYER_ATTACK:
    case PHASE_RESOLVE_ENEMY_ATTACK:
        // Just show floating texts (already drawn above)
        break;

    case PHASE_WIN: {
        const char *msg = "YOU WIN!";
        int msg_w = MeasureText(msg, 48);
        DrawText(msg, (int)(sw / 2 - msg_w / 2), (int)(sh * 0.3f), 48, GOLD);
        const char *sub = "Press any key to continue";
        int sub_w = MeasureText(sub, 20);
        DrawText(sub, (int)(sw / 2 - sub_w / 2), (int)(sh * 0.3f + 60), 20, WHITE);
    } break;

    case PHASE_LOSE: {
        const char *msg = "DEFEATED...";
        int msg_w = MeasureText(msg, 48);
        DrawText(msg, (int)(sw / 2 - msg_w / 2), (int)(sh * 0.3f), 48, RED);
        const char *sub = "Press any key to continue";
        int sub_w = MeasureText(sub, 20);
        DrawText(sub, (int)(sw / 2 - sub_w / 2), (int)(sh * 0.3f + 60), 20, WHITE);
    } break;
    }

    // HUD
    DrawText("BATTLE | ESC: flee (menu only) | F6: reinit", 10, 10, 20, WHITE);
    DrawFPS(10, 40);
}

// ---------- factory ----------

SceneFuncs scene_battle_funcs(void) {
    return (SceneFuncs){
        .init    = battle_init,
        .cleanup = battle_cleanup,
        .update  = battle_update,
        .draw    = battle_draw,
        .persistent = false,
    };
}
