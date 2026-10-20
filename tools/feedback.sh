#!/usr/bin/env bash
# One indirect-branch feedback pass: run the instrumented build for a while,
# merge the targets it reached into the toolkit's database, regenerate the
# code with them as function seeds, and rebuild.
#
#   tools/feedback.sh [seconds]      (default 60)
set -euo pipefail

here=$(cd "$(dirname "$0")/.." && pwd)
toolkit=${XBOXRECOMP_DIR:-$here/xboxrecomp}
secs=${1:-60}
runs=$here/runs
mkdir -p "$runs"

if [ ! -f "$here/build/feedback/build.ninja" ]; then
    cmake -S "$here" -B "$here/build/feedback" -G Ninja \
          -DCMAKE_C_COMPILER=clang -DNGB_ICALL_FEEDBACK=ON >/dev/null
fi
ninja -C "$here/build/feedback" >/dev/null

echo "== running for ${secs}s"
( cd "$runs" && rm -f icall_targets.dump icall_targets.dump.sites &&
  timeout -s INT "$secs" "$here/build/feedback/NinjaGaidenBlack" "$here/game_files" \
      > "$runs/feedback.log" 2>&1 ) || true
grep -c "Failed to resolve" "$runs/feedback.log" | sed 's/^/   unresolved ICALL reports: /'

py=.venv/bin/python
[ -d .venv/Scripts ] && py=.venv/Scripts/python
echo "== merge"
( cd "$toolkit" && "$py" -m tools.recomp.icall_feedback --db "$here/src/icall_targets.json" merge "$runs/icall_targets.dump" )

"$here/tools/pipeline.sh"
echo "== build"
ninja -C "$here/build/linux" >/dev/null && echo "   ok"
