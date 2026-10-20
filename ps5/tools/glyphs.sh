#!/usr/bin/env bash
# Burnout 3 for the PlayStation 5 - render Takedawn's button prompts (on the Mac).
#
#   ps5/tools/glyphs.sh      assets/glyphs/*.svg -> build/glyphs/bo3_glyph_art.{c,h}
#
# The prompts for a DualSense among them. tools/glyphs/build.py needs resvg; the
# VM has no renderer, so the build takes the rendered sources from build/glyphs.
# Python: vendor/takedawn/.venv (python3 -m venv .venv && .venv/bin/pip install capstone resvg-py).
#
# SPDX-License-Identifier: MIT
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
python=$root/vendor/takedawn/.venv/bin/python3
mkdir -p "$root/build/glyphs"
"$python" "$root/vendor/takedawn/tools/glyphs/build.py" "$root/vendor/takedawn/assets/glyphs" \
    "$root/build/glyphs/bo3_glyph_art.c" "$root/build/glyphs/bo3_glyph_art.h"
