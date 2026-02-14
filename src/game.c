#include "game.h"
#include "scene.h"
#include "sprite.h"
#include "event.h"
#include "audio.h"
#include "settings.h"
#include "ui.h"
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static const char *RENDER_LAYER_NAMES[RENDER_LAYER_COUNT] = {
    [RENDER_LAYER_GROUND]       = "ground",
    [RENDER_LAYER_BELOW_PLAYER] = "below_player",
    [RENDER_LAYER_PLAYER]       = "player",
    [RENDER_LAYER_ABOVE_PLAYER] = "above_player",
};

RenderLayer render_layer_from_name(const char *name) {
    if (!name || !name[0]) return RENDER_LAYER_GROUND;
    for (int i = 0; i < RENDER_LAYER_COUNT; i++) {
        if (strcmp(name, RENDER_LAYER_NAMES[i]) == 0) return (RenderLayer)i;
    }
    return RENDER_LAYER_GROUND;
}

static SceneFuncs scene_table[SCENE_COUNT];
static bool scene_table_built = false;

static void build_scene_table(void) {
    if (scene_table_built) return;
    scene_table[SCENE_MENU]      = scene_menu_funcs();
    scene_table[SCENE_OVERWORLD] = scene_overworld_funcs();
    scene_table[SCENE_DUNGEON_1] = scene_dungeon1_funcs();
    scene_table[SCENE_SETTINGS]  = scene_settings_funcs();
    scene_table[SCENE_BATTLE]    = scene_battle_funcs();
    scene_table_built = true;
}

static void perform_transition(Game *game) {
    SceneID next = game->next_scene;
    if (next == SCENE_NONE) return;
    game->next_scene = SCENE_NONE;

    // Clean up old scene if not persistent
    SceneID old = game->current_scene;
    if (old >= 0 && old < SCENE_COUNT) {
        SceneFuncs *old_funcs = &scene_table[old];
        if (!old_funcs->persistent && old_funcs->cleanup) {
            old_funcs->cleanup(game);
        }
    }

    // Discard stale events from old scene
    event_clear(game->events);

    // Init new scene if no data exists yet
    game->current_scene = next;
    SceneFuncs *new_funcs = &scene_table[next];
    if (new_funcs->init) {
        bool has_data = (game->scene_data[next] != NULL);
        if (!has_data || !new_funcs->persistent) {
            new_funcs->init(game);
        }
    }

    // Notify listeners (audio etc.) about the scene change
    event_emit(game->events, (Event){
        .type = EVT_SCENE_ENTER,
        .entity_id = (int)next,
    });
}

void game_init(Game *game) {
    build_scene_table();

    // Clean up ALL scenes (for F6 reinit)
    for (int i = 0; i < SCENE_COUNT; i++) {
        if (game->scene_data[i] && scene_table[i].cleanup) {
            game->current_scene = (SceneID)i;
            scene_table[i].cleanup(game);
        }
        game->scene_data[i] = NULL;
    }

    // Clean up existing player sprite on reinit
    if (game->player_sprite) {
        sprite_destroy(game->player_sprite);
        game->player_sprite = NULL;
    }

    // Create fresh event bus (destroy old one on reinit)
    if (game->events) {
        event_bus_destroy(game->events);
    }
    game->events = event_bus_create();

    // Create fresh audio manager (destroy old one on reinit)
    if (game->audio) {
        audio_destroy(game->audio);
    }
    game->audio = audio_create();

    // Subscribe audio to event bus (before any scene transitions)
    audio_subscribe_events(game->audio, game->events);

    // Load and apply settings
    settings_load(&game->settings);
    settings_apply_resolution(game);
    settings_apply_volume(game);

    // Global player sprite setup
    game->player_sprite = sprite_create("../assets/player.png", 16, 32);
    sprite_add_animation(game->player_sprite, "walk_down",  0, 4, 0, 8.0f, true);
    sprite_add_animation(game->player_sprite, "walk_right", 4, 4, 0, 8.0f, true);
    sprite_add_animation(game->player_sprite, "walk_up",    8, 4, 0, 8.0f, true);
    sprite_add_animation(game->player_sprite, "walk_left", 12, 4, 0, 8.0f, true);
    game->player_sprite->render_layer = RENDER_LAYER_PLAYER;
    game->facing = 0;
    sprite_play(game->player_sprite, 0);
    sprite_stop(game->player_sprite);

    game->player_hp = 30;
    game->player_max_hp = 30;
    game->player_attack = 5;

    game->speed = 3.0f;
    game->color = (Color){ 230, 41, 55, 255 };

    // Camera setup
    game->camera.target = (Vector2){ 400, 300 };
    game->camera.offset = (Vector2){ GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
    game->camera.rotation = 0.0f;
    game->camera.zoom = 2.0f;

    // Reset UI overlay
    ui_init(&game->ui);

    // Inventory
    inventory_init(&game->inventory);
    if (game->item_icon.id != 0) UnloadTexture(game->item_icon);
    game->item_icon = LoadTexture("../assets/sack.png");

    // Day/night cycle
    if (game->render_target.id != 0) UnloadRenderTexture(game->render_target);
    if (game->daynight_shader.id != 0) UnloadShader(game->daynight_shader);
    game->render_target = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    game->daynight_shader = LoadShader(NULL, "../assets/daynight.fs");
    game->daynight_time_loc = GetShaderLocation(game->daynight_shader, "time_of_day");
    game->light_pos_loc = GetShaderLocation(game->daynight_shader, "light_pos");
    game->light_radius_loc = GetShaderLocation(game->daynight_shader, "light_radius");
    game->screen_size_loc = GetShaderLocation(game->daynight_shader, "screen_size");
    game->time_of_day = 8.0f;
    game->time_speed = 1.0f;
    game->daynight_active = false;
    game->torch_active = false;

    // Cloud shadow uniforms
    game->cloud_intensity_loc = GetShaderLocation(game->daynight_shader, "cloud_intensity");
    game->cloud_offset_loc = GetShaderLocation(game->daynight_shader, "cloud_offset");
    game->cloud_scale_loc = GetShaderLocation(game->daynight_shader, "cloud_scale");
    game->daynight_cam_target_loc = GetShaderLocation(game->daynight_shader, "camera_target");
    game->daynight_cam_offset_loc = GetShaderLocation(game->daynight_shader, "camera_offset");
    game->daynight_cam_zoom_loc = GetShaderLocation(game->daynight_shader, "camera_zoom");

    // Initialize cloud animation with random speed
    game->cloud_offset_x = 0.0f;
    game->cloud_offset_y = 0.0f;
    game->cloud_speed_x = 15.0f + (float)(rand() % 10);   // 15-25 world units/sec
    game->cloud_speed_y = 2.0f + (float)(rand() % 5);     // 2-7 world units/sec

    // Water shader
    if (game->water_shader.id != 0) UnloadShader(game->water_shader);
    game->water_shader = LoadShader(NULL, "../assets/water.fs");
    game->water_time_loc = GetShaderLocation(game->water_shader, "time");
    game->water_cam_target_loc = GetShaderLocation(game->water_shader, "camera_target");
    game->water_cam_offset_loc = GetShaderLocation(game->water_shader, "camera_offset");
    game->water_cam_zoom_loc = GetShaderLocation(game->water_shader, "camera_zoom");
    game->water_screen_size_loc = GetShaderLocation(game->water_shader, "screen_size");

    // Reflection shader
    if (game->reflection_shader.id != 0) UnloadShader(game->reflection_shader);
    game->reflection_shader = LoadShader(NULL, "../assets/reflection.fs");
    game->reflection_time_loc = GetShaderLocation(game->reflection_shader, "time");

    game->current_scene = SCENE_NONE;
    game->next_scene = SCENE_MENU;
    game->initialized = true;
}

void game_update(Game *game) {
    if (game->next_scene != SCENE_NONE) {
        perform_transition(game);
    }

    // Advance day/night clock (always ticks, even during overlay)
    game->time_of_day += game->time_speed * GetFrameTime() / 60.0f;
    if (game->time_of_day >= 24.0f) game->time_of_day -= 24.0f;

    // Animate cloud offset
    game->cloud_offset_x += game->cloud_speed_x * GetFrameTime();
    game->cloud_offset_y += game->cloud_speed_y * GetFrameTime();

    // F5: toggle torch light (debug)
    if (IsKeyPressed(KEY_F5)) {
        game->torch_active = !game->torch_active;
    }

    // F4: fast-forward time by +1 hour (debug)
    if (IsKeyPressed(KEY_F4)) {
        game->time_of_day += 1.0f;
        if (game->time_of_day >= 24.0f) game->time_of_day -= 24.0f;
    }

    // Determine if current scene uses the day/night shader
    game->daynight_active = (game->current_scene == SCENE_OVERWORLD ||
                              game->current_scene == SCENE_DUNGEON_1);

    // ESC opens pause overlay (skip on menu and settings scenes)
    if (!ui_is_active(&game->ui) && IsKeyPressed(KEY_ESCAPE)
        && game->current_scene != SCENE_MENU
        && game->current_scene != SCENE_SETTINGS) {
        ui_open(&game->ui);
    }

    // I opens inventory overlay (skip on menu and settings scenes)
    if (!ui_is_active(&game->ui) && IsKeyPressed(KEY_I)
        && game->current_scene != SCENE_MENU
        && game->current_scene != SCENE_SETTINGS) {
        ui_open_inventory(&game->ui);
    }

    if (ui_is_active(&game->ui)) {
        ui_update(&game->ui, game);
    } else if (game->current_scene >= 0 && game->current_scene < SCENE_COUNT) {
        SceneFuncs *funcs = &scene_table[game->current_scene];
        if (funcs->update) funcs->update(game);
    }

    event_flush(game->events);
    audio_update(game->audio);
}

void game_draw(Game *game) {
    if (game->daynight_active && game->current_scene >= 0 && game->current_scene < SCENE_COUNT) {
        // Draw scene into render texture, then post-process with shader
        BeginTextureMode(game->render_target);
            SceneFuncs *funcs = &scene_table[game->current_scene];
            if (funcs->draw) funcs->draw(game);
        EndTextureMode();

        SetShaderValue(game->daynight_shader, game->daynight_time_loc,
                       &game->time_of_day, SHADER_UNIFORM_FLOAT);

        // Torch light uniforms
        Vector2 player_screen = GetWorldToScreen2D(game->camera.target, game->camera);
        float light_radius = game->torch_active ? 80.0f * game->camera.zoom : 0.0f;
        float screen_size[2] = { (float)GetScreenWidth(), (float)GetScreenHeight() };
        SetShaderValue(game->daynight_shader, game->light_pos_loc, &player_screen, SHADER_UNIFORM_VEC2);
        SetShaderValue(game->daynight_shader, game->light_radius_loc, &light_radius, SHADER_UNIFORM_FLOAT);
        SetShaderValue(game->daynight_shader, game->screen_size_loc, screen_size, SHADER_UNIFORM_VEC2);

        // Cloud shadow uniforms
        // Intensity based on time of day: strongest at noon, zero at night/dawn/dusk
        float hour = game->time_of_day;
        float cloud_intensity = 0.0f;
        if (hour >= 8.0f && hour <= 18.0f) {
            // Daytime: ramp up from 8am, peak at noon, ramp down to 6pm
            float day_progress = (hour - 8.0f) / 10.0f;  // 0-1 over day
            cloud_intensity = sinf(day_progress * 3.14159f);  // Sine curve peaks at 0.5 (noon)
            cloud_intensity *= 0.8f;  // Max 80% intensity
        }
        float cloud_offset[2] = { game->cloud_offset_x, game->cloud_offset_y };
        float cloud_scale = 0.002f;  // Smaller = bigger clouds
        SetShaderValue(game->daynight_shader, game->cloud_intensity_loc, &cloud_intensity, SHADER_UNIFORM_FLOAT);
        SetShaderValue(game->daynight_shader, game->cloud_offset_loc, cloud_offset, SHADER_UNIFORM_VEC2);
        SetShaderValue(game->daynight_shader, game->cloud_scale_loc, &cloud_scale, SHADER_UNIFORM_FLOAT);
        SetShaderValue(game->daynight_shader, game->daynight_cam_target_loc, &game->camera.target, SHADER_UNIFORM_VEC2);
        SetShaderValue(game->daynight_shader, game->daynight_cam_offset_loc, &game->camera.offset, SHADER_UNIFORM_VEC2);
        SetShaderValue(game->daynight_shader, game->daynight_cam_zoom_loc, &game->camera.zoom, SHADER_UNIFORM_FLOAT);

        BeginShaderMode(game->daynight_shader);
            float w = (float)game->render_target.texture.width;
            float h = (float)game->render_target.texture.height;
            DrawTextureRec(game->render_target.texture,
                           (Rectangle){ 0, 0, w, -h }, (Vector2){ 0, 0 }, WHITE);
        EndShaderMode();
    } else if (game->current_scene >= 0 && game->current_scene < SCENE_COUNT) {
        SceneFuncs *funcs = &scene_table[game->current_scene];
        if (funcs->draw) funcs->draw(game);
    } else {
        ClearBackground(BLACK);
    }

    // UI overlay always draws on top, unaffected by shader
    if (ui_is_active(&game->ui)) {
        ui_draw(&game->ui, game);
    }
}

void game_cleanup(Game *game) {
    // Clean up all scenes
    for (int i = 0; i < SCENE_COUNT; i++) {
        if (game->scene_data[i] && scene_table[i].cleanup) {
            game->current_scene = (SceneID)i;
            scene_table[i].cleanup(game);
        }
        game->scene_data[i] = NULL;
    }

    // Clean up player sprite
    if (game->player_sprite) {
        sprite_destroy(game->player_sprite);
        game->player_sprite = NULL;
    }

    // Clean up inventory icon (while GL context alive)
    if (game->item_icon.id != 0) {
        UnloadTexture(game->item_icon);
        game->item_icon = (Texture2D){ 0 };
    }

    // Clean up day/night resources (while GL context alive)
    if (game->render_target.id != 0) {
        UnloadRenderTexture(game->render_target);
        game->render_target = (RenderTexture2D){ 0 };
    }
    if (game->daynight_shader.id != 0) {
        UnloadShader(game->daynight_shader);
        game->daynight_shader = (Shader){ 0 };
    }
    if (game->water_shader.id != 0) {
        UnloadShader(game->water_shader);
        game->water_shader = (Shader){ 0 };
    }
    if (game->reflection_shader.id != 0) {
        UnloadShader(game->reflection_shader);
        game->reflection_shader = (Shader){ 0 };
    }

    // Clean up audio (before event bus, while GL context alive)
    if (game->audio) {
        audio_destroy(game->audio);
        game->audio = NULL;
    }

    // Clean up event bus
    if (game->events) {
        event_bus_destroy(game->events);
        game->events = NULL;
    }
}
