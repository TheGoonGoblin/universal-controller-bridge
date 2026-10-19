# Roadmap

| Version | Goal | Status |
| --- | --- | --- |
| 0.1 | Portable wired Xbox 360 decoder, mappings, tests | Implemented and host-tested |
| 0.2 | PS5 USB detection and PS5X360 integration | Host USB diagnostic and sample adapter implemented; PS5 backend/integration pending |
| 0.3 | Background service | Conditional on permissions and lifetime tests |
| 0.4 | PS5 settings interface | Planned |
| 0.5 | More USB controllers and Bluetooth research | Planned |
| 1.0 | Stable support for compatible homebrew emulators | Long-term goal |

Next: build a minimal read-only USB diagnostic using a verified backend,
record descriptors and actual report lengths, then integrate at PS5X360's
input driver boundary. Gate this on a reproducible toolchain and the user's
actual firmware/environment. Test DualSense slot 0 plus Xbox slot 1,
disconnect/reconnect, profile login and UI capture before describing multiplayer
as working. No estimated release dates or firmware guarantees.

