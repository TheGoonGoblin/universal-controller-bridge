# PS5 research — Phase 1

Inspected 2026-10-19 UTC. Findings are source observations, not console tests.
User-reported firmware: 13.60. Nothing here independently validates that setup.

## PS5X360: verified source

Snapshot: [ad628f65ed460416c45103f4867053c5086eab1d](https://github.com/BrinooTk/PS5X360/tree/ad628f65ed460416c45103f4867053c5086eab1d).

- [Native entry point](https://github.com/BrinooTk/PS5X360/blob/ad628f65ed460416c45103f4867053c5086eab1d/src/canary_game_main.cpp):
  NativePad initializes scePad, discovers logged-in users, opens native pad
  handles and calls scePadReadState. It constructs PadSample from the read bytes.
  There are four owner/handle slots and four DualSenseInput drivers, each
  constructed with its slot index. poll_extra_players reads and submits slots
  1–3 and manages guest profile connection/logout. Source therefore implements
  a multi-controller path; mixed Xbox USB/native operation is not established.
- [Adapter declaration](https://github.com/BrinooTk/PS5X360/blob/ad628f65ed460416c45103f4867053c5086eab1d/include/xbox360ps5/dualsense_input.hpp)
  exposes DualSenseInput::Submit(PadSample), derives from xe::hid::InputDriver
  and stores a user index, mutex, state and keystroke queue.
- [Adapter implementation](https://github.com/BrinooTk/PS5X360/blob/ad628f65ed460416c45103f4867053c5086eab1d/platform/ps5/dualsense_input.cpp)
  maps native buttons to guest XInput, converts 8-bit native sticks to signed
  16-bit guest values, inverts Y, forwards analog triggers, increments packet
  numbers on changes, and reports disconnected users. GetState and capabilities
  gate on user_. Rumble is an optional callback. Touchpad maps to guest Back;
  no Guide entry appears in the button table.
- [Input contract test](https://github.com/BrinooTk/PS5X360/blob/ad628f65ed460416c45103f4867053c5086eab1d/src/input_contract.cpp)
  exercises the router but instantiates only one driver. Its “second controller
  absent” assertion is not proof that the active application's four-driver
  path is broken. This upstream test was read, not run here.

**Engineering inference:** adding an in-process xe::hid::InputDriver using UCB
states is the most practical first integration. Submit is a source-level method,
not a cross-process input injection service. No external controller-provider
protocol was identified in the inspected input path. Integration requires a
rebuilt emulator or agreed provider interface; it cannot be achieved merely by
running the current UCB CLI beside the installed emulator. A direct guest adapter
would retain 16-bit stick precision and avoid PlayStation mappings specific to
the UI. It must coordinate native slot ownership, profiles, UI focus and
disconnects. No emulator code was modified.

[PS5X360 LICENSE](https://github.com/BrinooTk/PS5X360/blob/main/LICENSE)
is MIT for original code. Its README explicitly retains third-party component
licenses; those must be reviewed before redistributing an integrated build.

## Toolchain, USB and background execution

[ps5-payload-dev/sdk](https://github.com/ps5-payload-dev/sdk/tree/f5d40f02770e9b0001dd9e18056eca70b3da8b40):
its README documents LLVM 18, ELF loaders, macOS Homebrew prerequisites,
LLVM_CONFIG on /opt/homebrew, PS5_PAYLOAD_SDK, and architecture-independent SDK
distribution. This is a credible Apple Silicon cross-build starting point,
not proof of UCB compilation or firmware 13.60 compatibility.

The SDK tree includes FreeBSD USB and libusb headers.
[libusb.h](https://github.com/ps5-payload-dev/sdk/blob/f5d40f02770e9b0001dd9e18056eca70b3da8b40/include/freebsd/libusb.h)
declares libusb_init, libusb_get_device_list, libusb_open,
libusb_claim_interface and libusb_interrupt_transfer.
**Declarations do not establish a linkable backend, usable device nodes,
permissions or working PS5 transfers.** No PS5 USB enumeration implementation
has been verified here. An attempted sce_stubs/libSceUsbd.c fetch returned 404;
etaHEN's tree contains a libSceUsbd.so file, which is not sufficient API evidence.
Do not invent sceUsbd call signatures or assume ordinary Linux libusb works.

[etaHEN snapshot](https://github.com/etaHEN/etaHEN/tree/dafa13b562ddb137a4b4a97b9aaa287c0c57cc9c)
README advertises plugins, payload auto-start options, two daemons and an ELF
loader; its source tree includes daemon components. This supports investigating
background execution but does not establish UCB USB ownership, app-close
survival, suspend/resume behavior or firmware support. The linked
LightningMods/etaHEN-SDK README returned 404 during this inspection, so no
plugin SDK contract was verified.

System-wide virtual DualSense: no supported creation/injection API was verified.
FreeBSD uinput headers in an SDK are not evidence of Sony ScePad integration.
Defer this route until a functioning implementation and required permissions
are demonstrated.

## Wired Xbox 360 protocol

[Linux xpad.c](https://github.com/torvalds/linux/blob/master/drivers/input/joystick/xpad.c),
blob 810e52782ae891f3273598474b66ac89db13a6c8, was examined as protocol evidence:
xpad360_process_packet reads D-pad/Menu/View/stick-click bits from byte 2,
bumpers/Guide/face bits from byte 3, triggers from 4/5, sticks at 6/8/10/12.
It checks report type 0. Its probe locates interrupt endpoints rather than
assuming a universal endpoint address. Original wired Microsoft VID/PID is
045e:028e; descriptor/interface validation is still required.
UCB implements its own small decoder, not copied Linux driver code.
xpad is GPL-licensed; do not incorporate driver code under UCB's MIT notice.
UCB intentionally requires the common exact 20-byte 00 14 report and needs
real captures before relaxing this policy. Wireless receiver framing differs.

## Packaging and firmware 13.60

[PS5X360 build notes](https://github.com/BrinooTk/PS5X360/blob/ad628f65ed460416c45103f4867053c5086eab1d/docs/BUILD.md)
say its native title needs a prepared SDK, PS5_Vulkan tree, runtime staging and
LLVM 18; a fresh checkout is not a self-contained one-command build. The
script produces eboot.elf and eboot.bin with executable validation.

[Release packager](https://github.com/BrinooTk/PS5X360/blob/ad628f65ed460416c45103f4867053c5086eab1d/tools/package-release.py)
requires eboot.bin, sce_module/libc.prx, sce_sys/param.json and artwork, plus
its own fonts/sounds. These are that application's requirements, not a universal
minimum for every homebrew title. Its README instructs copying PPSA50011 to
/data/homebrew/PPSA50011 and letting ShadowMountPlus register it. It reports a
development console on 13.60 with etaHEN and kstuff. This is an upstream claim,
not our verification or evidence that every homebrew tool supports 13.60.
PPSA50011 belongs to that emulator; do not reuse it for UCB.

No UCB title ID, mount recipe, executable, fake package or console installation
was generated. Before packaging: verify the actual toolchain/runtime,
application metadata schema, registration mechanism, distinct title identity
and all dependency licenses. Until then the only testable deliverable is the
portable host code.

## Validation and remaining gates

Host: GCC 13.3.0 Linux x86-64; CMake Release build, CTest 1/1 and
1,311,321 checks pass. ASan/UBSan pass with ASAN_OPTIONS=detect_leaks=0;
the initial LeakSanitizer run failed because process inspection is restricted,
not because a program leak was diagnosed. No Apple Silicon build, USB device,
PS5 SDK build, emulator integration, rumble, Bluetooth or console gameplay test
was performed.

Next hardware evidence needed: descriptor dump, accessible backend and transfer
error codes, captured wired reports, disconnect/reconnect behavior, concurrent
native pad polling, and profile-aware Player 2 delivery in a rebuilt emulator.

