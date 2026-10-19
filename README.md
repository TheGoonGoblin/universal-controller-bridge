# universal-controller-bridge
Open-source controller bridge research for PS5 homebrew

## v0.2 development progress

New: an optional **host libusb diagnostic**, an upstream-shaped PS5X360 sample
adapter, descriptor filters, and report sessions with disconnect timeouts.
These are tested development components, not a completed v0.2 PS5 release.

[Build and try USB diagnostics on your Mac](docs/usb-diagnostics.md) ·
[PS5X360 integration boundary and remaining work](docs/ps5x360-integration.md)

The updated suite passes 3/3 CTest targets when USB tests are enabled: the
original 1,311,321 checks, 556 integration checks and 12 simulated USB scenarios.
The real USB executable links here but cannot initialize USB in this container.
Physical USB reads, PS5 operation and Apple Silicon remain unverified.

## Universal Controller Bridge v0.1

Portable C++17 decoder and semantic controller mapping for a common original
Microsoft wired Xbox 360 USB input report. MIT licensed. PS5/etaHEN integration
is research-stage: the v0.1 baseline does not access USB, create a virtual DualSense,
run a background service, or install on a PS5.

### Build and test

macOS: install Xcode Command Line Tools (`xcode-select --install`) and CMake
(`brew install cmake`, if using Homebrew). Linux: C++17 compiler and CMake >=3.16.

```sh
git clone https://github.com/TheGoonGoblin/universal-controller-bridge.git
cd universal-controller-bridge
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

Without CMake:
```sh
c++ -std=c++17 -Wall -Wextra -Wpedantic -Isrc tests/test_xbox360.cpp -o /tmp/ucb_test
/tmp/ucb_test
c++ -std=c++17 -Isrc src/main.cpp -o /tmp/ucb_decode
```

`build/ucb_decode report.bin` decodes exactly one captured 20-byte report and
prints semantic PlayStation controls. It reads a file, not a connected controller.
Do not rename this host executable to eboot.bin.

### Implemented

- All face, directional, shoulder, stick-click, Menu/View and Guide buttons.
- Signed little-endian sticks, 8-bit triggers, configurable rescaled per-axis dead zones.
- A→Cross, B→Circle, X→Square, Y→Triangle; LB/RB→L1/R1;
  stick clicks→L3/R3; Start→Options, Back→Create, Guide→PS.
- Manufacturer-neutral state and four independent optional player slots.
- Exact report-length/type checks; no stale global controller state.

Back→Create is a generic semantic mapping. PS5X360 currently uses touchpad for
guest Back, so an emulator-specific adapter must override this choice.
These names and bit values are UCB's own representation, not a Sony ABI.

### Validation

Linux x86-64, GCC 13.3.0: Release CMake build and CTest passed (1/1).
The test executable passed **1,311,321 checks**, including exhaustive stick
ranges and all trigger values. AddressSanitizer and UndefinedBehaviorSanitizer
passed with leak detection disabled because LeakSanitizer cannot inspect this
container's processes. Apple Silicon and PS5 hardware were not tested.

See [architecture](docs/architecture.md), [verified research and open questions](docs/ps5-research.md),
and [roadmap](docs/roadmap.md). Initial goal: native DualSense Player 1 and wired
Xbox 360 Player 2 in PS5X360; this combination is not implemented yet.

