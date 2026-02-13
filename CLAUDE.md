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
- `src/main.c` -- entry point, game loop, F6 reinit. ~30 lines, calls game_init/update/draw directly. `SetExitKey(0)` disables raylib's default ESC-to-quit.
- `src/game.h` -- `Game` struct (global state, scene management, event bus, UI overlay), `RenderLayer`, `SceneID`, and `BattlePhase` enums, function declarations
- `src/game.c` -- game lifecycle, scene table dispatch, transitions. ESC interception opens UI overlay (skipped on menu/settings). Overlay gates scene update when active. Owns the scene table, performs init/cleanup/transition logic.

### Scenes
- `src/scene.h` -- `SceneFuncs` interface: init/cleanup/update/draw + persistent flag
- `src/scene_menu.c` -- title screen, transitions to overworld on Enter
- `src/scene_overworld.c` -- main gameplay: tilemap, collision, player movement, elevation system. Persistent (survives scene transitions).
- `src/scene_dungeon1.c` -- simple dungeon scene, non-persistent
- `src/scene_battle.c` -- turn-based battle with timed attacks/defense. Phases: intro, player menu, attack anim, resolve, enemy attack, resolve, win/lose. Timing windows during attack/defense anims scale damage. Phase transitions emit `EVT_BATTLE_PHASE_CHANGE` events to drive sectioned music. Menu: Attack/Defend/Flee (ESC opens pause overlay instead).
- `src/scene_settings.c` -- settings menu (resolution + music volume), navigable from the main menu

Each scene stores per-scene data in `game->scene_data[SCENE_ID]` (heap-allocated, cast as needed).

### Systems
- `src/ui.h / ui.c` -- UI overlay system. State machine (closed/opening/open/closing) with ease-out cubic animation (~0.25s open, ~0.20s close). Pause page (Resume/Settings/Quit to Menu) and settings sub-page (volume slider with live preview, resolution picker, back with revert). `UIOverlay` struct embedded by value in `Game`. `ui_update()` returns true when active to gate scene updates. `ui_is_active()` checks if overlay is consuming input.
- `src/audio.h / audio.c` -- audio manager. Crossfading music (1.5s fade, two-stream system), track dedup via path comparison, volume control. Sectioned music with named regions, loop support, and `SeekMusicStream`-based section playback. Subscribes to `EVT_SCENE_ENTER` (scene BGM mapping) and `EVT_BATTLE_PHASE_CHANGE` (battle section mapping) via event bus.
- `src/event.h / event.c` -- pub/sub event bus. Ring buffer of 256 events, 16 listeners per event type. `event_flush()` snapshots queue length to defer events emitted by callbacks. Wire: created in `game_init`, flushed after scene update, cleared on scene transitions. `event_clear()` resets the queue but preserves listener subscriptions.
- `src/tilemap.h / tilemap.c` -- Tiled JSON (.tmj/.tsj) loader and viewport-culled renderer. Handles tile flipping, external tilesets, render layer assignment via custom properties. `tilemap_draw_layer_tinted()` supports per-call tint color for elevation transparency.
- `src/collision.h / collision.c` -- AABB collision world. Fixed array of 256 bodies (static/kinematic) with per-body `elevation`. `collision_move_and_slide` only resolves against bodies at the same elevation. Ramp objects (`elevation_ramp` type) loaded separately via `collision_load_ramps_from_tilemap`. Collision rects loaded from Tiled object layers.
- `src/sprite.h / sprite.c` -- animated sprite system. Spritesheet-based, named animations, directional facing.
- `src/settings.h / settings.c` -- settings with persistent JSON config (`settings.json`). Resolution table + music volume (0.0-1.0). `settings_apply_volume()` syncs volume to audio manager.

### Vendored
- `src/cJSON.h / cJSON.c` -- JSON parser (MIT, v1.7.18). Used by tilemap loader.

## Key Patterns

- **Render layers**: tile layers and sprites are assigned a `RenderLayer` (ground/below_player/player/above_player). Drawing iterates layers in order so the player appears between map layers correctly.
- **Elevation system**: tile layers and collision bodies have an `elevation` field (int, from Tiled custom properties). Collisions only resolve between bodies at the same elevation. Tile layers above the player's elevation are forced to `RENDER_LAYER_ABOVE_PLAYER` and drawn semi-transparent (alpha 160). Ramp objects (type `elevation_ramp`, custom props `from_elevation`/`to_elevation`) transition the player between elevation levels.
- **Scene persistence**: overworld is `persistent = true` -- its data survives scene transitions. Non-persistent scenes (menu, dungeons, battles) are cleaned up on exit and re-initialized on entry.
- **Tilemap custom properties**: Tiled layer property `render_layer` (string) maps to `RenderLayer` enum. Tiled layer property `elevation` (int) sets the layer's height level. Collision rects come from an object layer named `objects_collision`. Object type `elevation_ramp` with `from_elevation`/`to_elevation` props defines ramp zones.
- **UI overlay**: ESC opens animated pause overlay (except on menu/settings scenes). `game_update` checks ESC before scene update; when overlay is active, scene update is skipped but audio/events still tick. Overlay draws on top of frozen scene frame. Settings sub-page snapshots volume on entry and reverts on ESC/Back. Quit to Menu closes instantly (no animation) and triggers scene transition.
- **Battle timing**: attack/defense phases use a normalized `anim_t` (0-1). Pressing action during the timing window (0.75-0.92) yields Good (1.5x) or Excellent (2x) damage/block multipliers.
- **Audio crossfade**: `audio_play_music()` moves the current stream to a fading slot and loads the new track at volume 0, interpolating over `fade_duration` (1.5s). Track dedup via `current_path` strcmp prevents reloading the same track (e.g., returning to overworld from dungeon).
- **Sectioned music**: `MusicSection` structs define named time regions with start/end and loop flag. `audio_play_section()` seeks to a section's start; `audio_update()` checks boundaries and loops or deactivates. Battle sections (intro, battle_loop, attack_hit, victory, defeat) are driven by `EVT_BATTLE_PHASE_CHANGE` events.
- **Event-driven audio**: audio subscribes to `EVT_SCENE_ENTER` and `EVT_BATTLE_PHASE_CHANGE` in `game_init()`. Scene BGM is mapped via `scene_bgm_path()`. Scenes no longer call audio directly.

## Assets

All in `assets/`, loaded as `../assets/` relative to `build/`:
- `overworld.tmj` -- Tiled map
- `overworld.tsj` -- Tiled tileset (external)
- `overworld.png` -- tileset spritesheet
- `player.png` -- player spritesheet (16x32 frames)
- `overworld_bgm.mp3` -- overworld background music (looping)
- `battle_bgm.mp3` -- battle background music (sectioned: intro, battle_loop, attack_hit, victory, defeat)

## Common Gotchas

- Tiled can produce backslashes in JSON paths on Windows -- the tilemap loader has a fixup pass for this
- Tileset image loading has a fallback chain: resolved path -> filename only -> lowercase filename
- `game_cleanup()` must be called before `CloseWindow()` to properly free textures while the GL context is alive
- `audio_destroy()` must unload both current and fading music streams to avoid leaks on F6 reinit
- Battle section timestamps in `audio.c` are placeholders -- tune `start_time`/`end_time` to match the actual `battle_bgm.mp3`
