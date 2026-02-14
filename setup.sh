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

echo "=== Checking for GCC ==="
if ! command -v gcc &> /dev/null; then
    if [ "$PLATFORM" = "windows" ]; then
        echo "ERROR: gcc not found. Install w64devkit first."
        echo "  Download: https://github.com/skeeto/w64devkit/releases"
        echo "  Extract and add w64devkit/bin to PATH"
    else
        echo "ERROR: gcc not found. Install build essentials:"
        echo "  Ubuntu/Debian: sudo apt install build-essential"
        echo "  Fedora: sudo dnf install gcc"
        echo "  Arch: sudo pacman -S gcc"
    fi
    exit 1
fi
gcc --version | head -1

# Check Linux dependencies
if [ "$PLATFORM" = "linux" ]; then
    echo "=== Checking Linux dev libraries ==="
    MISSING=""
    for lib in x11 gl; do
        if ! pkg-config --exists "$lib" 2>/dev/null; then
            MISSING="$MISSING $lib"
        fi
    done
    if [ -n "$MISSING" ]; then
        echo "ERROR: Missing dev libraries:$MISSING"
        echo "  Ubuntu/Debian: sudo apt install libx11-dev libgl-dev"
        echo "  Fedora: sudo dnf install libX11-devel mesa-libGL-devel"
        echo "  Arch: sudo pacman -S libx11 mesa"
        exit 1
    fi
    echo "All required libraries found."
fi

echo "=== Cloning raylib 5.5 ==="
if [ ! -d "raylib" ]; then
    git clone --depth 1 --branch 5.5 https://github.com/raysan5/raylib.git
else
    echo "raylib/ already exists, skipping clone."
fi

echo "=== Building raylib as static library ==="
bash build_raylib.sh

echo "=== Setup complete ($PLATFORM) ==="
if [ "$PLATFORM" = "windows" ]; then
    echo "Next: bash build.sh && cd build && ./main.exe"
else
    echo "Next: bash build.sh && cd build && ./main"
fi
