#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# Auto-detect w64devkit
W64DEV="$HOME/w64devkit/w64devkit/bin"
if [ -d "$W64DEV" ] && ! command -v gcc &>/dev/null; then
    export PATH="$W64DEV:$PATH"
fi

mkdir -p build

RAYLIB_INCLUDE="raylib/src"
RAYLIB_IMPORT_LIB="build/libraylibdll.a"

echo "=== Building main.exe ==="
gcc -o build/main.exe \
    src/main.c src/game.c src/event.c \
    src/scene_menu.c src/scene_overworld.c src/scene_dungeon1.c \
    src/tilemap.c src/cJSON.c src/collision.c src/sprite.c \
    -I"$RAYLIB_INCLUDE" \
    -Isrc \
    "$RAYLIB_IMPORT_LIB" \
    -lopengl32 -lgdi32 -lwinmm \
    -Wall -Wextra -O2

echo "=== Build complete ==="
echo "Run: cd build && ./main.exe"
