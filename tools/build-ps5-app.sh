#!/usr/bin/env bash
set -euo pipefail
task_root="$(cd "$(dirname "$0")/.." && pwd)"
: "${PS5_PAYLOAD_SDK:?Set PS5_PAYLOAD_SDK to an installed ps5-payload-dev SDK}"
task_cxx="$PS5_PAYLOAD_SDK/bin/prospero-clang++"
test -x "$task_cxx" || { echo "SDK C++ compiler wrapper missing" >&2; exit 1; }
mkdir -p "$task_root/build-ps5"
"$task_cxx" -std=c++17 -O2 -Wall -Wextra -I"$task_root/src" \
  "$task_root/src/platform/ps5/native_app.cpp" \
  -lSceVideoOut -lScePad -lSceUserService -lSceSystemService \
  -o "$task_root/build-ps5/ucb-settings.elf"
echo "Built experimental PS5 ELF: $task_root/build-ps5/ucb-settings.elf"
echo "Not a home-screen title. Console execution and USB bridging are unverified."
