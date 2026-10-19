// SPDX-License-Identifier: MIT
#include "transport/usb_profile.hpp"
#include "bridge/report_session.hpp"
#include "integration/ps5x360_sample.hpp"
#include <libusb.h>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {
struct Context {
  libusb_context* value=nullptr;
  ~Context() { if(value) libusb_exit(value); }
};
struct List {
  libusb_device** value=nullptr;
  ~List() { if(value) libusb_free_device_list(value,1); }
};
struct Handle {
  libusb_device_handle* value=nullptr; int interface=-1;
  ~Handle() { if(value) { if(interface>=0) libusb_release_interface(value,interface); libusb_close(value); } }
};
struct PadSample {
  uint32_t buttons=0;
  uint8_t lx=128,ly=128,rx=128,ry=128,l2=0,r2=0;
  bool connected=false;
};
uint64_t now() {
  return uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::steady_clock::now().time_since_epoch()).count());
}
int number(const char* arg,int low,int high) {
  size_t n=0; int v=std::stoi(arg,&n);
  if(n!=std::string(arg).size() || v<low || v>high) throw std::invalid_argument("range");
  return v;
}
}
int main(int argc,char** argv) {
  bool read=false; int bus=0,address=0,seconds=0;
  try {
    if(argc==1 || (argc==2 && std::string(argv[1])=="--list")) {}
    else if(argc==5 && std::string(argv[1])=="--read") {
      read=true; bus=number(argv[2],0,255); address=number(argv[3],1,127); seconds=number(argv[4],1,60);
    } else throw std::invalid_argument("usage");
  } catch(const std::exception&) {
    std::cerr<<"Usage: ucb_usb [--list | --read BUS ADDRESS SECONDS(1..60)]\n"; return 2;
  }
  Context context; int result=libusb_init(&context.value);
  if(result<0) { std::cerr<<"libusb_init: "<<libusb_error_name(result)<<"\n"; return 1; }
  List list; const auto count=libusb_get_device_list(context.value,&list.value);
  if(count<0) { std::cerr<<"enumeration: "<<libusb_error_name(int(count))<<"\n"; return 1; }
  for(ssize_t n=0;n<count;++n) {
    auto* dev=list.value[n]; libusb_device_descriptor desc{};
    if(libusb_get_device_descriptor(dev,&desc)<0) continue;
    const auto b=libusb_get_bus_number(dev), a=libusb_get_device_address(dev);
    if(read && (b!=bus || a!=address)) continue;
    if(!ucb::xbox360::matches(desc.idVendor,desc.idProduct)) continue;
    std::cout<<"candidate bus="<<int(b)<<" address="<<int(a)<<" vid=045e pid=028e\n";
    libusb_config_descriptor* raw=nullptr;
    result=libusb_get_active_config_descriptor(dev,&raw);
    if(result<0) { std::cerr<<"active config: "<<libusb_error_name(result)<<"\n"; if(read) return 1; continue; }
    std::unique_ptr<libusb_config_descriptor,decltype(&libusb_free_config_descriptor)> config(raw,libusb_free_config_descriptor);
    int interface=-1; uint8_t endpoint=0; int packet_size=0;
    for(int i=0;i<config->bNumInterfaces;++i) {
      const auto& iface=config->interface[i];
      for(int k=0;k<iface.num_altsetting;++k) {
        const auto& alt=iface.altsetting[k];
        std::cout<<" interface="<<int(alt.bInterfaceNumber)<<" alt="<<int(alt.bAlternateSetting)
          <<" class="<<int(alt.bInterfaceClass)<<" subclass="<<int(alt.bInterfaceSubClass)
          <<" protocol="<<int(alt.bInterfaceProtocol)<<"\n";
        for(int e=0;e<alt.bNumEndpoints;++e) {
          const auto& ep=alt.endpoint[e];
          std::cout<<"  endpoint="<<int(ep.bEndpointAddress)<<" attributes="<<int(ep.bmAttributes)
            <<" max_packet="<<ep.wMaxPacketSize<<"\n";
          if(interface<0 && ucb::wired_gamepad_interface(desc.idVendor,desc.idProduct,
            alt.bInterfaceClass,alt.bInterfaceSubClass,alt.bInterfaceProtocol,alt.bAlternateSetting) &&
            ucb::input_endpoint(ep.bEndpointAddress,ep.bmAttributes,ep.wMaxPacketSize)) {
            interface=alt.bInterfaceNumber; endpoint=ep.bEndpointAddress; packet_size=ep.wMaxPacketSize&0x7ff;
          }
        }
      }
    }
    if(!read) continue;
    if(interface<0) { std::cerr<<"No supported active gamepad interface\n"; return 1; }
    Handle handle;
    result=libusb_open(dev,&handle.value);
    if(result<0) { std::cerr<<"open: "<<libusb_error_name(result)<<"\n"; return 1; }
    result=libusb_claim_interface(handle.value,interface);
    if(result<0) { std::cerr<<"claim (no driver detach attempted): "<<libusb_error_name(result)<<"\n"; return 1; }
    handle.interface=interface;
    ucb::ControllerBridge bridge; ucb::ReportSession session(bridge);
    const uint64_t finish=now()+uint64_t(seconds)*1000;
    std::vector<uint8_t> bytes(static_cast<size_t>(packet_size)); unsigned valid=0;
    while(now()<finish) {
      int actual=0;
      result=libusb_interrupt_transfer(handle.value,endpoint,bytes.data(),packet_size,&actual,100);
      if(result==LIBUSB_ERROR_TIMEOUT) { session.tick(now()); continue; }
      if(result<0) { session.disconnect(); std::cerr<<"read: "<<libusb_error_name(result)<<"\n"; return 1; }
      if(actual<0 || actual>packet_size) { session.disconnect(); std::cerr<<"Invalid transfer length\n"; return 1; }
      std::cout<<"report length="<<actual<<" hex=";
      for(int i=0;i<actual;++i) std::cout<<std::hex<<std::setfill('0')<<std::setw(2)<<unsigned(bytes[size_t(i)]);
      std::cout<<std::dec<<"\n";
      if(session.receive(bytes.data(),static_cast<size_t>(actual),now())) {
        ++valid; const auto p=ucb::ps5x360::make_sample<PadSample>(bridge.state(1));
        std::cout<<"decoded player=2 mask="<<p.buttons<<" lx="<<int(p.lx)<<" ly="<<int(p.ly)
          <<" l2="<<int(p.l2)<<" r2="<<int(p.r2)<<"\n";
      }
      session.tick(now());
    }
    session.disconnect(); std::cout<<"valid_reports="<<valid<<"\n";
    return valid ? 0 : 3;
  }
  if(read) { std::cerr<<"Requested original wired Xbox 360 device not found\n"; return 3; }
  std::cout<<"Enumeration complete; only 045e:028e candidates shown.\n"; return 0;
}
