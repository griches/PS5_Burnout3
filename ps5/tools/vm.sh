#!/usr/bin/env bash
# Burnout 3 for the PlayStation 5 - run a build tool inside the Linux VM, from macOS.
#
#   ps5/tools/vm.sh build [ninja arguments]   ps5/tools/build.sh (the title, into dist/<TITLE_ID>/)
#   ps5/tools/vm.sh TOOL [arguments]          any other ps5/tools/TOOL.sh
#
# The stack (PS5_Vulkan with its RADV archive, PS5_Mesa, PS5_PayloadSDK) lives in
# the Lima VM "ps5" under ~/ps5, built with LLVM 22 (~/ps5/env.sh); this folder is
# mounted there at the same path. The build tree goes on the VM's own disk
# (~/ps5/build/PS5_Burnout3-game): the shared folder is several times slower.
# The console is reached from the Mac (ps5/tools/console.sh), not from the VM.
#
# SPDX-License-Identifier: MIT
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
tool=${1:?usage: vm.sh build|TOOL [arguments]}
shift
exec limactl shell --workdir / ps5 -- bash -lc '. ~/ps5/env.sh; export PS5_VULKAN_DIR=~/ps5/PS5_Vulkan PS5_PAYLOAD_SDK_FORK=~/ps5/PS5_PayloadSDK PS5_BUILD_DIR=${PS5_BUILD_DIR:-$HOME/ps5/build/PS5_Burnout3-game}; cd "$1"; tool=$2; shift 2; exec bash "ps5/tools/$tool.sh" "$@"' _ "$root" "$tool" "$@"
