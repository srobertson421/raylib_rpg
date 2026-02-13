#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "settings.h"
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
