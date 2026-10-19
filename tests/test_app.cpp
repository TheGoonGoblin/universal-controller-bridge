// SPDX-License-Identifier: MIT
#include "app/canvas.hpp"
#include "platform/ps5/tile_offset.hpp"
#include <cstdlib>
#include <iostream>
#include <unistd.h>
unsigned checks=0;
void check(bool ok) { ++checks;if(!ok){std::cerr<<"App check failed "<<checks<<"\n";std::exit(1);} }
int main() {
  using namespace ucb::app;
  Model m;check(valid(m.settings));
  for(int i=0;i<200;++i){m.change(1);}check(m.settings.left==90);
  for(int i=0;i<200;++i){m.change(-1);}check(m.settings.left==0);
  m.row=2;for(int i=0;i<10;++i)m.change(1);check(m.settings.player==4);
  for(int i=0;i<10;++i){m.change(-1);}check(m.settings.player==2);
  char path[]="/tmp/ucb-settings-test-XXXXXX";int fd=mkstemp(path);check(fd>=0);close(fd);
  m.settings={23,41,3};check(save(path,m.settings));Settings loaded;check(load(path,loaded));
  check(loaded.left==23 && loaded.right==41 && loaded.player==3);
  FILE* f=std::fopen(path,"w");check(f!=nullptr);std::fputs("UCB1 99 1 2",f);std::fclose(f);
  check(!load(path,loaded) && loaded.left==23);
  f=std::fopen(path,"w");std::fputs("UCB1 1 1 2 trailing",f);std::fclose(f);check(!load(path,loaded));
  std::remove(path);
  for(int tab=0;tab<5;++tab){m.tab=tab;Canvas c;c.draw(m);check(c.pixels.size()==1920u*1080u);check(c.pixels[0]!=c.pixels.back());}
  // Every visible pixel must map to a unique, in-bounds tiled video address.
  std::vector<bool> seen(0x2000000/4,false);
  for(uint32_t y=0;y<1080;++y)for(uint32_t x=0;x<1920;++x){auto p=ucb::ps5::tile_pixel(x,y,1920);check(p<seen.size());check(!seen[p]);seen[p]=true;}
  std::cout<<checks<<" app checks passed\n";
}
