#!/usr/bin/env bash
# Burnout 3 for the PlayStation 5 - build the title into dist/<TITLE_ID>/.
#
#   ps5/tools/build.sh [ninja targets]      (default: burnout3, the title)
#
# Applies the console's patches to vendor/takedawn and vendor/xboxrecomp
# (tools/setup-source.sh), configures Takedawn with the payload SDK's CMake
# toolchain (once) and ps5/cmake/ps5-game.cmake, which gives it SDL2 with the
# console's drivers, glslang, RADV's Vulkan headers and the title's link
# (tools/link-title.sh). The translated game code (vendor/takedawn/src/game/recomp/gen)
# comes from your own default.xbe (tools/regen.sh, on the Mac). The button
# prompts are rendered on the Mac too (build/glyphs, tools/glyphs.sh).
#
#   PS5_BUILD_DIR    the build tree (default build/game; the Linux VM's own disk
#                    is several times faster than the shared folder)
#   PS5_VULKAN_DIR   the PS5_Vulkan checkout (default ../PS5_Vulkan)
#
# SPDX-License-Identifier: MIT
set -euo pipefail

ps5=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
root=$(dirname "$ps5")
vulkan=$(cd -- "${PS5_VULKAN_DIR:-$root/../PS5_Vulkan}" && pwd)
bash "$ps5/tools/setup-source.sh"
sdl=$(bash "$ps5/tools/setup-sdl.sh")
glslang=$(bash "$ps5/tools/setup-glslang.sh")
sdk=$(bash "$ps5/tools/setup-sdk.sh")
build=${PS5_BUILD_DIR:-$root/build/game}
ninja=$(command -v ninja || echo "$HOME/.local/bin/ninja")
export PS5_CLANG=${PS5_CLANG:-$(command -v clang || true)}
[[ -f $root/build/glyphs/bo3_glyph_art.c ]] || { echo "no build/glyphs: run ps5/tools/glyphs.sh on the Mac" >&2; exit 2; }

if [[ ! -f $build/build.ninja ]]; then
    mkdir -p "$build"
    cmake -S "$root/vendor/takedawn" -B "$build" -G Ninja -DCMAKE_MAKE_PROGRAM="$ninja" \
        -DCMAKE_TOOLCHAIN_FILE="$sdk/toolchain/prospero.cmake" \
        -DCMAKE_PROJECT_burnout3_recomp_INCLUDE="$ps5/cmake/ps5-game.cmake" \
        -DPS5_PORT_DIR="$root" -DPS5_VULKAN_DIR="$vulkan" -DSDL_DIR="$sdl" -DGLSLANG_DIR="$glslang" \
        -DXBOXRECOMP_DIR="$root/vendor/xboxrecomp" -DBO3_GLYPHS_ART_DIR="$root/build/glyphs" \
        -DBO3_BIN_DIR="$build/bin" \
        -DCMAKE_BUILD_TYPE=Release -DCMAKE_VERBOSE_MAKEFILE=OFF > "$build/configure.log" 2>&1 \
        || { tail -60 "$build/configure.log" >&2; exit 1; }
fi
"$ninja" -C "$build" "${@:-burnout3}"
