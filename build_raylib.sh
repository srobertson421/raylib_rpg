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

echo "=== Building raylib as static library ($PLATFORM) ==="

cd raylib/src
make clean
make PLATFORM=PLATFORM_DESKTOP RAYLIB_BUILD_MODE=RELEASE -j$(nproc)

echo "=== Copying static library to build/ ==="
mkdir -p ../../build
cp libraylib.a ../../build/

echo "=== raylib static build complete ==="
