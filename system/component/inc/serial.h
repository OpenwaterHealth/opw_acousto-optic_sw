#pragma once

#include <cstdint>

// Use a forward reference to avoid bringing in windows.h
typedef void* HANDLE;

class Serial {
 public:
  Serial() {}
  virtual ~Serial();

  struct Init {
    int port = -1;
    int baud = 115200;
    int bits = 8;
    enum { NONE, EVEN, ODD, MARK, SPACE } parity = NONE;
    double stop_bits = 1.0;
    bool cts = false;
    bool rts = false;
  };

  // Open a serial port with parameters from the init structure
  // @param cfg configuration structure
  // @returns 0 on success
  virtual int Open(const Serial::Init& cfg);

  // Close the serial port
  virtual void Close();

  // Transmit data over the serial port
  // @param data pointer to bytes to send
  // @param len length of bytes to send.  -1 to send a null terminated string
  // @returns number of bytes actually written
  virtual int Transmit(const uint8_t* data, int len = -1);

  // convenience function
  int Transmit(const char* data, int len = -1) { return Transmit((const uint8_t*)data, len); }

  // Recieve data from the serial port
  // @param data where to put received bytes
  // @param max_bytes maximum number of bytes to receive
  // @returns number of bytes actually put in to the data buffer
  // @note will not block.
  virtual int Receive(uint8_t* data, int max_bytes);

 private:
  HANDLE port_ = nullptr;  // HANDLE on Win; use void* to avoid windows.h
};
