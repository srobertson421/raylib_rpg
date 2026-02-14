#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# Platform detection
OS="$(uname -s)"
case "$OS" in
    Linux*)   PLATFORM=linux ;;
    MINGW*|MSYS*|CYGWIN*) PLATFORM=windows ;;
    *)
        echo "ERROR: Unsupported platform: $OS"
        exit 1
        ;;
esac

# Auto-detect w64devkit on Windows
if [ "$PLATFORM" = "windows" ]; then
    W64DEV="$HOME/w64devkit/w64devkit/bin"
    if [ -d "$W64DEV" ] && ! command -v gcc &>/dev/null; then
        export PATH="$W64DEV:$PATH"
    fi
fi

mkdir -p build

RAYLIB_INCLUDE="raylib/src"
RAYLIB_LIB="build/libraylib.a"

if [ "$PLATFORM" = "windows" ]; then
    OUTPUT="build/main.exe"
    LIBS="-lopengl32 -lgdi32 -lwinmm"
else
    OUTPUT="build/main"
    LIBS="-lGL -lm -lpthread -ldl -lrt -lX11"
fi

echo "=== Building ${OUTPUT##*/} ($PLATFORM) ==="
gcc -o "$OUTPUT" \
    src/main.c src/game.c src/event.c src/settings.c src/audio.c src/ui.c src/inventory.c \
    src/scene_menu.c src/scene_overworld.c src/scene_dungeon1.c src/scene_settings.c src/scene_battle.c \
    src/tilemap.c src/cJSON.c src/collision.c src/sprite.c \
    -I"$RAYLIB_INCLUDE" \
    -Isrc \
    "$RAYLIB_LIB" \
    $LIBS \
    -Wall -Wextra -O2

echo "=== Build complete ==="
echo "Run: cd build && ./${OUTPUT##*/}"
