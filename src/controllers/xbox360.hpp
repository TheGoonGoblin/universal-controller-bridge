// SPDX-License-Identifier: MIT
#pragma once
#include "bridge/controller_bridge.hpp"

namespace ucb::xbox360 {
constexpr uint16_t microsoft_vendor=0x045e, wired_product=0x028e;
// Identification hint only: transport must also inspect interface/endpoints.
constexpr bool matches(uint16_t vendor, uint16_t product) {
  return vendor==microsoft_vendor && product==wired_product;
}
struct DeadZones { float left=0.15f, right=0.15f; };
inline int32_t signed_le16(const uint8_t* p) {
  const uint32_t v=uint32_t(p[0]) | (uint32_t(p[1])<<8);
  return v>=32768 ? static_cast<int32_t>(v)-65536 : static_cast<int32_t>(v);
}
inline float axis(const uint8_t* p, float dz) {
  const int32_t v=signed_le16(p);
  return dead_zone(static_cast<float>(v)/(v<0 ? 32768.0f : 32767.0f),dz);
}
// Common wired input report: 00 14, 20 bytes. Wireless/other messages rejected.
// Caller supplies actual transfer length, not allocated buffer capacity.
inline std::optional<ControllerState> decode(const uint8_t* data, size_t size, DeadZones dz={}) {
  if (!data || size!=20 || data[0]!=0 || data[1]!=20) return std::nullopt;
  ControllerState s;
  constexpr std::array<Button,8> low{{Button::Up,Button::Down,Button::Left,Button::Right,
    Button::Menu,Button::View,Button::LeftStick,Button::RightStick}};
  for(size_t i=0;i<low.size();++i) if(data[2] & (1u<<i)) s.buttons |= bit(low[i]);
  constexpr std::array<Button,7> high{{Button::LeftShoulder,Button::RightShoulder,Button::Home,
    Button::South,Button::East,Button::West,Button::North}};
  constexpr std::array<unsigned,7> positions{{0,1,2,4,5,6,7}};
  for(size_t i=0;i<high.size();++i) if(data[3] & (1u<<positions[i])) s.buttons |= bit(high[i]);
  s.lt=data[4]/255.0f; s.rt=data[5]/255.0f;
  s.lx=axis(data+6,dz.left); s.ly=axis(data+8,dz.left);
  s.rx=axis(data+10,dz.right); s.ry=axis(data+12,dz.right);
  return s;
}
} // namespace ucb::xbox360
