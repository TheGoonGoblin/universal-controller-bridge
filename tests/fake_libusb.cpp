// SPDX-License-Identifier: MIT
// Test-only fake backend. Never use as a USB implementation or PS5 binary.
#include <libusb.h>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <thread>
namespace {
int tag=0,transfers=0;
bool mode(const char* s) { const char* v=std::getenv("UCB_TEST_MODE"); return v && std::strcmp(v,s)==0; }
libusb_device* device() { return reinterpret_cast<libusb_device*>(&tag); }
libusb_endpoint_descriptor endpoint{};
libusb_interface_descriptor alt{};
libusb_interface iface{};
libusb_config_descriptor config{};
}
extern "C" {
int LIBUSB_CALL libusb_init(libusb_context** c) { *c=reinterpret_cast<libusb_context*>(&tag); return mode("init_error")?LIBUSB_ERROR_OTHER:0; }
void LIBUSB_CALL libusb_exit(libusb_context*) {}
ssize_t LIBUSB_CALL libusb_get_device_list(libusb_context*,libusb_device*** list) {
  static libusb_device* devices[]{device(),nullptr}; *list=devices; return mode("empty")?0:1;
}
void LIBUSB_CALL libusb_free_device_list(libusb_device**,int) {}
int LIBUSB_CALL libusb_get_device_descriptor(libusb_device*,libusb_device_descriptor* d) {
  *d={}; d->idVendor=0x045e;d->idProduct=0x028e;return 0;
}
uint8_t LIBUSB_CALL libusb_get_bus_number(libusb_device*) { return 1; }
uint8_t LIBUSB_CALL libusb_get_device_address(libusb_device*) { return 2; }
int LIBUSB_CALL libusb_get_active_config_descriptor(libusb_device*,libusb_config_descriptor** c) {
  endpoint={};endpoint.bEndpointAddress=0x83;endpoint.bmAttributes=3;endpoint.wMaxPacketSize=32;
  alt={};alt.bInterfaceNumber=0;alt.bInterfaceClass=0xff;alt.bInterfaceSubClass=0x5d;
  alt.bInterfaceProtocol=mode("wrong_interface")?2:1;alt.bNumEndpoints=1;alt.endpoint=&endpoint;
  iface={};iface.num_altsetting=1;iface.altsetting=&alt;
  config={};config.bNumInterfaces=1;config.interface=&iface;*c=&config;return 0;
}
void LIBUSB_CALL libusb_free_config_descriptor(libusb_config_descriptor*) {}
int LIBUSB_CALL libusb_open(libusb_device*,libusb_device_handle** h) {
  *h=nullptr;if(mode("open_error")) return LIBUSB_ERROR_ACCESS;
  *h=reinterpret_cast<libusb_device_handle*>(&tag); return 0;
}
void LIBUSB_CALL libusb_close(libusb_device_handle*) {}
int LIBUSB_CALL libusb_claim_interface(libusb_device_handle*,int) { return mode("claim_error")?LIBUSB_ERROR_BUSY:0; }
int LIBUSB_CALL libusb_release_interface(libusb_device_handle*,int) { return 0; }
int LIBUSB_CALL libusb_interrupt_transfer(libusb_device_handle*,unsigned char ep,unsigned char* data,int length,int* actual,unsigned int) {
  if(ep!=0x83 || length!=32) return LIBUSB_ERROR_INVALID_PARAM;
  if(mode("unplug")) return LIBUSB_ERROR_NO_DEVICE;
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  if(transfers++>0) { *actual=0;return LIBUSB_ERROR_TIMEOUT; }
  std::memset(data,0,20);data[1]=20;data[3]=16;data[5]=255;
  *actual=mode("malformed")?19:20;return 0;
}
const char* LIBUSB_CALL libusb_error_name(int code) {
  switch(code) { case LIBUSB_ERROR_ACCESS:return "LIBUSB_ERROR_ACCESS";
    case LIBUSB_ERROR_BUSY:return "LIBUSB_ERROR_BUSY";
    case LIBUSB_ERROR_NO_DEVICE:return "LIBUSB_ERROR_NO_DEVICE";
    default:return "LIBUSB_ERROR_OTHER"; }
}
}
