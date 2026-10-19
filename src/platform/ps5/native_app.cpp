// SPDX-License-Identifier: MIT
// Experimental PS5 ELF application. API layouts follow the cited SDL PS5 port.
#include "app/canvas.hpp"
#include "tile_offset.hpp"
#include <sys/event.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <array>
#include <cstdio>
#include <cstring>
extern "C" {
int sceSystemServiceHideSplashScreen();
int sceKernelAllocateMainDirectMemory(size_t,size_t,int,intptr_t*);
int sceKernelMapDirectMemory(void**,size_t,int,int,intptr_t,size_t);
int sceKernelReleaseDirectMemory(intptr_t,size_t);
int sceKernelCreateEqueue(struct kevent**,const char*);
int sceKernelWaitEqueue(struct kevent*,struct kevent*,int,int*,unsigned*);
int sceKernelDeleteEqueue(struct kevent*);
int sceVideoOutOpen(int,int,int,const void*);
void sceVideoOutClose(int);
int sceVideoOutAddFlipEvent(struct kevent*,int,void*);
int sceVideoOutDeleteFlipEvent(struct kevent*,int);
int sceVideoOutSetFlipRate(int,int);
int sceVideoOutSubmitFlip(int,int,uint32_t,int64_t);
struct VideoBuffer { void* data;uint64_t reserved[3]; };
struct VideoAttribute { uint8_t bytes[80]; };
void sceVideoOutSetBufferAttribute2(VideoAttribute*,uint64_t,uint32_t,uint32_t,uint32_t,uint64_t,uint32_t,uint64_t);
int sceVideoOutRegisterBuffers2(int,int,int,VideoBuffer*,int,VideoAttribute*,int,void*);
int sceUserServiceInitialize(void*);
int sceUserServiceGetLoginUserIdList(int32_t*);
int scePadInit();int scePadOpen(int32_t,int32_t,int32_t,const void*);
int scePadReadState(int32_t,void*);int scePadClose(int32_t);
}
namespace {
struct Display {
  static constexpr size_t bytes=0x4000000;
  int handle=-1;intptr_t physical=0;void* memory=nullptr;struct kevent* queue=nullptr;
  bool allocated=false,flip_event=false;
  VideoBuffer buffers[2]{};
  ~Display() {
    if(flip_event)sceVideoOutDeleteFlipEvent(queue,handle);
    if(queue)sceKernelDeleteEqueue(queue);
    if(handle>=0)sceVideoOutClose(handle);
    if(memory)munmap(memory,bytes);
    if(allocated)sceKernelReleaseDirectMemory(physical,bytes);
  }
  bool init() {
    handle=sceVideoOutOpen(0xff,0,0,nullptr);if(handle<0)return false;
    if(sceKernelAllocateMainDirectMemory(bytes,0x20000,3,&physical))return false;
    allocated=true;
    if(sceKernelMapDirectMemory(&memory,bytes,0x33,0,physical,0x20000)) {memory=nullptr;return false;}
    std::memset(memory,0,bytes);
    buffers[0].data=memory;buffers[1].data=static_cast<uint8_t*>(memory)+bytes/2;
    if(sceKernelCreateEqueue(&queue,"UCB flip"))return false;
    if(sceVideoOutAddFlipEvent(queue,handle,nullptr))return false;
    flip_event=true;if(sceVideoOutSetFlipRate(handle,0))return false;
    VideoAttribute attr{};
    sceVideoOutSetBufferAttribute2(&attr,0x8000000022000000ULL,0,1920,1080,0,0,0);
    return sceVideoOutRegisterBuffers2(handle,0,0,buffers,2,&attr,0,nullptr)==0;
  }
  bool present(const ucb::app::Canvas& canvas,uint32_t frame) {
    auto* dest=static_cast<uint32_t*>(buffers[frame%2].data);
    for(uint32_t y=0;y<1080;++y)for(uint32_t x=0;x<1920;++x)
      dest[ucb::ps5::tile_pixel(x,y,1920)]=canvas.pixels[size_t(y)*1920+x];
    if(sceVideoOutSubmitFlip(handle,int(frame%2),1,frame))return false;
    struct kevent event{};int count=0;unsigned timeout=1000000;
    return sceKernelWaitEqueue(queue,&event,1,&count,&timeout)==0;
  }
};
}
int main() {
  std::puts("UCB native settings preview 0.3 - USB/emulator backend unavailable");
  sceSystemServiceHideSplashScreen();
  Display display;if(!display.init()){std::puts("UCB: display initialization failed");return 1;}
  ucb::app::Model model;ucb::app::Canvas canvas;
  mkdir("/data/ucb",0700);
  const char* config="/data/ucb/settings.conf";
  if(ucb::app::load(config,model.settings))model.message="SAVED SETTINGS LOADED";
  sceUserServiceInitialize(nullptr);scePadInit();int pad=-1;
  uint32_t previous=0,frame=0;
  while(true) {
    if(pad<0 && frame%60==0){std::array<int32_t,4> users{{-1,-1,-1,-1}};
      if(sceUserServiceGetLoginUserIdList(users.data())>=0)for(int32_t id:users)if(id>=0){pad=scePadOpen(id,0,0,nullptr);if(pad>=0)break;}}
    alignas(16) std::array<uint8_t,256> data{};
    const bool connected=pad>=0 && scePadReadState(pad,data.data())>=0 && data[76]!=0;
    if(!connected && pad>=0){scePadClose(pad);pad=-1;}
    model.native_connected=connected;model.buttons=0;
    model.lx=model.ly=model.rx=model.ry=128;model.l2=model.r2=0;
    if(connected){model.buttons=uint32_t(data[0])|(uint32_t(data[1])<<8)|(uint32_t(data[2])<<16)|(uint32_t(data[3])<<24);
      model.lx=data[4];model.ly=data[5];model.rx=data[6];model.ry=data[7];model.l2=data[8];model.r2=data[9];}
    const uint32_t pressed=model.buttons&~previous;previous=model.buttons;
    if(pressed&0x2000)break;
    if(pressed&0x400)model.tab=(model.tab+4)%5;
    if(pressed&0x800)model.tab=(model.tab+1)%5;
    if(model.tab==1){if(pressed&0x10)model.row=(model.row+2)%3;if(pressed&0x40)model.row=(model.row+1)%3;
      if(pressed&0x80)model.change(-1);if(pressed&0x20)model.change(1);}
    if(pressed&0x4000)model.message=ucb::app::save(config,model.settings)?"SETTINGS SAVED":"SAVE FAILED - DATA DIRECTORY UNAVAILABLE";
    canvas.draw(model);if(!display.present(canvas,frame++)){std::puts("UCB: display flip failed");break;}
    usleep(16000);
  }
  if(pad>=0)scePadClose(pad);
  return 0;
}
