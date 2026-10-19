// SPDX-License-Identifier: MIT
#pragma once
#include "controllers/xbox360.hpp"
namespace ucb {
// Descriptor filter for the original wired controller's gamepad interface.
// Endpoint addresses are discovered, not hardcoded. No driver detach/reset.
constexpr bool wired_gamepad_interface(uint16_t vendor, uint16_t product,
    uint8_t cls, uint8_t sub, uint8_t protocol, uint8_t alternate) {
  return xbox360::matches(vendor,product) && cls==0xff && sub==0x5d &&
         protocol==1 && alternate==0;
}
constexpr bool input_endpoint(uint8_t address, uint8_t attributes, uint16_t packet_size) {
  return (address&0x80)!=0 && (address&15)!=0 && (attributes&3)==3 &&
         (packet_size&0x7ff)>=20 && (packet_size&0x7ff)<=1024;
}
}
