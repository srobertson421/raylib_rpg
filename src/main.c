#include "raylib.h"
#include "game.h"

#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#include <windows.h>
#include <stdio.h>
#include <stdbool.h>

typedef GameAPI (*GetGameAPIFunc)(void);

typedef struct HotReloadState {
    HMODULE dll;
    GameAPI api;
    FILETIME last_write_time;
    bool loaded;
} HotReloadState;

static FILETIME get_file_write_time(const char *path) {
    FILETIME ft = {0};
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (GetFileAttributesExA(path, GetFileExInfoStandard, &data)) {
        ft = data.ftLastWriteTime;
    }
    return ft;
}

static bool file_times_differ(FILETIME a, FILETIME b) {
    return a.dwLowDateTime != b.dwLowDateTime ||
           a.dwHighDateTime != b.dwHighDateTime;
}

static const char *GAME_DLL_PATH      = "game.dll";
static const char *GAME_DLL_LIVE_PATH = "game_live.dll";

static bool load_game_dll(HotReloadState *hrs) {
    if (!CopyFileA(GAME_DLL_PATH, GAME_DLL_LIVE_PATH, FALSE)) {
        printf("[hot-reload] Failed to copy %s -> %s (error %lu)\n",
               GAME_DLL_PATH, GAME_DLL_LIVE_PATH, GetLastError());
        return false;
    }

    hrs->dll = LoadLibraryA(GAME_DLL_LIVE_PATH);
    if (!hrs->dll) {
        printf("[hot-reload] Failed to load %s (error %lu)\n",
               GAME_DLL_LIVE_PATH, GetLastError());
        return false;
    }

    GetGameAPIFunc get_api =
        (GetGameAPIFunc)(void *)GetProcAddress(hrs->dll, "get_game_api");
    if (!get_api) {
        printf("[hot-reload] get_game_api not found (error %lu)\n",
               GetLastError());
        FreeLibrary(hrs->dll);
        hrs->dll = NULL;
        return false;
    }

    hrs->api = get_api();
    hrs->last_write_time = get_file_write_time(GAME_DLL_PATH);
    hrs->loaded = true;
    printf("[hot-reload] Loaded game DLL.\n");
    return true;
}

static void unload_game_dll(HotReloadState *hrs) {
    if (hrs->dll) {
        FreeLibrary(hrs->dll);
        hrs->dll = NULL;
    }
    hrs->loaded = false;
    hrs->api = (GameAPI){0};
    printf("[hot-reload] Unloaded game DLL.\n");
}

static bool reload_game_dll(HotReloadState *hrs) {
    unload_game_dll(hrs);
    Sleep(100);
    return load_game_dll(hrs);
}

int main(void) {
    InitWindow(800, 600, "raylib hot-reload demo");
    SetTargetFPS(60);

    Game state = {0};

    HotReloadState hrs = {0};
    if (!load_game_dll(&hrs)) {
        printf("[error] Could not load game DLL. Exiting.\n");
        CloseWindow();
        return 1;
    }

    if (hrs.api.init) {
        hrs.api.init(&state);
    }

    while (!WindowShouldClose()) {
        bool should_reload = false;
        bool should_reinit = false;

        if (IsKeyPressed(KEY_F5)) {
            printf("[hot-reload] Manual reload (F5).\n");
            should_reload = true;
        }

        if (IsKeyPressed(KEY_F6)) {
            printf("[hot-reload] Reload + reinit (F6).\n");
            should_reload = true;
            should_reinit = true;
        }

        FILETIME current_time = get_file_write_time(GAME_DLL_PATH);
        if (file_times_differ(current_time, hrs.last_write_time)) {
            printf("[hot-reload] File change detected.\n");
            should_reload = true;
        }

        if (should_reload) {
            reload_game_dll(&hrs);
            if (should_reinit && hrs.loaded && hrs.api.init) {
                hrs.api.init(&state);
            }
        }

        if (hrs.loaded && hrs.api.update) {
            hrs.api.update(&state);
        }

        BeginDrawing();
        if (hrs.loaded && hrs.api.draw) {
            hrs.api.draw(&state);
        } else {
            ClearBackground(BLACK);
            DrawText("Game DLL not loaded!", 250, 280, 30, RED);
        }
        EndDrawing();
    }

    unload_game_dll(&hrs);
    CloseWindow();
    return 0;
}
