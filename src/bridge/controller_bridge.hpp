// SPDX-License-Identifier: MIT
#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>

namespace ucb {
// Face names describe physical position, independent of manufacturer.
enum class Button : uint32_t {
  Up=1u<<0, Down=1u<<1, Left=1u<<2, Right=1u<<3,
  Menu=1u<<4, View=1u<<5, LeftStick=1u<<6, RightStick=1u<<7,
  LeftShoulder=1u<<8, RightShoulder=1u<<9, Home=1u<<10,
  South=1u<<11, East=1u<<12, West=1u<<13, North=1u<<14
};
constexpr uint32_t bit(Button b) { return static_cast<uint32_t>(b); }
struct ControllerState {
  uint32_t buttons=0;
  // Signed stick coordinates: right/up positive, exact endpoints -1 and +1.
  float lx=0, ly=0, rx=0, ry=0;
  float lt=0, rt=0; // [0,1]
  bool pressed(Button b) const { return (buttons & bit(b)) != 0; }
};
inline float dead_zone(float v, float threshold) {
  if (!std::isfinite(threshold) || threshold < 0 || threshold >= 1)
    throw std::invalid_argument("dead zone must be finite and in [0,1)");
  const float magnitude=std::fabs(v);
  return magnitude <= threshold ? 0 : std::copysign((magnitude-threshold)/(1-threshold),v);
}
// Semantic PlayStation controls only; NOT a ScePad ABI or virtual device report.
enum class PsButton : uint32_t {
  Up=1u<<0, Down=1u<<1, Left=1u<<2, Right=1u<<3,
  Options=1u<<4, Create=1u<<5, L3=1u<<6, R3=1u<<7,
  L1=1u<<8, R1=1u<<9, PS=1u<<10,
  Cross=1u<<11, Circle=1u<<12, Square=1u<<13, Triangle=1u<<14
};
struct PlayStationState {
  uint32_t buttons=0;
  float lx=0, ly=0, rx=0, ry=0, l2=0, r2=0;
  bool pressed(PsButton b) const { return (buttons & static_cast<uint32_t>(b)) != 0; }
};
inline PlayStationState map_playstation(const ControllerState& s) {
  PlayStationState p;
  constexpr std::array<Button,15> from{{Button::Up,Button::Down,Button::Left,Button::Right,
    Button::Menu,Button::View,Button::LeftStick,Button::RightStick,Button::LeftShoulder,
    Button::RightShoulder,Button::Home,Button::South,Button::East,Button::West,Button::North}};
  constexpr std::array<PsButton,15> to{{PsButton::Up,PsButton::Down,PsButton::Left,PsButton::Right,
    PsButton::Options,PsButton::Create,PsButton::L3,PsButton::R3,PsButton::L1,
    PsButton::R1,PsButton::PS,PsButton::Cross,PsButton::Circle,PsButton::Square,PsButton::Triangle}};
  for (size_t i=0;i<from.size();++i) if(s.pressed(from[i])) p.buttons |= static_cast<uint32_t>(to[i]);
  p.lx=s.lx; p.ly=s.ly; p.rx=s.rx; p.ry=s.ry; p.l2=s.lt; p.r2=s.rt;
  return p;
}
// Single-threaded ownership. Empty optional means disconnected, not centered.
// Slots are zero-based: 0 reserved for native DualSense, 1 initial Xbox target.
class ControllerBridge {
 public:
  static constexpr size_t capacity=4;
  void submit(size_t slot, const ControllerState& state) { slots_.at(slot)=state; }
  void disconnect(size_t slot) { slots_.at(slot).reset(); }
  const std::optional<ControllerState>& state(size_t slot) const { return slots_.at(slot); }
 private:
  std::array<std::optional<ControllerState>,capacity> slots_{};
};
} // namespace ucb
