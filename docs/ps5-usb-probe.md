# Experimental PS5 wired Xbox 360 input probe

This is a separate, bounded developer payload, not an updated home-screen app
and not working emulator controller support. It has cross-compiled against PS5
payload SDK v0.44; operation on firmware 13.60 still needs hardware evidence.
The existing native settings app has now been reported launching by its user,
with the Sony pad shown connected. Xbox input is still absent from that app.

## Run without reinstalling the settings app

1. Close UCB and any running game. Leave PPSA73001 installed.
2. Connect the original Microsoft wired Xbox 360 controller.
3. Use your existing PS5 Upload payload sender to send `ucb-usb-probe.elf`
   to the console's already-running ELF loader. Do not copy this ELF over
   `eboot.bin`, and do not register another home-screen title.
4. During the next 60 seconds, move both sticks and press buttons/triggers.
   Back + Start together ends the read test early. There is no on-screen UI.
5. Download `/data/ucb/usb-probe.log` through PS5 Upload. A log with INPUT lines
   proves USB reports reached the decoder; it does not prove emulator support.

The payload does not request a restart or change the jailbreak, firmware,
ShadowMount configuration, emulator files, or native controller assignments.
It cannot guarantee a system-level fault never occurs on untested hardware.
If a loader is unavailable, report that; do not restart merely to run this test.

## What it does

- Scans `/dev/ugen0.1` through `/dev/ugen15.127`, logging accessible devices
  and access failures. This bounded scan is not a complete USB topology API.
- Reads descriptors before configuring or claiming anything. Only VID/PID
  045e:028e and interface FF/5D/01 with a valid interrupt-IN endpoint qualify.
- Rejects truncated descriptors, alternate settings and unsafe packet lengths.
- Selects configuration index zero only if it is not already active, claims
  that interface and queues bounded USB_FS interrupt transfers using SDK types.
- Decodes exact 20-byte input reports with the existing tested decoder.
- Logs button changes and analog values. Closes USB transfer resources and
  releases its interface on ordinary exit, including setup failures.
- Uses a process-held exclusive file lock to prevent two probe instances.
- Does not detach drivers, send controller initialization/rumble commands,
  patch kernel code or create virtual controllers. Device configuration changes
  are not restored: original inactive configuration restoration is not yet
  established. Unplugging the Xbox controller resets its USB connection.

The 60-second limit bounds the transfer loop, not the operating system's ioctl
execution time. Timeout status is logged and retried; a fatal call error exits.
Current log is replaced on each successful probe start.

## Build and validation

```sh
export PS5_PAYLOAD_SDK=/path/to/ps5-payload-sdk
bash tools/build-ps5-usb-probe.sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Cross-compilation with warnings as errors passed. All six configured host test
suites passed, including 65,825 descriptor checks and existing decoder,
integration, app, FTP installer and fake host USB tests. Host tests do not
exercise PS5 ioctls. A descriptor/access log from the console is required next.

## Research and licensing

FGG-XSense, commit bfc5f9d324e2ca8b0a90fc498957699caa9fe5b0:
https://github.com/FGGstore/FGG-XSense/tree/bfc5f9d324e2ca8b0a90fc498957699caa9fe5b0

Its `src/gip.c` provides a concrete PS5 USB_FS device access example. It targets
Xbox One/Series GIP and explicitly excludes Xbox 360. Its reported hardware
validation is firmware 11.60, not evidence for this payload on 13.60. This UCB
probe is an original implementation using SDK FreeBSD structures; no upstream
GIP driver or virtual-pad code is included. The existing Xbox 360 protocol
and decoder remain separate from GIP. This supersedes the earlier research
statement that there was no concrete PS5 USB transport example available.

SDK USB ABI definitions:
https://github.com/ps5-payload-dev/sdk/tree/v0.44

The optional virtual-pad path described by FGG-XSense is a new research lead,
not established UCB functionality. Player 2/user assignment and compatibility
with PS5X360 remain untested. Do not use a hardcoded user ID as a substitute
for verified player assignment.
