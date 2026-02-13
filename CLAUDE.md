# Project: Raylib Adventure

Plain C, single-executable 2D game built with raylib 5.5 on Windows.

## Toolchain

- **Compiler**: w64devkit (GCC) at `~/w64devkit/w64devkit/bin/` -- build scripts auto-detect this
- **Raylib**: built as a static library (`build/libraylib.a`), linked directly into the executable
- **Build**: `bash build.sh` compiles everything into `build/main.exe`
- **Run**: must run from `build/` directory so relative asset paths (`../assets/`) resolve

## Build Commands

```
bash setup.sh        # one-time: clone raylib + build static lib
bash build.sh        # full build: all .c files -> build/main.exe
bash watch.sh        # file watcher: auto-rebuild on src/ changes
```

## Source Layout

### Core
- `src/main.c` -- entry point, game loop, F6 reinit. ~30 lines, calls game_init/update/draw directly.
- `src/game.h` -- `Game` struct (global state, scene management, event bus), `RenderLayer` and `SceneID` enums, function declarations
- `src/game.c` -- game lifecycle, scene table dispatch, transitions. Owns the scene table, performs init/cleanup/transition logic.

### Scenes
- `src/scene.h` -- `SceneFuncs` interface: init/cleanup/update/draw + persistent flag
- `src/scene_menu.c` -- title screen, transitions to overworld on Enter
- `src/scene_overworld.c` -- main gameplay: tilemap, collision, player movement. Persistent (survives scene transitions).
- `src/scene_dungeon1.c` -- simple dungeon scene, non-persistent

Each scene stores per-scene data in `game->scene_data[SCENE_ID]` (heap-allocated, cast as needed).

### Systems
- `src/event.h / event.c` -- pub/sub event bus. Ring buffer of 256 events, 16 listeners per event type. `event_flush()` snapshots queue length to defer events emitted by callbacks. Wire: created in `game_init`, flushed after scene update, cleared on scene transitions.
- `src/tilemap.h / tilemap.c` -- Tiled JSON (.tmj/.tsj) loader and viewport-culled renderer. Handles tile flipping, external tilesets, render layer assignment via custom properties.
- `src/collision.h / collision.c` -- AABB collision world. Fixed array of 256 bodies (static/kinematic). `collision_move_and_slide` handles wall-sliding. Collision rects loaded from Tiled object layers.
- `src/sprite.h / sprite.c` -- animated sprite system. Spritesheet-based, named animations, directional facing.

### Vendored
- `src/cJSON.h / cJSON.c` -- JSON parser (MIT, v1.7.18). Used by tilemap loader.

## Key Patterns

- **Render layers**: tile layers and sprites are assigned a `RenderLayer` (ground/below_player/player/above_player). Drawing iterates layers in order so the player appears between map layers correctly.
- **Scene persistence**: overworld is `persistent = true` -- its data survives scene transitions. Non-persistent scenes (menu, dungeons) are cleaned up on exit and re-initialized on entry.
- **Tilemap custom properties**: Tiled layer property `render_layer` (string) maps to `RenderLayer` enum. Collision rects come from an object layer named `objects_collision`.

## Assets

All in `assets/`, loaded as `../assets/` relative to `build/`:
- `overworld.tmj` -- Tiled map
- `overworld.tsj` -- Tiled tileset (external)
- `overworld.png` -- tileset spritesheet
- `player.png` -- player spritesheet (16x32 frames)

## Common Gotchas

- Tiled can produce backslashes in JSON paths on Windows -- the tilemap loader has a fixup pass for this
- Tileset image loading has a fallback chain: resolved path -> filename only -> lowercase filename
- `game_cleanup()` must be called before `CloseWindow()` to properly free textures while the GL context is alive
