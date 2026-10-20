#!/usr/bin/env bash
# Burnout 3 for the PlayStation 5 - zip dist/PPSA99316/ as a ready-made release.
#
#   ps5/tools/package-release.sh VERSION    -> dist/Burnout3-PS5-<VERSION>.zip
#
# The title as it is on the console, with an empty dump/ where the player puts
# their own disc image; the first start installs the game data from it
# (ps5/src/firstrun.c). Nothing from the disc or a player goes in: this checks
# that before zipping.
#
# SPDX-License-Identifier: MIT
set -euo pipefail
root=$(cd "$(dirname "$0")/../.." && pwd)
version=${1:?usage: package-release.sh VERSION (e.g. 0.2.0)}
title=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["titleId"])' "$root/ps5/sce_sys/param.json")
dist="$root/dist/$title"
[[ -f $dist/eboot.bin ]] || { echo "package-release.sh: build first (no $dist/eboot.bin)" >&2; exit 2; }

found=$(find "$dist" \( -iname '*.xbe' -o -iname '*.iso' -o -iname '*.xiso' -o -iname '*.img' \
    -o -name 'Burnout 3 Takedown' -o -name .complete -o -name install.journal -o -name userdata \
    -o -name cache -o -name settings.toml -o -name env.txt -o -name args.txt \) -print)
[[ -z $found ]] || { echo "package-release.sh: game or player files in $dist:" >&2; echo "$found" >&2; exit 2; }

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
cp -R "$dist" "$work/$title"
mkdir -p "$work/$title/dump"
cat > "$work/$title/dump/PUT YOUR DISC HERE.txt" <<'TXT'
Put your own image of the North America (USA) Xbox disc of Burnout 3: Takedown
in this folder (.iso or .xiso, any file name), then start the game. The first
start checks the disc and sets up the game data (a few minutes, once); after
that you can delete the image.

An extracted copy of the disc works too: the folder that holds default.xbe,
put here (or named "Burnout 3 Takedown" beside eboot.bin).
TXT
zip_path="$root/dist/Burnout3-PS5-$version.zip"
rm -f "$zip_path"
(cd "$work" && COPYFILE_DISABLE=1 zip -q -r -X "$zip_path" "$title")
echo "$zip_path ($(du -h "$zip_path" | cut -f1), sha256 $(shasum -a 256 "$zip_path" | cut -d' ' -f1))"
