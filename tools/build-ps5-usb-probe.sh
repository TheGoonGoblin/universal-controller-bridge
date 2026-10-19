#!/usr/bin/env bash
set -euo pipefail
: "${PS5_PAYLOAD_SDK:?Set PS5_PAYLOAD_SDK}"
root="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$root/build-probe"
"$PS5_PAYLOAD_SDK/bin/prospero-clang++" -std=c++17 -O2 -Wall -Wextra -Werror -I"$root/src" \
 "$root/src/platform/ps5/usb_probe.cpp" -o "$root/build-probe/ucb-usb-probe.elf"
echo "Built experimental USB probe; hardware operation remains unverified."
