// SPDX-License-Identifier: MIT
#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
namespace ucb::usb {
struct Layout { int interface_number; uint8_t endpoint; uint16_t packet_size; };
// Original wired Xbox 360: vendor-specific class FF, subclass 5D, protocol 01.
// Parse the whole descriptor before accepting anything. Never cross interfaces
// or alternate settings; reject truncated descriptors and unsafe packet sizes.
inline std::optional<Layout> xbox360_layout(const uint8_t* p,size_t n) {
  if(!p || n<9 || p[0]!=9 || p[1]!=2)return {};
  const size_t total=size_t(p[2])+(size_t(p[3])<<8);
  if(total!=n)return {};
  std::optional<Layout> found;
  int iface=-1;
  for(size_t pos=0;pos<n;) {
    if(n-pos<2)return {};
    const auto len=p[pos];const auto* d=p+pos;
    if(len<2 || len>n-pos)return {};
    if(d[1]==4) {
      if(len<9)return {};
      iface=(d[3]==0 && d[5]==0xff && d[6]==0x5d && d[7]==1)?d[2]:-1;
    } else if(d[1]==5) {
      if(len<7)return {};
      const uint16_t mps=uint16_t(d[4])|(uint16_t(d[5])<<8);
      if(iface>=0 && (d[2]&0x80) && (d[2]&15) && !(d[2]&0x70) &&
         (d[3]&3)==3 && mps>=20 && mps<=256 && !found)
        found=Layout{iface,d[2],mps};
    }
    pos+=len;
  }
  return found;
}
}
