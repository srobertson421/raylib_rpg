#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# Auto-detect w64devkit
W64DEV="$HOME/w64devkit/w64devkit/bin"
if [ -d "$W64DEV" ] && ! command -v gcc &>/dev/null; then
    export PATH="$W64DEV:$PATH"
fi

echo "=== Building raylib as static library ==="

cd raylib/src
make clean
make PLATFORM=PLATFORM_DESKTOP RAYLIB_BUILD_MODE=RELEASE -j$(nproc)

echo "=== Copying static library to build/ ==="
mkdir -p ../../build
cp libraylib.a ../../build/

echo "=== raylib static build complete ==="
