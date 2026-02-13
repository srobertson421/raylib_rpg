#ifndef SCENE_H
#define SCENE_H

#include "game.h"
#include <stdbool.h>

typedef void (*SceneInitFunc)(Game *game);
typedef void (*SceneCleanupFunc)(Game *game);
typedef void (*SceneUpdateFunc)(Game *game);
typedef void (*SceneDrawFunc)(Game *game);

typedef struct SceneFuncs {
    SceneInitFunc init;
    SceneCleanupFunc cleanup;
    SceneUpdateFunc update;
    SceneDrawFunc draw;
    bool persistent;
} SceneFuncs;

SceneFuncs scene_menu_funcs(void);
SceneFuncs scene_overworld_funcs(void);
SceneFuncs scene_dungeon1_funcs(void);
SceneFuncs scene_settings_funcs(void);

#endif
