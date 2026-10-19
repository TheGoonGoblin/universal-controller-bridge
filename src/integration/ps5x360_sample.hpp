// SPDX-License-Identifier: MIT
#pragma once
#include "bridge/controller_bridge.hpp"
#include <algorithm>
namespace ucb::ps5x360 {
inline uint8_t axis_byte(float value, bool invert=false) {
  if(!std::isfinite(value)) value=0;
  value=std::clamp(invert ? -value : value,-1.0f,1.0f);
  return static_cast<uint8_t>(std::lround(128.0f+value*(value<0 ? 128.0f : 127.0f)));
}
inline uint8_t trigger_byte(float value) {
  if(!std::isfinite(value)) value=0;
  return static_cast<uint8_t>(std::lround(std::clamp(value,0.0f,1.0f)*255.0f));
}
// Sample must be xbox360ps5::PadSample or a structurally identical host type.
// Native mask values verified against upstream dualsense_input.cpp, not Sony ABI.
// Back deliberately uses touchpad, which PS5X360 translates into guest Back.
// Guide is omitted: current upstream driver has no guest Guide mapping.
template<class Sample>
Sample make_sample(const std::optional<ControllerState>& state) {
  Sample p{};
  p.lx=p.ly=p.rx=p.ry=128; p.buttons=0; p.l2=p.r2=0; p.connected=false;
  if(!state) return p;
  const auto& s=*state; p.connected=true;
  constexpr Button from[]{Button::Up,Button::Down,Button::Left,Button::Right,
    Button::Menu,Button::View,Button::LeftStick,Button::RightStick,Button::LeftShoulder,
    Button::RightShoulder,Button::South,Button::East,Button::West,Button::North};
  constexpr uint32_t to[]{0x10,0x40,0x80,0x20,0x08,0x100000,0x02,0x04,
    0x400,0x800,0x4000,0x2000,0x8000,0x1000};
  for(size_t i=0;i<14;++i) if(s.pressed(from[i])) p.buttons|=to[i];
  p.lx=axis_byte(s.lx); p.ly=axis_byte(s.ly,true);
  p.rx=axis_byte(s.rx); p.ry=axis_byte(s.ry,true);
  p.l2=trigger_byte(s.lt); p.r2=trigger_byte(s.rt);
  return p;
}
}
