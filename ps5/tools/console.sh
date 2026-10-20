#!/usr/bin/env bash
# Burnout 3 for the PlayStation 5 - deploy and run from macOS.
#
#   ps5/tools/console.sh deploy              dist/<TITLE_ID>/ to the console (FTP)
#   ps5/tools/console.sh game-files          "gamedata/Burnout 3 Takedown" (your disc's files) to
#                                            the title folder (files already there at the same size
#                                            are skipped)
#   ps5/tools/console.sh run [SECONDS]       launch it and follow klog (default 30 s)
#   ps5/tools/console.sh env [KEY=VALUE...]  write the title's env.txt (no arguments: remove it)
#   ps5/tools/console.sh args [ARGUMENT...]  write the title's args.txt (no arguments: remove it)
#   ps5/tools/console.sh quit                close the title
#
# Uses the Mac's PS5_Vulkan checkout (../PS5_Vulkan, PS5_HOST in its .env) and
# the console's ftpsrv (2121), klogsrv (3232) and PS5_Vulkan's ps5vkctl (9111).
# The Lima VM cannot reach the console: this runs on the Mac.
#
# SPDX-License-Identifier: MIT
set -euo pipefail
ps5=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
root=$(dirname "$ps5")
vulkan=$(cd -- "${PS5_VULKAN_DIR:-$root/../PS5_Vulkan}" && pwd)
title_id=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["titleId"])' "$ps5/sce_sys/param.json")
title_name=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["localizedParameters"]["en-US"]["titleName"])' "$ps5/sce_sys/param.json")
host=${PS5_HOST:-$(sed -n 's/^PS5_HOST=//p' "$vulkan/.env")}
ftp="ftp://$host:2121/data/homebrew/$title_id"

case ${1:?usage: console.sh deploy|game-files|run|env|args|quit} in
deploy)
    # The art always goes: every 3840x2160 pic0/pic1.dds has the same size, and
    # the tool skips a file whose size matches. The home screen shows
    # /user/appmeta/<TITLE_ID>, copied at registration, so that one is refreshed too.
    art=(sce_sys/icon0.png sce_sys/pic0.dds sce_sys/pic1.dds)
    always=(--always eboot.bin --always code.bin --always code.txt)
    for file in "${art[@]}"; do [[ -f $root/dist/$title_id/$file ]] && always+=(--always "$file"); done
    python3 "$vulkan/tools/deploy-title-folder.py" "${always[@]}" "$root/dist/$title_id"
    for file in "${art[@]}"; do
        [[ -f $root/dist/$title_id/$file ]] || continue
        curl -s -T "$root/dist/$title_id/$file" "ftp://$host:2121/user/appmeta/$title_id/${file#sce_sys/}" 2>/dev/null ||
            true
    done
    ;;
game-files)
    game="$root/gamedata/Burnout 3 Takedown"
    [[ -f $game/default.xbe ]] || { echo "no $game/default.xbe" >&2; exit 2; }
    put() {
        local file=$1 rel=${1#"$game/"} url size have
        url="$ftp/Burnout%203%20Takedown/$(python3 -c 'import sys,urllib.parse; print(urllib.parse.quote(sys.argv[1]))' "$rel")"
        size=$(stat -f %z "$file")
        have=$(curl -s -I "$url" | tr -d '\r' | awk '/Content-Length/ {print $2}')
        [[ $have == "$size" ]] && return
        for _ in 1 2 3; do curl -s --ftp-create-dirs -T "$file" "$url" && return; done
        echo "FAILED $rel" >&2
    }
    export -f put; export game ftp
    find "$game" -type f -print0 | xargs -0 -n1 -P3 bash -c 'put "$0"'
    echo "Burnout 3 Takedown/ sent"
    ;;
run)
    seconds=${2:-30}
    run_dir="$root/klog/$(date +%Y%m%d-%H%M%S)"
    mkdir -p "$run_dir"
    python3 "$vulkan/tools/run-title.py" "$title_id" \
        --until 'exit status|killing coredump|APP_SUSPENDING|Sending signal' --timeout "$seconds" \
        --echo "^\\[$title_name\\]|fatal|Sending signal|coredump" \
        --elf "$root/build/last-title.elf" --output "$run_dir/klog.log" || true
    echo "klog: $run_dir/klog.log"
    ;;
env | args)
    file=$1.txt
    shift
    if [[ $# -eq 0 ]]; then
        curl -s -Q "DELE /data/homebrew/$title_id/$file" "ftp://$host:2121/" > /dev/null || true
        echo "$file removed"
    else
        printf '%s\n' "$@" | curl -s -T - "$ftp/$file"
        printf "$file: %s\n" "$@"
    fi
    ;;
quit)
    printf 'kill %s\n' "$title_id" | nc -w 5 "$host" 9111
    ;;
*)
    echo "usage: console.sh deploy|game-files|run|env|args|quit" >&2
    exit 2
    ;;
esac
