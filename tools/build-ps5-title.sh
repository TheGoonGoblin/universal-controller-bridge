#!/usr/bin/env bash
# Build an experimental native home-screen title; no console deployment.
set -euo pipefail
task_root="$(cd "$(dirname "$0")/.." && pwd)"
: "${PS5_PAYLOAD_SDK:?Set PS5_PAYLOAD_SDK to SDK v0.44 or a compatible installation}"
sdk="$PS5_PAYLOAD_SDK"
native="$task_root/third_party/native-title"
work="$task_root/build-title"
title="$task_root/build-title/PPSA73001"
host_cxx="${CXX:-c++}"
mkdir -p "$work/host" "$work/obj" "$title/sce_sys" "$title/sce_module" "$title/licenses"
"$host_cxx" -std=c++20 -O2 "$native/native_app_builder.cpp" "$native/self_container.cpp" \
  "$native/elf_object.cpp" "$native/sce_module_writer.cpp" -lz -o "$work/host/ps5-native-tool"
"$host_cxx" -std=c++20 -O2 "$native/libc_builder.cpp" -o "$work/host/libc-builder"
tool="$work/host/ps5-native-tool"
"$work/host/libc-builder" "$native/runtime/api-surface.txt" "$native/runtime/imports.txt" "$work/libc.raw.elf"
"$tool" self --sign --in "$work/libc.raw.elf" --out "$title/sce_module/libc.prx"
python3 - "$title/sce_module/libc.prx" <<'PY'
import hashlib,sys
assert hashlib.sha256(open(sys.argv[1],'rb').read()).hexdigest()=='e6ff45d16adf687855cc3b33b0c8a4132b6504360b221e0a34c7e99fb3ba0036','Runtime reproduction mismatch'
PY
for item in "src/platform/ps5/native_app.cpp:app" "src/platform/ps5/fp_init.cpp:fp" "third_party/native-title/app_crt.cpp:crt"; do
  source="${item%:*}"; name="${item##*:}"
  "$sdk/bin/prospero-clang++" -std=c++17 -O2 -fno-exceptions -fno-rtti \
    -fdenormal-fp-math=ieee -ffunction-sections -fdata-sections -I"$task_root/src" \
    -c "$task_root/$source" -o "$work/obj/$name.o"
done
"$sdk/bin/prospero-lld" -T "$native/ps5-pie.ld" --eh-frame-hdr --gc-sections \
  --version-script "$native/app-symbols.map" --exclude-libs=ALL -e _start \
  -o "$work/llvm-pie.elf" "$work/obj/crt.o" "$work/obj/fp.o" "$work/obj/app.o" \
  -L "$sdk/target/lib" --start-group -lc++ -lc++abi -lunwind -lc --end-group \
  --as-needed "$sdk"/target/lib/*.so
"$tool" link --in "$work/llvm-pie.elf" --out "$work/eboot.elf" \
  --stub-dir "$sdk/target/lib" --module-sdk 0x02000009 --companion-sdk 0x08050001 --file-name eboot.elf
"$tool" self --sign --in "$work/eboot.elf" --out "$title/eboot.bin" --magic 0x1D3D154F
"$tool" self --inspect --file "$title/eboot.bin" > "$work/eboot-inspection.txt"
"$tool" self --extract --file "$title/eboot.bin" --out "$work/eboot-roundtrip.elf"
python3 "$task_root/tools/verify-native-title.py" "$work/eboot.elf" "$work/eboot-roundtrip.elf"
cp "$task_root/packaging/param.json" "$title/sce_sys/param.json"
cp "$task_root/assets/brand/icon0.png" "$title/sce_sys/icon0.png"
for asset in pic0.png pic1.png; do
  if test -f "$task_root/assets/brand/$asset"; then cp "$task_root/assets/brand/$asset" "$title/sce_sys/$asset"; fi
done
cp "$task_root"/licenses/* "$title/licenses/"
cp "$task_root/README.md" "$title/README.md"
echo "Built native experimental title: $title"
echo "Build/container verification passed. PS5 launch and controller bridging remain unverified."
