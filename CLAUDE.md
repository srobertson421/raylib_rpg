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
- `src/game.h` -- `Game` struct (global state, scene management, event bus, UI overlay, inventory, day/night + torch light, per-layer shaders), `RenderLayer`, `SceneID`, and `BattlePhase` enums, function declarations
- `src/game.c` -- game lifecycle, scene table dispatch, transitions. ESC opens pause overlay, I opens inventory overlay (both skipped on menu/settings). Overlay gates scene update when active. F5 toggles torch light. Owns the scene table, performs init/cleanup/transition logic.

### Scenes
- `src/scene.h` -- `SceneFuncs` interface: init/cleanup/update/draw + persistent flag
- `src/scene_menu.c` -- title screen, transitions to overworld on Enter
- `src/scene_overworld.c` -- main gameplay: tilemap, collision, player movement, elevation system, per-layer shader dispatch. Persistent (survives scene transitions).
- `src/scene_dungeon1.c` -- simple dungeon scene, non-persistent
- `src/scene_battle.c` -- turn-based battle with timed attacks/defense. Phases: intro, player menu, attack anim, resolve, enemy attack, resolve, win/lose. Timing windows during attack/defense anims scale damage. Phase transitions emit `EVT_BATTLE_PHASE_CHANGE` events to drive sectioned music. Menu: Attack/Defend/Flee (ESC opens pause overlay instead).
- `src/scene_settings.c` -- settings menu (resolution + music volume), navigable from the main menu

Each scene stores per-scene data in `game->scene_data[SCENE_ID]` (heap-allocated, cast as needed).

### Systems
- `src/inventory.h / inventory.c` -- player inventory system. `ItemID` enum (potion, key, sword, shield, rope, torch, map, compass), `ItemMetadata` table, `Inventory` struct with 32 slots + money. `inventory_add_item` stacks onto existing slots first, then fills empty. `inventory_remove_item` decrements and clears at zero. Initialized with test items (3 potions, 1 key, 1 sword, 1 shield, 5 torches, 1 rope) and 100 gold.
- `src/ui.h / ui.c` -- UI overlay system. State machine (closed/opening/open/closing) with ease-out cubic animation (~0.25s open, ~0.20s close). Three pages: pause (Resume/Settings/Quit to Menu), settings sub-page (volume slider with live preview, resolution picker, back with revert), and inventory (8-column grid, item icons, quantity badges, detail panel). Page-dependent panel sizing (80% screen for inventory, 420x320 for pause/settings). `UIOverlay` struct embedded by value in `Game`. `ui_update()` returns true when active to gate scene updates. `ui_is_active()` checks if overlay is consuming input.
- `src/audio.h / audio.c` -- audio manager. Crossfading music (1.5s fade, two-stream system), track dedup via path comparison, volume control. Sectioned music with named regions, loop support, and `SeekMusicStream`-based section playback. Subscribes to `EVT_SCENE_ENTER` (scene BGM mapping) and `EVT_BATTLE_PHASE_CHANGE` (battle section mapping) via event bus.
- `src/event.h / event.c` -- pub/sub event bus. Ring buffer of 256 events, 16 listeners per event type. `event_flush()` snapshots queue length to defer events emitted by callbacks. Wire: created in `game_init`, flushed after scene update, cleared on scene transitions. `event_clear()` resets the queue but preserves listener subscriptions.
- `src/tilemap.h / tilemap.c` -- Tiled JSON (.tmj/.tsj) loader and viewport-culled renderer. Handles tile flipping, external tilesets, render layer assignment via custom properties, per-tile animations, and per-layer shader tags. `TileLayer.shader_name` parsed from Tiled `shader` custom property. `tilemap_draw_layer_tinted()` supports per-call tint color for elevation transparency. `tilemap_update()` advances the shared animation clock.
- `src/collision.h / collision.c` -- AABB collision world. Fixed array of 256 bodies (static/kinematic) with per-body `elevation`. `collision_move_and_slide` only resolves against bodies at the same elevation. Ramp objects (`elevation_ramp` type) loaded separately via `collision_load_ramps_from_tilemap`. Collision rects loaded from Tiled object layers.
- `src/sprite.h / sprite.c` -- animated sprite system. Spritesheet-based, named animations, directional facing. `sprite_draw_reflected()` draws a vertically-flipped sprite for water reflections.
- `src/settings.h / settings.c` -- settings with persistent JSON config (`settings.json`). Resolution table + music volume (0.0-1.0). `settings_apply_volume()` syncs volume to audio manager.

### Vendored
- `src/cJSON.h / cJSON.c` -- JSON parser (MIT, v1.7.18). Used by tilemap loader.

## Key Patterns

- **Render layers**: tile layers and sprites are assigned a `RenderLayer` (ground/below_player/player/above_player). Drawing iterates layers in order so the player appears between map layers correctly.
- **Elevation system**: tile layers and collision bodies have an `elevation` field (int, from Tiled custom properties). Collisions only resolve between bodies at the same elevation. Tile layers above the player's elevation are forced to `RENDER_LAYER_ABOVE_PLAYER` and drawn semi-transparent (alpha 160). Ramp objects (type `elevation_ramp`, custom props `from_elevation`/`to_elevation`) transition the player between elevation levels.
- **Scene persistence**: overworld is `persistent = true` -- its data survives scene transitions. Non-persistent scenes (menu, dungeons, battles) are cleaned up on exit and re-initialized on entry.
- **Tilemap custom properties**: Tiled layer property `render_layer` (string) maps to `RenderLayer` enum. Tiled layer property `elevation` (int) sets the layer's height level. Tiled layer property `shader` (string) tags a layer for per-layer shader dispatch (e.g., `"water"`). Collision rects come from an object layer named `objects_collision`. Object type `elevation_ramp` with `from_elevation`/`to_elevation` props defines ramp zones.
- **UI overlay**: ESC opens animated pause overlay, I opens inventory overlay (both except on menu/settings scenes). `game_update` checks ESC/I before scene update; when overlay is active, scene update is skipped but audio/events still tick. Overlay draws on top of frozen scene frame. Settings sub-page snapshots volume on entry and reverts on ESC/Back. Quit to Menu closes instantly (no animation) and triggers scene transition.
- **Inventory**: `Inventory` struct lives in `Game` (persists across scenes). 32 slots, stackable items. Grid UI with 8 columns, sack icon placeholder (`item_icon` texture loaded once in `game_init`), quantity badges, item detail panel below grid. Arrow key navigation with wrapping. ESC or I closes.
- **Battle timing**: attack/defense phases use a normalized `anim_t` (0-1). Pressing action during the timing window (0.75-0.92) yields Good (1.5x) or Excellent (2x) damage/block multipliers.
- **Audio crossfade**: `audio_play_music()` moves the current stream to a fading slot and loads the new track at volume 0, interpolating over `fade_duration` (1.5s). Track dedup via `current_path` strcmp prevents reloading the same track (e.g., returning to overworld from dungeon).
- **Sectioned music**: `MusicSection` structs define named time regions with start/end and loop flag. `audio_play_section()` seeks to a section's start; `audio_update()` checks boundaries and loops or deactivates. Battle sections (intro, battle_loop, attack_hit, victory, defeat) are driven by `EVT_BATTLE_PHASE_CHANGE` events.
- **Event-driven audio**: audio subscribes to `EVT_SCENE_ENTER` and `EVT_BATTLE_PHASE_CHANGE` in `game_init()`. Scene BGM is mapped via `scene_bgm_path()`. Scenes no longer call audio directly.
- **Tile animations**: Tiled per-tile animations are parsed from the `tiles` array in tileset JSON. `TileAnim` structs are stored in a flat `anim_lookup` array (indexed by local tile ID) for O(1) draw-time resolution. A shared `anim_time` clock (ms) on `TileMap` is advanced by `tilemap_update(dt)` in the scene update. Current frame is resolved via modular arithmetic over `total_duration`. Animations pause when the UI overlay is active (scene update gated).
- **Torch light**: Player torch is a point light in the day/night shader. Three uniforms (`screen_size`, `light_pos`, `light_radius`) drive a `smoothstep` blend from warm torch tint `(1.0, 0.9, 0.7)` at the player's screen position to the scene tint at the outer radius. Inner radius is 30% of outer for a solid bright core. Radius is `80 * camera.zoom` pixels (scales with zoom). `light_radius = 0` disables the effect entirely. F5 toggles on/off (debug).
- **Cloud shadows**: Procedural cloud shadows in the daynight post-process shader. Uses 3-octave FBM (Fractal Brownian Motion) simplex noise to generate smooth, organic cloud shapes. World-space coordinates ensure shadows stay locked to the world, not the screen. Cloud offset animates over time (randomized speed 15-25 horizontal, 2-7 vertical world units/sec). Intensity is time-of-day modulated: peak at noon, zero at night/dawn/dusk. Tunable parameters: `cloud_scale` in game.c (smaller = bigger clouds, currently 0.002), smoothstep thresholds in daynight.fs (currently 0.15-0.45 for broad coverage with soft edges), shadow darkening factor (currently 0.65 = max 65% darkening).
- **Per-layer water shader**: tile layers tagged with `shader: "water"` in Tiled are wrapped with `BeginShaderMode`/`EndShaderMode` during the overworld draw loop. The water fragment shader reconstructs world-space position from `gl_FragCoord` using camera uniforms (`camera_target`, `camera_offset`, `camera_zoom`), producing brightness waves and a blue-green tint shift locked to the world (not the screen). Uses `GetTime()` so waves continue during overlay. Preserves raylib's tint pipeline (`colDiffuse * fragColor`) so elevated semi-transparent water layers work correctly. Composes inside both `BeginMode2D` and `BeginTextureMode` (day/night render target) with no extra render textures. **Important**: Water shader uses brightness/tint modulation, NOT UV displacement -- UV displacement causes tile edge artifacts on tilemaps.
- **Player water reflection**: After drawing water layers, a vertically-flipped player sprite is drawn with a separate reflection shader (`reflection.fs`) that applies UV displacement ripple + blue tint + 50% transparency. Reflection only draws when player elevation matches the water layer's elevation. Draw order masking hides reflection edges: water layer (bottom) → reflection → grass/ground layer (top, occludes edges). Achieved by placing water on `render_layer: "ground"` below grass in Tiled layer order.

## Assets

All in `assets/`, loaded as `../assets/` relative to `build/`:
- `overworld.tmj` -- Tiled map
- `overworld.tsj` -- Tiled tileset (external)
- `overworld.png` -- tileset spritesheet
- `player.png` -- player spritesheet (16x32 frames)
- `sack.png` -- placeholder item icon for inventory grid
- `overworld_bgm.mp3` -- overworld background music (looping)
- `battle_bgm.mp3` -- battle background music (sectioned: intro, battle_loop, attack_hit, victory, defeat)
- `daynight.fs` -- day/night post-process fragment shader (time-of-day tinting + torch point light + cloud shadows)
- `water.fs` -- per-layer water shimmer shader (world-space brightness waves + blue-green tint shift)
- `reflection.fs` -- player reflection shader (UV displacement ripple + blue tint + transparency)

## Common Gotchas

- Tiled can produce backslashes in JSON paths on Windows -- the tilemap loader has a fixup pass for this
- Tileset image loading has a fallback chain: resolved path -> filename only -> lowercase filename
- `game_cleanup()` must be called before `CloseWindow()` to properly free textures while the GL context is alive
- `audio_destroy()` must unload both current and fading music streams to avoid leaks on F6 reinit
- Battle section timestamps in `audio.c` are placeholders -- tune `start_time`/`end_time` to match the actual `battle_bgm.mp3`
