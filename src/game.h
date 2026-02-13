#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "settings.h"
#include "ui.h"
#include "inventory.h"
#include <stdbool.h>

typedef enum RenderLayer {
    RENDER_LAYER_GROUND,
    RENDER_LAYER_BELOW_PLAYER,
    RENDER_LAYER_PLAYER,
    RENDER_LAYER_ABOVE_PLAYER,
    RENDER_LAYER_COUNT
} RenderLayer;

typedef enum SceneID {
    SCENE_NONE = -1,
    SCENE_MENU,
    SCENE_OVERWORLD,
    SCENE_DUNGEON_1,
    SCENE_SETTINGS,
    SCENE_BATTLE,
    SCENE_COUNT
} SceneID;

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

typedef struct TileMap TileMap;
typedef struct CollisionWorld CollisionWorld;
typedef struct AnimatedSprite AnimatedSprite;
typedef struct EventBus EventBus;
typedef struct AudioManager AudioManager;

typedef struct Game {
    // Global state (persists across scenes)
    AnimatedSprite *player_sprite;
    float speed;
    Color color;
    int facing;
    Camera2D camera;
    bool initialized;

    // Event system
    EventBus *events;

    // Audio
    AudioManager *audio;

    // Player stats (persist across scenes)
    int player_hp;
    int player_max_hp;
    int player_attack;

    // Settings
    Settings settings;

    // UI overlay
    UIOverlay ui;

    // Inventory
    Inventory inventory;
    Texture2D item_icon;    // sack.png, loaded once

    // Day/night cycle
    RenderTexture2D render_target;     // scene draws here, then post-processed to screen
    Shader daynight_shader;            // post-process fragment shader
    int daynight_time_loc;             // cached uniform location for time_of_day
    float time_of_day;                 // 0.0–24.0 (hours), wraps
    float time_speed;                  // game-minutes per real-second
    bool daynight_active;              // true when current scene uses the shader

    // Torch light
    bool torch_active;                 // debug toggle for torch light (F5)
    int light_pos_loc;                 // cached uniform: light_pos (vec2, screen pixels)
    int light_radius_loc;             // cached uniform: light_radius (float, screen pixels)
    int screen_size_loc;              // cached uniform: screen_size (vec2)

    // Per-layer shaders
    Shader water_shader;
    int water_time_loc;            // uniform: time (float, seconds)
    int water_cam_target_loc;      // uniform: camera_target (vec2)
    int water_cam_offset_loc;      // uniform: camera_offset (vec2)
    int water_cam_zoom_loc;        // uniform: camera_zoom (float)

    // Scene management
    SceneID current_scene;
    SceneID next_scene;
    void *scene_data[SCENE_COUNT];
} Game;

RenderLayer render_layer_from_name(const char *name);

void game_init(Game *game);
void game_update(Game *game);
void game_draw(Game *game);
void game_cleanup(Game *game);

#endif
