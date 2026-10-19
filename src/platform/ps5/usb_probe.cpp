// SPDX-License-Identifier: MIT
// Experimental bounded PS5 payload. No kernel patches or virtual pad creation.
// USB_FS access approach researched in FGG-XSense (GPL-3.0); implementation here
// is original and uses the SDK's FreeBSD structures, not handwritten ABI layouts.
#include "transport/xbox360_layout.hpp"
#include "controllers/xbox360.hpp"
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <dev/usb/usb.h>
#include <dev/usb/usb_ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <ctime>
namespace {
FILE* log_file=nullptr;
void note(const char* stage,int error) {
  std::fprintf(log_file,"%s: errno=%d (%s)\n",stage,error,std::strerror(error));
  std::fflush(log_file);
}
struct Device {
  int fd=-1,interface_number=-1;bool claimed=false,initialized=false,pending=false;
  usb_fs_endpoint endpoint{};uint8_t buffer[256]{};void* pointer=buffer;uint32_t length=sizeof buffer;
  ~Device() {
    if(fd<0)return;
    if(initialized){usb_fs_uninit u{};if(ioctl(fd,USB_FS_UNINIT,&u))note("UNINIT",errno);}
    if(claimed && ioctl(fd,USB_RELEASE_INTERFACE,&interface_number))note("RELEASE",errno);
    close(fd);
  }
  bool start(uint16_t size) {
    length=size;endpoint.nFrames=1;endpoint.aFrames=0;endpoint.status=0;
    usb_fs_start request{};
    if(ioctl(fd,USB_FS_START,&request)){note("START",errno);return false;}
    pending=true;return true;
  }
};
unsigned word(const void* p) {const auto* b=static_cast<const uint8_t*>(p);return unsigned(b[0])|(unsigned(b[1])<<8);}
int probe() {
  // Only original Microsoft wired 045e:028e is ever configured or claimed.
  // Bounded bus/unit scan; log access errors instead of claiming no hardware.
  for(int bus=0;bus<16;++bus)for(int unit=1;unit<128;++unit) {
    char path[64];std::snprintf(path,sizeof path,"/dev/ugen%d.%d",bus,unit);
    Device d;d.fd=open(path,O_RDWR|O_NONBLOCK);
    if(d.fd<0){if(errno!=ENOENT){std::fprintf(log_file,"%s ",path);note("OPEN",errno);}continue;}
    usb_device_descriptor desc{};
    if(ioctl(d.fd,USB_GET_DEVICE_DESC,&desc)){note("DEVICE_DESC",errno);continue;}
    const auto vid=word(&desc.idVendor),pid=word(&desc.idProduct);
    std::fprintf(log_file,"DEVICE %s %04x:%04x\n",path,vid,pid);std::fflush(log_file);
    if(!ucb::xbox360::matches(vid,pid))continue;
    uint8_t config[4096]{};usb_gen_descriptor gd{};
    gd.ugd_data=config;gd.ugd_maxlen=sizeof config;gd.ugd_config_index=0;
    if(ioctl(d.fd,USB_GET_FULL_DESC,&gd)){note("FULL_DESC",errno);return 2;}
    if(gd.ugd_actlen>sizeof config){note("DESCRIPTOR_TOO_LONG",EINVAL);return 2;}
    auto layout=ucb::usb::xbox360_layout(config,gd.ugd_actlen);
    if(!layout){note("NO_SAFE_XINPUT_INTERFACE",EINVAL);return 2;}
    d.interface_number=layout->interface_number;
    std::fprintf(log_file,"MATCH interface=%d endpoint=%02x packet=%u\n",d.interface_number,layout->endpoint,layout->packet_size);
    int current=-1;
    if(ioctl(d.fd,USB_GET_CONFIG,&current)){note("GET_CONFIG",errno);return 2;}
    // FreeBSD configuration index zero. Do not reset an already active device.
    if(current!=0){int first=0;if(ioctl(d.fd,USB_SET_CONFIG,&first)){note("SET_CONFIG",errno);return 2;}}
    if(ioctl(d.fd,USB_CLAIM_INTERFACE,&d.interface_number)){note("CLAIM",errno);return 2;}
    d.claimed=true;
    d.endpoint.ppBuffer=&d.pointer;d.endpoint.pLength=&d.length;
    d.endpoint.nFrames=1;d.endpoint.flags=USB_FS_FLAG_SINGLE_SHORT_OK;d.endpoint.timeout=1000;
    usb_fs_init init{};init.pEndpoints=&d.endpoint;init.ep_index_max=1;
    if(ioctl(d.fd,USB_FS_INIT,&init)){note("FS_INIT",errno);return 2;}d.initialized=true;
    usb_fs_open ep{};ep.ep_no=layout->endpoint;ep.max_bufsize=sizeof d.buffer;ep.max_frames=1;
    if(ioctl(d.fd,USB_FS_OPEN,&ep)){note("FS_OPEN",errno);return 2;}
    timespec begun{};if(clock_gettime(CLOCK_MONOTONIC,&begun)){note("CLOCK",errno);return 2;}
    unsigned valid=0,invalid=0;uint8_t last[20]{};bool have_last=false;
    for(;;) {
      timespec now{};if(clock_gettime(CLOCK_MONOTONIC,&now)){note("CLOCK",errno);return 2;}
      if(now.tv_sec-begun.tv_sec>=60)break;
      if(!d.pending && !d.start(layout->packet_size))return 2;
      usb_fs_complete completion{};
      if(ioctl(d.fd,USB_FS_COMPLETE,&completion)) {
        if(errno!=EBUSY && errno!=EAGAIN){note("COMPLETE",errno);return 2;}
        usleep(2000);continue;
      }
      if(completion.ep_index!=0){note("BAD_COMPLETION_INDEX",EINVAL);return 2;}
      d.pending=false;
      if(d.endpoint.status){std::fprintf(log_file,"TRANSFER_STATUS %u\n",unsigned(d.endpoint.status));std::fflush(log_file);usleep(10000);continue;}
      if(d.endpoint.aFrames!=1 || d.length>sizeof d.buffer){note("BAD_TRANSFER_LENGTH",EINVAL);return 2;}
      auto state=ucb::xbox360::decode(d.buffer,d.length);
      if(!state){++invalid;continue;}
      ++valid;
      if(!have_last || std::memcmp(last,d.buffer,20)) {
        std::memcpy(last,d.buffer,20);have_last=true;
        std::fprintf(log_file,"INPUT buttons=%08x LT=%.3f RT=%.3f LX=%.3f LY=%.3f RX=%.3f RY=%.3f\n",state->buttons,double(state->lt),double(state->rt),double(state->lx),double(state->ly),double(state->rx),double(state->ry));std::fflush(log_file);
      }
      if(state->pressed(ucb::Button::Menu) && state->pressed(ucb::Button::View))break;
    }
    std::fprintf(log_file,"SUMMARY valid=%u other=%u; no emulator output enabled\n",valid,invalid);
    return valid?0:3;
  }
  std::fprintf(log_file,"NO_MATCH: no accessible Microsoft wired Xbox 360 device (045e:028e)\n");
  return 4;
}
}
int main() {
  mkdir("/data/ucb",0700);
  int lock=open("/data/ucb/usb-probe.lock",O_CREAT|O_RDWR,0600);
  if(lock<0)return 5;
  if(flock(lock,LOCK_EX|LOCK_NB)){close(lock);return 5;}
  log_file=std::fopen("/data/ucb/usb-probe.log","w");
  if(!log_file){close(lock);return 5;}
  std::fprintf(log_file,"UCB Xbox 360 USB probe v0.1 - experimental; 60 second maximum read test\n");std::fflush(log_file);
  const int rc=probe();std::fprintf(log_file,"END result=%d\n",rc);std::fclose(log_file);close(lock);return rc;
}
