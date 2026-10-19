# USB diagnostic — v0.2 development

This is a host libusb program, not a PS5 payload. It lists candidates matching
the original wired Microsoft controller (045e:028e), inspects the active USB
configuration, and can claim the vendor-specific gamepad interface for a bounded
read. It discovers interrupt IN endpoints and supplies actual transfer lengths
to the decoder. No control/output transfers, driver detach, reset, configuration
change, Bluetooth or rumble are implemented. Claiming an interface can still
fail if the OS owns it; the tool reports that error rather than forcing access.

## Apple Silicon Mac

With Homebrew and Xcode Command Line Tools already installed:

```sh
brew install cmake libusb
git clone https://github.com/TheGoonGoblin/universal-controller-bridge.git
cd universal-controller-bridge
cmake -S . -B build-usb -DUCB_BUILD_USB=ON -DCMAKE_PREFIX_PATH="$(brew --prefix)"
cmake --build build-usb
ctest --test-dir build-usb --output-on-failure
./build-usb/ucb_usb --list
```

For an existing checkout, run `git pull --ff-only` instead of cloning again.
Connect the original wired controller directly or through an appropriate USB
adapter. If a candidate appears, use its printed decimal bus/address values:

```sh
# Example only: replace 1 and 2 with the values printed for your controller.
./build-usb/ucb_usb --read 1 2 15
```

Move both sticks, press buttons and squeeze triggers during those 15 seconds.
The tool prints raw report hex and translated PS5X360 sample values; it does
not control your PS5 or any game on the Mac. Read duration accepts 1–60 seconds.
Do not launch ucb_usb_fixture as a device tool: it is a simulated test program.

Linux development packages are commonly `libusb-1.0-0-dev` and `cmake`.
Access permissions and existing drivers can prevent reads on either host.
Exit codes: 0 enumeration/read success; 1 backend/device failure; 2 bad arguments;
3 requested device absent or no valid reports. Successful enumeration is not
proof that the controller was present: check for a `candidate` line.

## Validation in the development container

- GCC 13.3.0 compiled and linked the real diagnostic against the installed Linux
  libusb-1.0.so.0. Its development header was fetched from upstream libusb
  (blob 7c3d830b0e646e1aec45b661caea16cc8ac51fc9); no header was vendored.
- Real `--list` returned `libusb_init: LIBUSB_ERROR_OTHER` in the restricted
  container. No physical device enumeration or transfer succeeded here.
- All 3 CTest targets passed, including 12 simulated USB CLI scenarios for
  valid reports, malformed lengths, unplug, access denied, busy interface,
  unsupported descriptor, absent device, init failure and argument validation.
- 556 new integration checks passed alongside the original 1,311,321 decoder
  checks. New integration tests also passed ASan/UBSan with leak detection off.
- The simulated backend checks a discovered endpoint of 0x83 so a hardcoded
  0x81 endpoint would fail. It is not a PS5 API mock or console validation.
- macOS and PS5 hardware remain untested.

## PS5 gate

This code deliberately uses real libusb symbols rather than guessed sceUsbd
signatures. It cannot be declared a usable PS5 backend until a linkable library,
correct ABI and permitted device access are demonstrated. No PS5 binary is
produced by these build commands. A Mac capture verifies the controller and
report decoder, not console USB access.
