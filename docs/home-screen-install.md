# UCB home-screen preview — installation

This download contains a real native title candidate, `PPSA73001`, not a renamed
ELF payload. Its native executable, generated runtime, metadata and icon were
built and checked on the development computer. **PS5 launch on 13.60 has not
been tested. Xbox USB bridging and PS5X360 input delivery are still unfinished.**
Installing the settings preview does not make an Xbox controller work in games.

## Easiest path with PS5 Upload

1. Download `UCB_PS5_HomeScreen_Preview.zip` and extract it on your Mac.
2. With your usual etaHEN/homebrew environment running, use PS5 Upload to copy
   the complete `PPSA73001` folder to `/data/homebrew/PPSA73001`.
3. Use your existing ShadowMountPlus title-registration/refresh procedure, as
   you do for PS5X360. The app is named **Universal Controller Bridge - Preview**.
4. Launch it and check that its screen appears. L1/R1 switches tabs; Circle exits.

Do not nest the folder twice, and do not copy only eboot.bin. If PPSA73001 already
belongs to a different app on your console, do not overwrite it.
The install ZIP contains only PPSA73001. Source and optional developer/FTP tools
are kept separately in the GitHub repository; they are not needed for the
folder-copy installation.

## First hardware test

Confirm the icon/title registers, the window opens, the DualSense navigates all
five tabs, the input-test bars respond, saving reports success or a clear error,
and Circle closes the app. If it fails, record where it stopped and any loader
error. These are still experimental console behaviors, not verified results.

## Build and test evidence

- Native title conversion, container integrity, entry point and loaded-byte
  roundtrip validation passed. The container format intentionally omits a final
  non-loadable note; the verifier checks that all executable/data bytes match.
- The generated libc.prx matches the upstream reproducible SHA-256
  e6ff45d16adf687855cc3b33b0c8a4132b6504360b221e0a34c7e99fb3ba0036.
- Five host CTest targets pass, including an actual local FTP server test.
- UI screens are host-rendered and visually checked; this is not a PS5 screenshot.
- Runtime execution, loader registration and USB access need console testing.

Native title tools are vendored with provenance under third_party/native-title.
The original source stays MIT; linked/vendored components retain their own
licenses and the combined binary is distributed under GPLv3+. Build with
`tools/build-ps5-title.sh`; package with `tools/package-home-screen.py`.
