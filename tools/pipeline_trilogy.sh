#!/usr/bin/env bash
# Regenerate src/recomp/gen_trilogy from the disc's NinjaTrilogy.xbe -- the NES Ninja Gaiden that the
# arcade cabinet and the main menu's ARCADE NINJA GAIDEN launch (#18; src/trilogy/main.c, src/launch.c).
#
#   tools/pipeline_trilogy.sh         (after tools/pipeline.sh has unpacked the disc into game_files/)
#   tools/pipeline_trilogy.sh tomoe   NinjaTomoe.xbe, the Japanese build, into src/recomp/gen_tomoe
#
# The analysis goes to build/trilogy (build/tomoe), apart from default.xbe's. The two builds' code is the
# same function for function (three struct offsets in the pause menu differ), so they share the seeds.
set -euo pipefail

here=$(cd "$(dirname "$0")/.." && pwd)
toolkit=${XBOXRECOMP_DIR:-$here/xboxrecomp}
toolkit=$(cd "$toolkit" && pwd)
if [ "${1:-}" = tomoe ]; then
    xbe=$here/game_files/NinjaTomoe.xbe out=$here/build/tomoe gen=$here/src/recomp/gen_tomoe name="Ninja Gaiden (NES, Japanese)"
else
    xbe=$here/game_files/NinjaTrilogy.xbe out=$here/build/trilogy gen=$here/src/recomp/gen_trilogy name="Ninja Gaiden (NES)"
fi
py=$toolkit/.venv/bin/python
[ -x "$py" ] || { echo "run tools/pipeline.sh first (it makes the toolkit's venv and unpacks the disc)"; exit 1; }
[ -f "$xbe" ] || { echo "no $xbe: run tools/pipeline.sh first"; exit 1; }
mkdir -p "$out/disasm" "$out/func_id" "$out/abi" "$out/recomp"
cd "$toolkit"
echo "== parse";   "$py" -m tools.xbe_parser "$xbe" --json "$out/analysis.json" >/dev/null
# Indirect-call targets the analysis misses (the controller library's completion routines among them:
# without them the pad never finished enumerating), measured by earlier runs ([ICALL] Failed lines).
seeds=()
[ -f "$here/src/icall_targets_trilogy.json" ] && seeds=(--seed-functions "$here/src/icall_targets_trilogy.json")
echo "== disasm";  "$py" -m tools.disasm "$xbe" --force -o "$out/disasm" --analysis-json "$out/analysis.json" "${seeds[@]}" >/dev/null
echo "== func_id"; "$py" -m tools.func_id "$xbe" --functions "$out/disasm/functions.json" \
                      --strings "$out/disasm/strings.json" --xrefs "$out/disasm/xrefs.json" -o "$out/func_id" >/dev/null
echo "== abi";     "$py" -m tools.abi_analysis "$xbe" --disasm-dir "$out/disasm" --func-id-dir "$out/func_id" \
                      --output-dir "$out/abi" >/dev/null
echo "== recomp"
mkdir -p "$gen" && find "$gen" -name "*.[ch]" -delete
"$py" -m tools.recomp "$xbe" --all --split 250 --game-name "$name" \
    --disasm-dir "$out/disasm" --func-id-dir "$out/func_id" --abi-dir "$out/abi" \
    -o "$out/recomp" --gen-dir "$gen" > "$out/recomp_log.txt" 2>&1 \
    || { tail -20 "$out/recomp_log.txt"; exit 1; }
tail -3 "$out/recomp_log.txt"
