// SPDX-License-Identifier: MIT
#include "controllers/xbox360.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
int main(int argc, char** argv) {
  if(argc!=2) { std::cerr<<"Usage: ucb_decode report.bin (one 20-byte wired report; no USB access)\n"; return 2; }
  std::ifstream f(argv[1],std::ios::binary);
  if(!f) { std::cerr<<"Cannot open report\n"; return 2; }
  std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(f)),{});
  const auto s=ucb::xbox360::decode(bytes.data(),bytes.size());
  if(!s) { std::cerr<<"Invalid wired Xbox 360 report\n"; return 1; }
  auto p=ucb::map_playstation(*s);
  std::cout<<"Semantic PS buttons="<<p.buttons<<" sticks="<<p.lx<<","<<p.ly<<","<<p.rx<<","<<p.ry
           <<" triggers="<<p.l2<<","<<p.r2<<"\n";
}
