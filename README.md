# Raylib Adventure

A 2D top-down adventure game built from scratch in C with [raylib](https://www.raylib.com/) 5.5.

![C](https://img.shields.io/badge/language-C-blue)
![raylib](https://img.shields.io/badge/raylib-5.5-green)
![Platform](https://img.shields.io/badge/platform-Windows-lightgrey)

## Features

- **Tiled map support** -- loads Tiled JSON exports (`.tmj`/`.tsj`) with external tilesets, tile flipping/rotation, and viewport culling
- **Scene management** -- menu, overworld, and dungeon scenes with transitions and optional persistence
- **Render layers** -- label-based draw ordering (ground, below player, player, above player)
- **AABB collision** -- axis-aligned bounding box collision with wall-sliding, loaded from Tiled object layers
- **Animated sprites** -- spritesheet-based animation system with named animations and directional facing
- **Pub/sub events** -- fixed-size ring buffer event bus for decoupled game systems
- **File watcher** -- `watch.sh` auto-rebuilds on source changes

## Prerequisites

- **[w64devkit](https://github.com/skeeto/w64devkit/releases)** -- portable GCC toolchain for Windows (extract to `~/w64devkit/`)
- **Git** -- for cloning raylib during setup
- **Bash** -- Git Bash (ships with Git for Windows) or similar

## Getting Started

```bash
# One-time setup: clone raylib 5.5 and build it as a shared library
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
| Enter/Space | Start game (menu) |
| 1 | Enter dungeon / return to overworld |
| Escape | Return to overworld (in dungeon) |
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
    scene_overworld.c   Main overworld with tilemap + collision
    scene_dungeon1.c    Dungeon scene (scene transition example)
    event.h / event.c   Pub/sub event bus
    tilemap.h / .c      Tiled JSON map loader + renderer
    collision.h / .c    AABB collision world with move-and-slide
    sprite.h / .c       Animated sprite system
    cJSON.h / .c        Vendored JSON parser (MIT, v1.7.18)
  assets/
    overworld.tmj       Tiled map (JSON)
    overworld.tsj       Tiled tileset (JSON)
    overworld.png       Tileset spritesheet
    player.png          Player spritesheet
  build.sh              Build script (single executable)
  build_game.sh         Delegates to build.sh (used by watch.sh)
  build_raylib.sh       Builds raylib as a shared library
  setup.sh              One-time setup (clone + build raylib)
  watch.sh              File watcher for auto-rebuild
```

## Build Scripts

| Script | Purpose |
|--------|---------|
| `bash setup.sh` | One-time: clone raylib 5.5, build as DLL |
| `bash build.sh` | Compile all source into `build/main.exe` |
| `bash watch.sh` | Watch `src/` for changes and auto-rebuild |

Raylib is built as a shared library (`raylib.dll`). The game links against the import library (`libraylibdll.a`) and ships with the DLL.

## Architecture Notes

**Single executable** -- all game code compiles into one `main.exe`. No DLL boundaries to worry about for function pointers or callbacks.

**Scene system** -- each scene provides `init`/`cleanup`/`update`/`draw` callbacks via a `SceneFuncs` struct. Scenes can be marked `persistent` to survive transitions (the overworld keeps its state when you enter and leave dungeons).

**Event bus** -- a fixed-size ring buffer (256 events) with up to 16 listeners per event type. Events emitted during a flush are deferred to the next flush to prevent infinite loops. Used for decoupling game systems (collision enter/exit, zone triggers, dialog, etc.).

**Tilemap rendering** -- only tiles visible within the camera viewport are drawn. Tile layers are assigned render layers via Tiled custom properties, allowing layers to draw above or below the player.

## License

Game code is unlicensed / public domain. Vendored dependencies have their own licenses:
- **raylib** -- zlib/libpng license
- **cJSON** -- MIT license
