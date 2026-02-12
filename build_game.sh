#!/bin/bash
# Delegates to build.sh (kept for compatibility with watch.sh)
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec bash "$SCRIPT_DIR/build.sh"
