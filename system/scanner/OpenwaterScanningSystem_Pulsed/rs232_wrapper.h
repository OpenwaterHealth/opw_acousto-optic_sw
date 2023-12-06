#pragma once

#include <string>

#include "system/third_party/rs232/rs232.h"

// Wrap a class around the bare C rs232 module.
class RS232 {
 public:
  RS232() {}
  virtual ~RS232() { if (port_ >= 0) RS232_CloseComport(port_); }

  virtual int Open(int port, int baud_rate, const char* mode) {
    port_ = port;
    return RS232_OpenComport(port, baud_rate, mode);
  }

  virtual int Port() const { return port_; }

  virtual int SendString(const std::string& str) {
    return RS232_SendBuf(port_, (unsigned char*)str.c_str(), (int)str.length());
  }

  virtual int Poll(unsigned char* buffer, int max_size) {
    return RS232_PollComport(port_, buffer, max_size);
  }

  virtual void FlushRXTX() { RS232_flushRXTX(port_); }

 private:
  int port_ = -1;
};
