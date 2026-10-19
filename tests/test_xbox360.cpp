// SPDX-License-Identifier: MIT
#include "controllers/xbox360.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
namespace {
unsigned checks=0;
void check(bool ok) { ++checks; if(!ok) { std::cerr<<"Failed check "<<checks<<"\n"; std::exit(1); } }
bool near(float a,float b) { return std::fabs(a-b)<0.00001f; }
using Report=std::array<uint8_t,20>;
Report report() { Report r{}; r[1]=20; return r; }
}
int main() {
  using namespace ucb;
  auto r=report();
  auto s=xbox360::decode(r.data(),r.size()).value();
  check(!s.buttons && s.lx==0 && s.ly==0 && s.rx==0 && s.ry==0 && s.lt==0 && s.rt==0);
  const std::array<Button,15> buttons{{Button::Up,Button::Down,Button::Left,Button::Right,
    Button::Menu,Button::View,Button::LeftStick,Button::RightStick,Button::LeftShoulder,
    Button::RightShoulder,Button::Home,Button::South,Button::East,Button::West,Button::North}};
  const std::array<PsButton,15> mapped{{PsButton::Up,PsButton::Down,PsButton::Left,PsButton::Right,
    PsButton::Options,PsButton::Create,PsButton::L3,PsButton::R3,PsButton::L1,PsButton::R1,
    PsButton::PS,PsButton::Cross,PsButton::Circle,PsButton::Square,PsButton::Triangle}};
  const std::array<unsigned,15> wire{{0,1,2,3,4,5,6,7,8,9,10,12,13,14,15}};
  for(size_t i=0;i<buttons.size();++i) {
    r=report(); r[2+wire[i]/8]=uint8_t(1u<<(wire[i]%8));
    s=xbox360::decode(r.data(),r.size()).value();
    check(s.buttons==bit(buttons[i]));
    auto p=map_playstation(s);
    check(p.buttons==static_cast<uint32_t>(mapped[i]) && p.pressed(mapped[i]));
  }
  // Every d-pad combination, including diagonals and opposing directions.
  for(unsigned mask=0;mask<16;++mask) {
    r=report(); r[2]=uint8_t(mask);
    check(xbox360::decode(r.data(),20)->buttons==mask);
  }
  r=report(); r[2]=255; r[3]=255;
  check(xbox360::decode(r.data(),20)->buttons==0x7fff); // reserved wire bit ignored
  r=report(); r[3]=8; check(xbox360::decode(r.data(),20)->buttons==0);
  for(unsigned v=0;v<256;++v) {
    r=report(); r[4]=uint8_t(v); r[5]=uint8_t(255-v);
    s=xbox360::decode(r.data(),20).value();
    check(near(s.lt,v/255.0f) && near(s.rt,(255-v)/255.0f));
    const auto p=map_playstation(s); check(p.l2==s.lt && p.r2==s.rt);
  }
  // Exhaust signed little-endian range independently on all four axes.
  for(size_t off=6;off<=12;off+=2) {
    float previous=-2;
    for(int v=-32768;v<=32767;++v) {
      r=report(); auto u=static_cast<uint16_t>(v);
      r[off]=uint8_t(u&255); r[off+1]=uint8_t(u>>8);
      s=xbox360::decode(r.data(),20,{0,0}).value();
      const std::array<float,4> values{{s.lx,s.ly,s.rx,s.ry}};
      const size_t index=(off-6)/2;
      const float expected=float(v)/(v<0?32768.0f:32767.0f);
      check(near(values[index],expected) && values[index]>=previous && values[index]>=-1 && values[index]<=1);
      previous=values[index];
      for(size_t j=0;j<4;++j) if(j!=index) check(values[j]==0);
      const auto p=map_playstation(s);
      check(p.lx==s.lx && p.ly==s.ly && p.rx==s.rx && p.ry==s.ry);
    }
  }
  check(dead_zone(0.15f,0.15f)==0 && dead_zone(-0.15f,0.15f)==0);
  check(near(dead_zone(0.575f,0.15f),0.5f));
  check(near(dead_zone(-0.575f,0.15f),-0.5f));
  check(dead_zone(1,0.99f)==1 && dead_zone(-1,0.99f)==-1);
  r=report(); r[7]=16; r[11]=16;
  s=xbox360::decode(r.data(),20,{0.2f,0}).value(); check(s.lx==0 && s.rx>0);
  for(float bad : {-1.0f,1.0f,2.0f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()}) {
    bool threw=false; try { (void)xbox360::decode(r.data(),20,{bad,0}); } catch(const std::invalid_argument&) { threw=true; } check(threw);
  }
  r=report(); check(!xbox360::decode(nullptr,20));
  for(size_t n=0;n<20;++n) check(!xbox360::decode(r.data(),n));
  std::array<uint8_t,21> long_report{}; long_report[1]=20;
  check(!xbox360::decode(long_report.data(),21));
  r[0]=1; check(!xbox360::decode(r.data(),20));
  r=report(); r[1]=19; check(!xbox360::decode(r.data(),20));
  ControllerBridge bridge; check(!bridge.state(0) && !bridge.state(1));
  r=report(); r[3]=16; s=xbox360::decode(r.data(),20).value(); bridge.submit(1,s);
  ControllerState native; native.buttons=bit(Button::North); bridge.submit(0,native);
  check(bridge.state(1)->pressed(Button::South) && bridge.state(0)->pressed(Button::North));
  bridge.disconnect(1); check(!bridge.state(1) && bridge.state(0)->pressed(Button::North));
  bridge.submit(1,{}); check(bridge.state(1) && !bridge.state(1)->buttons);
  bool threw=false; try { bridge.submit(4,s); } catch(const std::out_of_range&) { threw=true; } check(threw);
  check(xbox360::matches(0x045e,0x028e) && !xbox360::matches(0x045e,0x0719));
  std::cout<<checks<<" checks passed\n";
}
