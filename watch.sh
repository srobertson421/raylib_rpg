#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

echo "=== Watching src/ for changes (Ctrl+C to stop) ==="

last_hash=""
while true; do
    current_hash=$(cat src/*.c src/*.h 2>/dev/null | md5sum)
    if [ "$current_hash" != "$last_hash" ] && [ -n "$last_hash" ]; then
        echo ""
        echo "--- Change detected at $(date +%H:%M:%S) ---"
        bash build_game.sh
    fi
    last_hash="$current_hash"
    sleep 1
done
