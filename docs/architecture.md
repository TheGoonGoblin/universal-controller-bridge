# Architecture

## Implemented portable path

A caller supplies a completed USB report → xbox360::decode → ControllerState →
map_playstation → PlayStationState. The CLI exercises this path from a binary file.

The decoder accepts only the common 20-byte wired 00 14 report. Bytes 2–3 hold
buttons; 4–5 triggers; 6–13 signed little-endian sticks. Bytes 14–19 are ignored.
Wireless receiver reports, Xbox One and arbitrary HID devices are not supported.
Reserved button bit 11 is ignored. Contradictory D-pad bits are retained for
adapter policy rather than silently discarded. The decoder rejects a null pointer,
wrong length, wrong type, or wrong declared length.

Stick output is [-1,1], right/up positive; triggers [0,1]. Dead zones default
to 0.15 per axis, with magnitudes inside the zone zeroed and the remaining range
rescaled to reach full travel. This is axial, not radial filtering. Invalid
thresholds throw invalid_argument. No trigger threshold is applied.

ControllerBridge owns four independent optional states. Slots are zero-based:
0 is the initial native DualSense reservation, 1 the initial Xbox target.
Absent means disconnected. It is single-threaded; a future USB worker must
synchronize publication. submit does not assign physical devices automatically.
disconnect clears input. A future transport must clear on removal/error or
timeout; malformed reports alone must not be interpreted as releases.

## Planned boundaries (not PS5 implementations)

- Detection/transport: enumerate VID/PID, inspect configuration/interface and
  interrupt endpoints, obtain access, read completed transfers, handle hotplug.
  matches(045e,028e) is only a candidate predicate, not device enumeration.
- Translation: controller-specific decoders publish manufacturer-neutral state.
- Mapping: configurable profiles and calibration; current mapping is fixed.
- Integration: prefer an in-process PS5X360 input provider; preserve signed
  16-bit precision by using its guest input driver interface rather than an
  unnecessary 8-bit PlayStation round trip. Add slot arbitration so native
  polling never overwrites the bridged player and never steals Player 1.
- Service: later, only after USB and lifetime behavior are measured. Separate
  settings UI would need authenticated/local IPC, state versioning, connection
  timestamps, timeout neutralization, and clean shutdown/reconnect.
- Settings: later, remapping, calibration, status and player assignment.

No PS5 USB backend, IPC protocol, emulator patch, daemon or GUI is shipped.
Guide→PS is semantic only; the current PS5X360 button table has no Guide mapping.
Future emulator integration must choose a deliberate guide/UI policy and respect
UI capture, guest connection status, profiles, keystrokes and rumble capabilities.

## v0.2 development components

An optional host libusb diagnostic now implements descriptor-based interface
selection and bounded reads. ReportSession publishes decoded reports to one
explicit non-native slot and clears disconnected/stale inputs. The templated
PS5X360 sample adapter implements that emulator's native bit conventions rather
than treating the generic PlayStation mapping as a ScePad report. See
[diagnostics](usb-diagnostics.md) and [integration](ps5x360-integration.md).

