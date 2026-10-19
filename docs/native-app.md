# UCB native settings application — experimental build

**Newer packaging:** [home-screen preview and installation](home-screen-install.md)
now provides a converted native title folder and optional verified FTP transfer.
This page retains the earlier ELF-payload launch instructions. Neither build has
been executed on a PS5 here; Xbox bridging remains unfinished.

The native application now cross-compiles to `ucb-settings.elf` using
ps5-payload-dev SDK v0.44 and Clang/LLD 18.1.3. This is a real PS5-targeted ELF
payload with native display and controller calls, not a renamed host executable.
It has not been executed on a PS5. A successful link does not establish that the
loader, display ownership, pad access or filesystem permissions work on 13.60.

## Implemented screens

- Overview: native pad status; explicit unavailable USB/emulator/service status.
- Settings: change left/right dead zones and intended Xbox player 2–4 assignment.
- Mapping: show the fixed Xbox-to-PS5X360 profile.
- Input test: display the native Sony controller's buttons, sticks and triggers.

Controls: L1/R1 change tabs; D-pad up/down chooses a setting; left/right adjusts
it; Cross saves; Circle exits. Config is stored in `/data/ucb/settings.conf` if
the running payload has permission. Settings are saved preferences for future
integration; the current app does not change an installed emulator's input.

## What is not in this build

Xbox USB acquisition on PS5, controller injection, PS5X360 runtime hookup,
custom remapping, Bluetooth pairing, daemon persistence and a home-screen title
are not implemented. The native Sony pad is used for the interface and input
test; the app does not claim it detected an Xbox controller. Earlier host USB
diagnostics remain a separate computer program.

## Build on Apple Silicon

Install Xcode Command Line Tools, Homebrew LLVM 18 and the official payload SDK:

```sh
brew install llvm@18 wget
export LLVM_CONFIG="$(brew --prefix llvm@18)/bin/llvm-config"
# Download and extract ps5-payload-sdk.zip from upstream SDK release v0.44.
# Preserve executable permissions on the SDK bin scripts.
export PS5_PAYLOAD_SDK=/absolute/path/to/ps5-payload-sdk
bash tools/build-ps5-app.sh
```

[SDK v0.44 distribution](https://github.com/ps5-payload-dev/sdk/releases/tag/v0.44)
and [complete SDK source](https://github.com/ps5-payload-dev/sdk/archive/refs/tags/v0.44.zip).
This exact Apple Silicon build has not been run here. The packaged binary was
cross-built on Linux using the same SDK compiler wrapper and Clang 18.1.3.

## Console launch: experimental ELF path

Use an ELF loader already working with your etaHEN setup. For a loader listening
on port 9021, the SDK documents deployment using `prospero-deploy`:

```sh
"$PS5_PAYLOAD_SDK/bin/prospero-deploy" -h YOUR_PS5_IP -p 9021 build-ps5/ucb-settings.elf
```

Replace the IP and port with those of your loader. The binary is an ELF payload;
copying it into `/data/homebrew/PPSA...` will not register a home-screen app.
Do not rename it `eboot.bin`. PS5 Upload support for this specific ELF is not
verified, so use its ELF mode only if that mode is documented by your tool.
The first hardware check is simply whether the screen opens and Circle exits.
No remote deployment was attempted here.

A proper registered home-screen title needs a distinct title ID, title metadata,
runtime packaging and an executable built for that registration path. Those
pieces have not been validated for UCB; a dummy PPSA folder is not shipped.

## Validation

- Cross-compilation and linking against actual SDK stubs succeeded.
- ELF header inspected: x86-64, 64-bit, PIE with SDK linker layout and native
  SceVideoOut/ScePad/SceUserService/SceSystemService imports.
- Host renderer generated all four screens; layouts were visually checked.
- App tests cover settings bounds, persistence, invalid configs and all 2,073,600
  visible pixels' unique, bounded tiled-memory addresses (4,147,220 checks).
- Existing decoder, integration and simulated USB suites still pass (4/4 CTest).
- None of this substitutes for console testing.

## Source and licenses for the binary

Original UCB source stays MIT. The compiled ELF links SDK runtime code under
GPLv3+ and LLVM C++ runtimes under Apache 2.0 with LLVM exceptions / their
included notices. Distribute this combined experimental binary under GPLv3+
with `licenses/` and the complete corresponding source references below.

- UCB application and build source: this repository's `src/`, `tools/`,
  `CMakeLists.txt`, tests, and source files included in the experimental bundle.
- SDK v0.44 corresponding source is available from the source archive linked
  above and in the bundle's `SOURCE_LINKS.txt`, alongside the binary download.
  Its runtime, linker scripts and compiler wrappers are the source of the
  linked SDK components; the SDK binary distribution is available above.
- SDK `libcxx.sh` pins LLVM 18.1.8 for libc++, libc++abi and libunwind:
  [LLVM runtime source](https://github.com/llvm/llvm-project/releases/download/llvmorg-18.1.8/llvm-project-18.1.8.src.tar.xz).
  This Apache/LLVM source is available from the upstream copying facility.
- Display setup and tiling follow the zlib-licensed
  [SDL PS5 port](https://github.com/ps5-payload-dev/SDL/tree/release-2.30.x-ps5).
  `tile_offset.hpp` is an explicitly marked scalar adaptation retaining its
  notice. All other UCB UI code is original.

No Sony firmware, proprietary SDK, games, BIOS or keys are included.
