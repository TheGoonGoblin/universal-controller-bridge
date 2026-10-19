// SPDX-License-Identifier: MIT
#include <xmmintrin.h>
extern "C" void ps5_fp_ieee() {
  // Disable flush-to-zero (bit 15) and denormals-are-zero (bit 6).
  _mm_setcsr(_mm_getcsr() & ~unsigned(0x8040));
}
