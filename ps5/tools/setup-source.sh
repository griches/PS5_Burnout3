#!/usr/bin/env bash
# Burnout 3 for the PlayStation 5 - Takedawn and its toolkit, with the console's changes.
#
#   ps5/tools/setup-source.sh          apply ps5/patches/{takedawn,xboxrecomp}.patch
#   ps5/tools/setup-source.sh --save   write the trees' changes back to the patches
#
# vendor/takedawn (the game's port) and vendor/xboxrecomp (the recompiler and the
# Xbox runtime, its takedawn branch) are upstream at a pinned revision. What the
# console needs changed is kept here as a patch per tree, not as a fork. A patch
# already applied is left alone, so this can run before every build; a build
# refuses to start over edits not yet saved.
#
# SPDX-License-Identifier: MIT
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)

# the tree's changes, new files too (plain git diff leaves out untracked ones);
# generated code, build output and the disc are ignored by the trees' own .gitignore
tree_diff() {
    git -C "$1" add --intent-to-add -A -- .
    git -C "$1" diff --ignore-submodules -- .
    git -C "$1" reset -q
}

for name in takedawn xboxrecomp; do
    tree="$root/vendor/$name"
    patch="$root/ps5/patches/$name.patch"
    [[ -d $tree/.git || -f $tree/.git ]] || git -C "$root" submodule update --init "vendor/$name"
    if [[ ${1:-} == --save ]]; then
        tree_diff "$tree" > "$patch"
        echo "setup-source.sh: saved $name.patch ($(wc -l < "$patch") lines)" >&2
        continue
    fi
    [[ -s $patch ]] || continue
    if git -C "$tree" apply --reverse --check "$patch" 2>/dev/null; then
        # applied already: refuse edits made since that are not in the patch
        if ! cmp -s <(tree_diff "$tree") "$patch"; then
            echo "setup-source.sh: vendor/$name has edits not in the patch: run ps5/tools/setup-source.sh --save" >&2
            exit 2
        fi
        continue
    fi
    git -C "$tree" apply --check "$patch" || {
        echo "setup-source.sh: $name.patch does not apply to $tree (a different upstream revision?)" >&2
        exit 2
    }
    git -C "$tree" apply --whitespace=nowarn "$patch"
    echo "setup-source.sh: applied $name.patch" >&2
done
