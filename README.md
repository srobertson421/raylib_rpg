# Raylib Adventure

A 2D top-down adventure game built from scratch in C with [raylib](https://www.raylib.com/) 5.5.

![C](https://img.shields.io/badge/language-C-blue)
![raylib](https://img.shields.io/badge/raylib-5.5-green)
![Platform](https://img.shields.io/badge/platform-Windows-lightgrey)

## Features

- **Tiled map support** -- loads Tiled JSON exports (`.tmj`/`.tsj`) with external tilesets, tile flipping/rotation, and viewport culling
- **Scene management** -- menu, overworld, dungeon, battle, and settings scenes with transitions and optional persistence
- **Turn-based battle system** -- timed attacks and defense with animated lunges, damage multipliers, and timing feedback
- **Elevation system** -- collision filtering by elevation, ramp/stair transitions, and ALttP-style visual layering (higher terrain renders semi-transparent above the player)
- **Render layers** -- label-based draw ordering (ground, below player, player, above player) with elevation-aware overrides
- **AABB collision** -- axis-aligned bounding box collision with wall-sliding and per-body elevation, loaded from Tiled object layers
- **Animated sprites** -- spritesheet-based animation system with named animations and directional facing
- **Audio system** -- background music with crossfading between scenes, track deduplication, volume control, and sectioned music with loop regions for battle phases
- **Pub/sub events** -- fixed-size ring buffer event bus for decoupled game systems (scene transitions, battle phases, audio triggers)
- **Settings menu** -- configurable resolution and music volume with persistent JSON config, live preview, and in-game settings screen
- **File watcher** -- `watch.sh` auto-rebuilds on source changes

## Prerequisites

- **[w64devkit](https://github.com/skeeto/w64devkit/releases)** -- portable GCC toolchain for Windows (extract to `~/w64devkit/`)
- **Git** -- for cloning raylib during setup
- **Bash** -- Git Bash (ships with Git for Windows) or similar

## Getting Started

```bash
# One-time setup: clone raylib 5.5 and build it as a static library
bash setup.sh

# Build the game
bash build.sh

# Run (must be in build/ so asset paths resolve)
cd build && ./main.exe
```

## Controls

| Key | Action |
|-----|--------|
| Arrow keys | Move player |
| Enter/Space | Start game (menu) / confirm |
| 1 | Enter dungeon |
| 2 | Enter battle |
| Escape | Return to overworld / back |
| F3 | Toggle collision debug wireframes |
| F6 | Reinitialize game state |

## Project Structure

```
raylib_fun/
  src/
    main.c              Entry point and game loop
    game.h / game.c     Game struct, init/update/draw, scene dispatch
    scene.h             Scene interface (SceneFuncs)
    scene_menu.c        Title screen
    scene_overworld.c   Main overworld with tilemap + collision + elevation
    scene_dungeon1.c    Dungeon scene (scene transition example)
    scene_battle.c      Turn-based battle with timed attacks/defense
    scene_settings.c    Settings menu (resolution + volume)
    settings.h / .c     Settings config (persistent JSON)
    audio.h / audio.c   Audio manager (crossfade, sections, events)
    event.h / event.c   Pub/sub event bus
    tilemap.h / .c      Tiled JSON map loader + renderer (with tinted draw)
    collision.h / .c    AABB collision world with elevation + ramps
    sprite.h / .c       Animated sprite system
    cJSON.h / .c        Vendored JSON parser (MIT, v1.7.18)
  assets/
    overworld.tmj       Tiled map (JSON)
    overworld.tsj       Tiled tileset (JSON)
    overworld.png       Tileset spritesheet
    player.png          Player spritesheet
    overworld_bgm.mp3   Overworld background music
    battle_bgm.mp3      Battle background music (with sections)
  build.sh              Build script (single executable)
  build_game.sh         Delegates to build.sh (used by watch.sh)
  build_raylib.sh       Builds raylib as a shared library
  setup.sh              One-time setup (clone + build raylib)
  watch.sh              File watcher for auto-rebuild
```

## Build Scripts

| Script | Purpose |
|--------|---------|
| `bash setup.sh` | One-time: clone raylib 5.5, build static lib |
| `bash build.sh` | Compile all source into `build/main.exe` |
| `bash watch.sh` | Watch `src/` for changes and auto-rebuild |

Raylib is built as a static library (`libraylib.a`) and linked directly into the executable -- no DLL needed at runtime.

## Architecture Notes

**Single executable** -- all game code compiles into one `main.exe`. No DLL boundaries to worry about for function pointers or callbacks.

**Scene system** -- each scene provides `init`/`cleanup`/`update`/`draw` callbacks via a `SceneFuncs` struct. Scenes can be marked `persistent` to survive transitions (the overworld keeps its state when you enter and leave dungeons).

**Event bus** -- a fixed-size ring buffer (256 events) with up to 16 listeners per event type. Events emitted during a flush are deferred to the next flush to prevent infinite loops. Used for decoupling game systems (collision enter/exit, zone triggers, dialog, scene transitions, audio, etc.).

**Tilemap rendering** -- only tiles visible within the camera viewport are drawn. Tile layers are assigned render layers via Tiled custom properties, allowing layers to draw above or below the player.

**Elevation system** -- collision bodies and tile layers have an `elevation` field. Collisions are only checked between bodies at the same elevation. Ramp objects (type `elevation_ramp` with `from_elevation`/`to_elevation` properties) transition the player between levels. Tile layers at a higher elevation than the player render semi-transparently above the player (ALttP-style).

**Battle system** -- turn-based combat with timed action mechanics. During attack and defense phases, pressing Space/Enter at the right moment in the animation window yields Good or Excellent timing, which scales damage dealt/blocked. Battle music uses sectioned playback with loop regions that change with each battle phase.

**Audio system** -- background music with 1.5s crossfading between tracks on scene transitions. Track deduplication prevents reloading when returning to a scene with the same BGM. Sectioned music supports named regions with start/end times and per-section looping, driven by event bus callbacks. Volume is adjustable in the settings menu with live preview.

## License

Game code is unlicensed / public domain. Vendored dependencies have their own licenses:
- **raylib** -- zlib/libpng license
- **cJSON** -- MIT license
