#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# Auto-detect w64devkit
W64DEV="$HOME/w64devkit/w64devkit/bin"
if [ -d "$W64DEV" ] && ! command -v gcc &>/dev/null; then
    export PATH="$W64DEV:$PATH"
fi

echo "=== Checking for GCC ==="
if ! command -v gcc &> /dev/null; then
    echo "ERROR: gcc not found. Install w64devkit first."
    echo "  Download: https://github.com/skeeto/w64devkit/releases"
    echo "  Extract and add w64devkit/bin to PATH"
    exit 1
fi
gcc --version | head -1

echo "=== Cloning raylib 5.5 ==="
if [ ! -d "raylib" ]; then
    git clone --depth 1 --branch 5.5 https://github.com/raysan5/raylib.git
else
    echo "raylib/ already exists, skipping clone."
fi

echo "=== Building raylib as DLL ==="
bash build_raylib.sh

echo "=== Setup complete ==="
echo "Next: bash build.sh && cd build && ./main.exe"
