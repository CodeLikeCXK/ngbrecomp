#!/usr/bin/env bash
# Ninja Gaiden Black (static recompilation) for Windows ARM64 (native on Snapdragon 8cx Gen 3 / WoA).
#
#   packaging/windows/build-arm64.sh [out-dir]      (default build/windows-arm64-release)
#
# Fetches llvm-mingw and MSYS2 CLANGARM64 packages in deps-arm64.txt, builds the game against them
# (cmake/toolchains/windows-arm64-llvm-mingw.cmake) and packs NinjaGaidenBlack-windows-arm64.zip.
set -euo pipefail

here=$(cd "$(dirname "$0")" && pwd)
repo=$(cd "$here/../.." && pwd)
deps=$(mkdir -p "${NGB_WIN_DEPS:-$repo/build/windows-deps-arm64}" && cd "${NGB_WIN_DEPS:-$repo/build/windows-deps-arm64}" && pwd)
out=$(mkdir -p "${1:-$repo/build/windows-arm64-release}" && cd "${1:-$repo/build/windows-arm64-release}" && pwd)

# Locate tar (prefer bsdtar or Windows system32 tar which supports zstd and zip)
if command -v bsdtar >/dev/null 2>&1; then
    tar_cmd="bsdtar"
elif [ -x /c/Windows/system32/tar.exe ]; then
    tar_cmd="/c/Windows/system32/tar.exe"
elif [ -x "C:/Windows/system32/tar.exe" ]; then
    tar_cmd="C:/Windows/system32/tar.exe"
elif command -v tar.exe >/dev/null 2>&1; then
    tar_cmd="tar.exe"
elif command -v tar >/dev/null 2>&1; then
    tar_cmd="tar"
else
    echo "Error: Neither bsdtar nor tar found." >&2
    exit 1
fi

os=$(uname -s)
case "$os" in
    MINGW*|MSYS*|CYGWIN*|Windows_NT)
        if [ "${PROCESSOR_ARCHITECTURE:-}" = "ARM64" ] || [ "${PROCESSOR_ARCHITEW6432:-}" = "ARM64" ] || [ "$(uname -m)" = "aarch64" ]; then
            llvm_mingw="llvm-mingw-20260922-ucrt-aarch64"
        else
            llvm_mingw="llvm-mingw-20260922-ucrt-x86_64"
        fi
        llvm_mingw_archive="$llvm_mingw.zip"
        ;;
    Linux*)
        llvm_mingw="llvm-mingw-20260922-ucrt-ubuntu-22.04-x86_64"
        llvm_mingw_archive="$llvm_mingw.tar.xz"
        ;;
    Darwin*)
        llvm_mingw="llvm-mingw-20260922-ucrt-macos-universal"
        llvm_mingw_archive="$llvm_mingw.tar.xz"
        ;;
    *)
        llvm_mingw="llvm-mingw-20260922-ucrt-ubuntu-22.04-x86_64"
        llvm_mingw_archive="$llvm_mingw.tar.xz"
        ;;
esac
msys=https://repo.msys2.org/mingw/clangarm64

[ -n "$(ls "$repo"/src/recomp/gen/*.c 2>/dev/null)" ] ||
    { echo "No generated code: run tools/pipeline.sh first." >&2; exit 1; }

# The toolchain (llvm-mingw has multi-target support including aarch64-w64-mingw32)
if [ ! -d "$deps/$llvm_mingw" ] || [ ! -e "$deps/$llvm_mingw/bin/clang" -a ! -e "$deps/$llvm_mingw/bin/clang.exe" ]; then
    echo "== downloading $llvm_mingw"
    [ -f "$deps/$llvm_mingw_archive" ] || curl -fL -o "$deps/$llvm_mingw_archive" \
        "https://github.com/mstorsjo/llvm-mingw/releases/download/20260922/$llvm_mingw_archive"
    "$tar_cmd" -xf "$deps/$llvm_mingw_archive" -C "$deps"
fi

# The sysroot: unpacked again whenever deps-arm64.txt changes
want=$(grep -v '^#' "$here/deps-arm64.txt" | grep .)
if [ "$want" != "$(cat "$deps/sysroot.stamp" 2>/dev/null)" ]; then
    mkdir -p "$deps/pkgs"
    rm -rf "${deps:?}/sysroot"
    mkdir -p "$deps/sysroot"
    for p in $want; do
        [ -s "$deps/pkgs/$p" ] || curl -fL -o "$deps/pkgs/$p" "$msys/$p"
        "$tar_cmd" -xf "$deps/pkgs/$p" -C "$deps/sysroot" clangarm64
    done
    echo "$want" > "$deps/sysroot.stamp"
fi

# Fetch third_party sources if needed
if [ ! -d "$repo/xboxrecomp/third_party/volk/.git" ] && [ ! -f "$repo/xboxrecomp/third_party/volk/volk.h" ]; then
    echo "== fetching third-party dependencies"
    "$repo/xboxrecomp/third_party/fetch.sh"
fi

# Ensure nv2a shim symlink/junction exists on Windows
shim_nv2a="$repo/xboxrecomp/src/nv2a_vk/shim/hw/xbox/nv2a"
if [ ! -d "$shim_nv2a" ]; then
    rm -f "$shim_nv2a"
    win_src=$(cygpath -w "$repo/xboxrecomp/src/nv2a_vk/xemu" 2>/dev/null || echo "$repo/xboxrecomp/src/nv2a_vk/xemu")
    win_dst=$(cygpath -w "$shim_nv2a" 2>/dev/null || echo "$shim_nv2a")
    cmd //c "mklink /J \"$win_dst\" \"$win_src\"" 2>/dev/null || cp -r "$repo/xboxrecomp/src/nv2a_vk/xemu" "$shim_nv2a"
fi

build_type="${BUILD_TYPE:-Release}"
if [ "${DEBUG:-0}" = "1" ]; then
    build_type="RelWithDebInfo"
fi

cmake_extra_flags=()
if [ "$build_type" != "Release" ]; then
    cmake_extra_flags+=(
        "-DCMAKE_C_FLAGS=-g -gcodeview"
        "-DCMAKE_CXX_FLAGS=-g -gcodeview"
        "-DCMAKE_EXE_LINKER_FLAGS=-Wl,--pdb="
    )
fi

cmake -S "$repo" -B "$out/build" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$repo/cmake/toolchains/windows-arm64-llvm-mingw.cmake" \
    -DXBOXRECOMP_DIR="$(cd "${XBOXRECOMP_DIR:-$repo/xboxrecomp}" && pwd)" \
    -DNGB_WIN_DEPS="$deps" -DCMAKE_BUILD_TYPE="$build_type" \
    "${cmake_extra_flags[@]}"

jobs="${JOBS:-$(nproc 2>/dev/null || echo "${NUMBER_OF_PROCESSORS:-4}")}"
ninja -C "$out/build" -j"$jobs" ninja_gaiden_black

pkg=$out/NinjaGaidenBlack
rm -rf "${pkg:?}"
mkdir -p "$pkg/share/NinjaGaidenBlack/shader-seed" "$pkg/licenses"
bin=$deps/$llvm_mingw/bin
llvm_strip="$bin/llvm-strip"
[ -f "$bin/llvm-strip.exe" ] && llvm_strip="$bin/llvm-strip.exe"
llvm_objdump="$bin/llvm-objdump"
[ -f "$bin/llvm-objdump.exe" ] && llvm_objdump="$bin/llvm-objdump.exe"
if [ "$build_type" = "Release" ] && [ "${STRIP:-1}" != "0" ]; then
    "$llvm_strip" -o "$pkg/NinjaGaidenBlack.exe" "$out/build/NinjaGaidenBlack.exe"
else
    cp "$out/build/NinjaGaidenBlack.exe" "$pkg/NinjaGaidenBlack.exe"
    [ -f "$out/build/NinjaGaidenBlack.pdb" ] && cp "$out/build/NinjaGaidenBlack.pdb" "$pkg/"
fi

py_cmd="python3"
command -v python3 >/dev/null 2>&1 || py_cmd="python"

# The DLLs it needs that Windows does not ship: from llvm-mingw and clangarm64 sysroot.
"$py_cmd" "$here/dlldeps.py" "$llvm_objdump" "$pkg/NinjaGaidenBlack.exe" \
    "$deps/$llvm_mingw/aarch64-w64-mingw32/bin" "$deps/sysroot/clangarm64/bin" |
    while IFS= read -r dll || [ -n "$dll" ]; do
        dll="${dll//$'\r'/}"
        [ -n "$dll" ] && cp "$dll" "$pkg/"
    done
cp "$repo/packaging/d3d-seed.bin" "$pkg/share/NinjaGaidenBlack/"
cp "$repo/packaging/shader-seed/states.bin" "$repo/packaging/shader-seed/pipelines.bin" \
   "$pkg/share/NinjaGaidenBlack/shader-seed/"
cp "$repo"/assets/fonts/OFL-*.txt "$repo/assets/prompts/License-Xelu.txt" "$pkg/licenses/"
cp "$repo/LICENSE" "$repo/THIRD_PARTY.md" "$pkg/licenses/"
toolkit=$(cd "${XBOXRECOMP_DIR:-$repo/xboxrecomp}" && pwd)
cp "$repo/src/d3d/NOTICE" "$pkg/licenses/NOTICE-renderer.txt"
cp "$toolkit/LICENSE" "$pkg/licenses/LICENSE-xboxrecomp.txt"
cp "$toolkit/NOTICE" "$pkg/licenses/NOTICE-xboxrecomp.txt"
cp "$toolkit/LICENSES/LGPL-2.1.txt" "$pkg/licenses/"
cat > "$pkg/README.txt" <<'TXT'
Ninja Gaiden Black - native static recompilation for Windows ARM64 (Vulkan)

Adapted for Snapdragon 8cx Gen 3 / WoA devices with Vulkan 1.1+ support.
Supports both Direct3D (Vulkan 1.3+) and NV2A hardware renderer (Vulkan 1.1+).

The game itself is not included. Start NinjaGaidenBlack.exe and choose your own copy of
the retail USA Xbox release: a disc image (.iso, a full Redump dump, or the .7z/.zip it came in)
or a folder of extracted files with default.xbe. A disc image is installed once (about 6 GB) into
%LOCALAPPDATA%\NinjaGaidenBlack\game; saves are kept there too, settings in
%APPDATA%\NinjaGaidenBlack.
TXT
rm -f "$out/NinjaGaidenBlack-windows-arm64.zip"
(cd "$out" && "$tar_cmd" -a -cf NinjaGaidenBlack-windows-arm64.zip NinjaGaidenBlack)
echo "Packed:"; ls "$pkg"
ls -l "$out/NinjaGaidenBlack-windows-arm64.zip"
