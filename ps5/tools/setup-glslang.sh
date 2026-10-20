#!/usr/bin/env bash
# Burnout 3 for the PlayStation 5 - glslang at a pinned release.
#
#   ps5/tools/setup-glslang.sh   fetch it into .deps/glslang (once a release), print the path
#
# The toolkit's Vulkan backend writes the NV2A's vertex programs and register
# combiners as GLSL at run time and compiles them with glslang's C interface.
# Built without the SPIR-V optimiser (ENABLE_OPT=OFF), as the Android build is,
# so no SPIRV-Tools.
#
# SPDX-License-Identifier: MIT
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
glslang="$root/.deps/glslang"
tag=16.6.0

if [[ ! -f $glslang/.ps5-glslang || $(<"$glslang/.ps5-glslang") != "$tag" ]]; then
    {
        rm -rf "$glslang"
        git clone -q --depth 1 -b "$tag" https://github.com/KhronosGroup/glslang.git "$glslang"
    } >&2
    echo "$tag" > "$glslang/.ps5-glslang"
fi
echo "$glslang"
