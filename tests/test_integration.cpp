// SPDX-License-Identifier: MIT
#include "transport/usb_profile.hpp"
#include "bridge/report_session.hpp"
#include "integration/ps5x360_sample.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
namespace {
unsigned checks=0;
void check(bool ok) { ++checks; if(!ok) { std::cerr<<"Integration check failed "<<checks<<"\n"; std::exit(1); } }
// Same public fields as the inspected upstream PadSample. This tests schema and
// golden vectors, not compilation of the entire upstream emulator.
struct Sample { uint32_t buttons=0; uint8_t lx=128,ly=128,rx=128,ry=128,l2=0,r2=0; bool connected=false; };
}
int main() {
  using namespace ucb;
  check(wired_gamepad_interface(0x045e,0x028e,0xff,0x5d,1,0));
  check(!wired_gamepad_interface(0x045e,0x028e,0xff,0x5d,2,0));
  check(!wired_gamepad_interface(0x045e,0x028e,0xff,0x5d,1,1));
  check(!wired_gamepad_interface(0x045e,0x0719,0xff,0x5d,1,0));
  check(!wired_gamepad_interface(0x045e,0x028e,3,0x5d,1,0));
  check(input_endpoint(0x81,3,32) && input_endpoint(0x83,3,20));
  check(!input_endpoint(0x01,3,32) && !input_endpoint(0x80,3,32));
  check(!input_endpoint(0x81,2,32) && !input_endpoint(0x81,3,19));
  auto p=ps5x360::make_sample<Sample>(std::nullopt);
  check(!p.connected && p.buttons==0 && p.lx==128 && p.ly==128 && p.l2==0);
  const Button from[]{Button::Up,Button::Down,Button::Left,Button::Right,Button::Menu,
    Button::View,Button::LeftStick,Button::RightStick,Button::LeftShoulder,
    Button::RightShoulder,Button::South,Button::East,Button::West,Button::North,Button::Home};
  const uint32_t golden[]{0x10,0x40,0x80,0x20,0x8,0x100000,0x2,0x4,0x400,0x800,0x4000,0x2000,0x8000,0x1000,0};
  for(size_t i=0;i<15;++i) {
    ControllerState s; s.buttons=bit(from[i]); p=ps5x360::make_sample<Sample>(s);
    check(p.connected && p.buttons==golden[i]);
  }
  ControllerState s; s.lx=-1;s.ly=1;s.rx=1;s.ry=-1;s.lt=93/255.0f;s.rt=247/255.0f;
  p=ps5x360::make_sample<Sample>(s);
  check(p.lx==0 && p.ly==0 && p.rx==255 && p.ry==255 && p.l2==93 && p.r2==247);
  // Exact native 8-bit round-trip, matching upstream's piecewise center.
  for(int v=0;v<256;++v) {
    const float axis=(v-128)/float(v<128?128:127);
    check(ps5x360::axis_byte(axis)==v && ps5x360::axis_byte(-axis,true)==v);
    check(ps5x360::trigger_byte(v/255.0f)==v);
  }
  check(ps5x360::axis_byte(std::numeric_limits<float>::quiet_NaN())==128);
  check(ps5x360::axis_byte(2)==255 && ps5x360::axis_byte(-2)==0);
  check(ps5x360::trigger_byte(2)==255 && ps5x360::trigger_byte(-2)==0);
  ControllerBridge bridge; ControllerState native; native.buttons=bit(Button::North); bridge.submit(0,native);
  ReportSession session(bridge,1,1000), second(bridge,2,500);
  std::array<uint8_t,20> r{};r[1]=20;r[3]=16;
  check(session.receive(r.data(),20,100));
  r[3]=32; check(second.receive(r.data(),20,100));
  check(bridge.state(0)->buttons==bit(Button::North));
  check(bridge.state(1)->pressed(Button::South) && bridge.state(2)->pressed(Button::East));
  check(!session.receive(r.data(),19,1000));
  session.tick(1099); check(bool(bridge.state(1))); session.tick(1100); check(!bridge.state(1));
  second.tick(600);check(!bridge.state(2) && bridge.state(0)->pressed(Button::North));
  check(session.receive(r.data(),20,1200));
  check(!session.receive(r.data(),20,1199));
  session.tick(1199);check(!bridge.state(1)); // clock regression clears state
  check(session.receive(r.data(),20,1300));session.disconnect();check(!bridge.state(1));
  for(size_t bad : {size_t(0),size_t(4)}) {
    bool threw=false;try { ReportSession invalid(bridge,bad); } catch(const std::invalid_argument&) { threw=true; }check(threw);
  }
  bool threw=false;try { ReportSession invalid(bridge,1,0); } catch(const std::invalid_argument&) { threw=true; }check(threw);
  std::cout<<checks<<" integration checks passed\n";
}
