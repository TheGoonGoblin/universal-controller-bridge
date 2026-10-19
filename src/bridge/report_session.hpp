// SPDX-License-Identifier: MIT
#pragma once
#include "controllers/xbox360.hpp"
namespace ucb {
// Single-threaded report publication with an injectable monotonic millisecond clock.
// Owns one bridged player; cannot publish into native slot 0.
class ReportSession {
 public:
  explicit ReportSession(ControllerBridge& bridge, size_t slot=1, uint64_t timeout_ms=1000)
      : bridge_(bridge), slot_(slot), timeout_(timeout_ms) {
    if(slot==0 || slot>=ControllerBridge::capacity || timeout_ms==0)
      throw std::invalid_argument("invalid bridged slot or timeout");
  }
  bool receive(const uint8_t* data, size_t length, uint64_t now_ms) {
    if (seen_ && now_ms<last_) return false;
    const auto sample=xbox360::decode(data,length);
    if(!sample) return false; // malformed packets do not refresh the watchdog
    bridge_.submit(slot_,*sample); last_=now_ms; seen_=true; return true;
  }
  void tick(uint64_t now_ms) {
    if(seen_ && (now_ms<last_ || now_ms-last_>=timeout_)) disconnect();
  }
  void disconnect() { bridge_.disconnect(slot_); seen_=false; }
 private:
  ControllerBridge& bridge_;
  size_t slot_;
  uint64_t timeout_, last_=0;
  bool seen_=false;
};
}
