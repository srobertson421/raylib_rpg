#ifndef GAME_H
#define GAME_H

#include "raylib.h"

typedef struct TileMap TileMap;
typedef struct CollisionWorld CollisionWorld;
typedef struct AnimatedSprite AnimatedSprite;

typedef struct GameState {
    float pos_x;
    float pos_y;
    float speed;
    Color color;
    Camera2D camera;
    TileMap *tilemap;
    CollisionWorld *collision_world;
    int player_body;
    AnimatedSprite *player_sprite;
    int facing;
    bool initialized;
} GameState;

typedef void (*GameInitFunc)(GameState *state);
typedef void (*GameUpdateFunc)(GameState *state);
typedef void (*GameDrawFunc)(GameState *state);

typedef struct GameAPI {
    GameInitFunc init;
    GameUpdateFunc update;
    GameDrawFunc draw;
} GameAPI;

#ifdef BUILD_GAME_DLL
  #define GAME_EXPORT __declspec(dllexport)
#else
  #define GAME_EXPORT __declspec(dllimport)
#endif

GAME_EXPORT GameAPI get_game_api(void);

#endif
