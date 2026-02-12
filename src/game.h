#ifndef GAME_H
#define GAME_H

#include "raylib.h"
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
    SCENE_COUNT
} SceneID;

typedef struct TileMap TileMap;
typedef struct CollisionWorld CollisionWorld;
typedef struct AnimatedSprite AnimatedSprite;

typedef struct Game {
    // Global state (persists across scenes)
    AnimatedSprite *player_sprite;
    float speed;
    Color color;
    int facing;
    Camera2D camera;
    bool initialized;

    // Scene management
    SceneID current_scene;
    SceneID next_scene;
    void *scene_data[SCENE_COUNT];
} Game;

typedef void (*GameInitFunc)(Game *game);
typedef void (*GameUpdateFunc)(Game *game);
typedef void (*GameDrawFunc)(Game *game);

typedef struct GameAPI {
    GameInitFunc init;
    GameUpdateFunc update;
    GameDrawFunc draw;
} GameAPI;

RenderLayer render_layer_from_name(const char *name);

#ifdef BUILD_GAME_DLL
  #define GAME_EXPORT __declspec(dllexport)
#else
  #define GAME_EXPORT __declspec(dllimport)
#endif

GAME_EXPORT GameAPI get_game_api(void);

#endif
