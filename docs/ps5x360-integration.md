# PS5X360 integration boundary

The implemented `src/integration/ps5x360_sample.hpp` converts an optional UCB
state to an upstream-shaped PadSample. It maps Back to touchpad, inverts stick Y
for upstream's native convention, centers disconnected axes at 128, clamps
values, and omits Guide because the inspected upstream button table ignores it.
It does not inject input into an existing process or register an input driver.

The adapter is tied to the fields and button table inspected at PS5X360 commit
`ad628f65ed460416c45103f4867053c5086eab1d`. Changes upstream need review.
Tests use a host struct with the same fields plus independent expected masks;
the entire emulator and its SDK dependencies have not been compiled here.

## Source integration pattern (not applied to PS5X360)

Inside an emulator build with its real PadSample declaration available:

```cpp
#include "bridge/report_session.hpp"
#include "integration/ps5x360_sample.hpp"
#include "xbox360ps5/dualsense_input.hpp"

// Keep these alive in the input owner, on one thread.
ucb::ControllerBridge bridge;
ucb::ReportSession xbox_player2(bridge, 1, 1000);

// On a verified completed controller transfer:
// xbox_player2.receive(buffer, actual_transfer_length, monotonic_ms);
// On each input tick:
// xbox_player2.tick(monotonic_ms);
// On USB removal/error: xbox_player2.disconnect();

// In poll_extra_players, IF slot 1 has been explicitly reserved for UCB:
// const auto sample = slot == 1
//     ? ucb::ps5x360::make_sample<xbox360ps5::PadSample>(bridge.state(1))
//     : pad.Read(slot);
// player_inputs[slot]->Submit(sample);
```

The comments mark the missing transport hookup, not working hardware code.
Use this selection IN PLACE OF that slot's native read, not an extra Submit
followed by native polling. Otherwise native polling overwrites the bridge.
Preserve the rest of poll_extra_players, including connection changes and
profile sign-in/logout. Do not change Player 1's read or UI guide behavior.
Reserve Player 2 explicitly; do not silently take an occupied native slot.

An absent UCB state means the reserved slot is disconnected. Do not automatically
fall back to another native player without a deliberate reassignment policy.
USB acquisition must share the polling thread or publish through synchronization;
ControllerBridge and ReportSession are not thread-safe. One session owns one
slot, and native slot 0 is rejected by ReportSession.

The watchdog expires invalid/no-report streams without renewing them on malformed
packets; clock regressions also disconnect. A 1000 ms default is provisional:
measure whether the real controller produces periodic or change-only reports
before choosing a final policy, so idle or held input is not disconnected.

This adapter quantizes sticks to upstream PadSample's 8 bits. A future dedicated
xe::hid::InputDriver can retain signed 16-bit precision and support deliberate
Guide, keystroke and rumble behavior. The PadSample adapter is a small integration
starting point, not a claim that mixed-controller gameplay now works.

## Remaining console work

1. Demonstrate permitted USB enumeration and interrupt transfers on the PS5.
2. Build a reproducible PS5X360 dependency environment and integrate that backend.
3. Verify actual profile-aware slot 1 delivery while native slot 0 remains active.
4. Test connection/error/reconnect, held inputs, UI capture and sleep/resume.
5. Only then design daemon IPC and settings UI, with measured process lifetime.

No upstream repository or installed emulator was modified.
