#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# Auto-detect w64devkit
W64DEV="$HOME/w64devkit/w64devkit/bin"
if [ -d "$W64DEV" ] && ! command -v gcc &>/dev/null; then
    export PATH="$W64DEV:$PATH"
fi

RAYLIB_INCLUDE="raylib/src"
RAYLIB_IMPORT_LIB="build/libraylibdll.a"

echo "=== Rebuilding game.dll ==="
gcc -shared -o build/game.dll \
    src/game.c src/tilemap.c src/cJSON.c \
    -I"$RAYLIB_INCLUDE" \
    -Isrc \
    -DBUILD_GAME_DLL \
    "$RAYLIB_IMPORT_LIB" \
    -lopengl32 -lgdi32 -lwinmm \
    -Wall -Wextra -O2

echo "=== game.dll rebuilt ==="
