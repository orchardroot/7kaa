#!/bin/sh
# Launch the game briefly to prove it boots. Usage: tools/smoke.sh
cd "$(dirname "$0")/.." || exit 1
SKDATA=data src/7kaa & pid=$!
sleep 8
if kill -0 "$pid" 2>/dev/null; then kill "$pid"; echo "SMOKE OK"; exit 0
else echo "SMOKE FAIL: game exited early"; wait "$pid"; exit 1; fi
