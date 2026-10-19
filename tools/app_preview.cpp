// SPDX-License-Identifier: MIT
#include "app/canvas.hpp"
#include <cstdlib>
int main(int argc,char** argv) {
  if(argc!=3)return 2;
  ucb::app::Model m;m.tab=std::atoi(argv[1]);if(m.tab<0||m.tab>3)return 2;
  ucb::app::Canvas c;c.draw(m);
  FILE* f=std::fopen(argv[2],"wb");if(!f)return 1;
  std::fprintf(f,"P6\n%d %d\n255\n",c.width,c.height);
  for(auto pixel:c.pixels){const unsigned char rgb[]{uint8_t(pixel),uint8_t(pixel>>8),uint8_t(pixel>>16)};std::fwrite(rgb,1,3,f);}
  return std::fclose(f)==0?0:1;
}
