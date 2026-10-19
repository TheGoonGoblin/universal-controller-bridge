// SPDX-License-Identifier: MIT
#include "transport/xbox360_layout.hpp"
#include <vector>
#include <cstdio>
#include <cstdlib>
static unsigned checks=0;
static void check(bool b){++checks;if(!b){std::fprintf(stderr,"layout check %u failed\n",checks);std::exit(1);}}
int main(){
  std::vector<uint8_t> d{9,2,25,0,1,1,0,0x80,50,9,4,0,0,1,0xff,0x5d,1,0,7,5,0x81,3,32,0,4};
  auto good=ucb::usb::xbox360_layout(d.data(),d.size());check(good && good->interface_number==0 && good->endpoint==0x81 && good->packet_size==32);
  check(!ucb::usb::xbox360_layout(nullptr,25));
  for(size_t n=0;n<d.size();++n)check(!ucb::usb::xbox360_layout(d.data(),n));
  for(size_t field:{size_t(12),size_t(14),size_t(15),size_t(16),size_t(21)}){
    auto bad=d;bad[field]=field==12?1:0;check(!ucb::usb::xbox360_layout(bad.data(),bad.size()));
  }
  for(unsigned length=0;length<256;++length){auto bad=d;bad[9]=uint8_t(length);if(length!=9)check(!ucb::usb::xbox360_layout(bad.data(),bad.size()));}
  for(unsigned packet=0;packet<65536;++packet){auto b=d;b[22]=uint8_t(packet);b[23]=uint8_t(packet>>8);check(bool(ucb::usb::xbox360_layout(b.data(),b.size()))==(packet>=20&&packet<=256));}
  // An endpoint in a following non-XInput interface must not be borrowed.
  auto other=d;other.insert(other.begin()+18,{9,4,1,0,1,3,0,0,0});other[2]=uint8_t(other.size());check(!ucb::usb::xbox360_layout(other.data(),other.size()));
  auto tail=d;tail.push_back(0);tail[2]=uint8_t(tail.size());check(!ucb::usb::xbox360_layout(tail.data(),tail.size()));
  std::printf("%u USB layout checks passed\n",checks);
}
