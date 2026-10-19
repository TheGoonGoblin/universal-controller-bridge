# Native title tooling provenance

Vendored from mihawk-99/PS5_Vulkan commit
f410e67a0241b3e8edcdbcd2a9b285dfc4a412ed, tooling/native.
https://github.com/mihawk-99/PS5_Vulkan/tree/f410e67a0241b3e8edcdbcd2a9b285dfc4a412ed/tooling/native

Original author: BlackBearReloaded, ps5-native-app-boilerplate.
GPL-3.0-or-later; headers and license retained. UCB modifies ps5-pie.ld to
define LLVM unwind boundary symbols; the other vendored source is unchanged.
No proprietary runtime module is included. libc.prx is reproduced from
libc_builder.cpp plus runtime/api-surface.txt and imports.txt.
