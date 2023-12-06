#include "system/component/inc/serial.h"

#include <cstdio>

#ifdef _MSC_VER

#include <windows.h>

Serial::~Serial() { Close(); }

// largely cribbed from http://members.ee.net/brey/Serial.pdf
int Serial::Open(const Serial::Init& cfg) {
  char buf[80];
  sprintf_s(buf, 80, "\\\\.\\COM%d", cfg.port);

  // https://support.microsoft.com/en-us/help/115831/howto-specify-serial-ports-larger-than-com9?wa=wsignin1.0
  port_ = CreateFileA(buf, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                      0, NULL);

  DCB dcb = {0};
  if (!GetCommState(port_, &dcb)) {
    printf("Could not open COM port: %d\n", cfg.port);
    return -1;
  }

  dcb.BaudRate = cfg.baud;
  dcb.ByteSize = cfg.bits;
  switch (cfg.parity) {
    case Init::NONE: {
      dcb.Parity = NOPARITY;
    } break;
    case Init::EVEN: {
      dcb.Parity = (BYTE)PARITY_EVEN;
    } break;
    case Init::ODD: {
      dcb.Parity = (BYTE)PARITY_ODD;
    } break;
    case Init::MARK: {
      dcb.Parity = (BYTE)PARITY_MARK;
    } break;
    case Init::SPACE: {
      dcb.Parity = (BYTE)PARITY_SPACE;
    } break;
  }

  if (cfg.stop_bits < 1.25) {
    dcb.StopBits = ONESTOPBIT;
  } else if (cfg.stop_bits < 1.75) {
    dcb.StopBits = ONE5STOPBITS;
  } else {
    dcb.StopBits = TWOSTOPBITS;
  }

  // turn on RTS permanently if no hardware flow control
  dcb.fRtsControl = cfg.rts ? RTS_CONTROL_HANDSHAKE : RTS_CONTROL_ENABLE;
  dcb.fOutxCtsFlow = cfg.cts;

  if (!SetCommState(port_, &dcb)) {
    printf("Could not configure COM port: %d\n", cfg.port);
    return -1;
  }

  // all timeouts being zero except ReadIntervalTimeout leads to non-blocking read
  // https://docs.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-commtimeouts
  COMMTIMEOUTS timeout = {0};
  timeout.ReadIntervalTimeout = MAXDWORD;

  if (!SetCommTimeouts(port_, &timeout)) {
    printf("Could not configure timings COM port: %d\n", cfg.port);
    return -1;
  }

  return 0;
}


void Serial::Close() {
  if (port_) CloseHandle(port_);
  port_ = NULL;
}


int Serial::Transmit(const uint8_t* data, int len) {
  if (len < 0) len = (int)strlen((const char*)data);

  DWORD bytes_written;
  if (WriteFile(port_, data, (DWORD)len, &bytes_written, NULL)) {
    return (int)bytes_written;
  }

  return -1;
}


int Serial::Receive(uint8_t* data, int len) {
  DWORD bytes_read;
  if (ReadFile(port_, data, (DWORD)len, &bytes_read, NULL)) {
    return (int)bytes_read;
  }

  return -1;
}

#else

Serial::~Serial() {}
int Serial::Open(const Serial::Init& cfg) { return 0; }
void Serial::Close() {}
int Serial::Transmit(const uint8_t* data, int len) { return len; }
int Serial::Receive(uint8_t* data, int max_bytes) { return 1; }

#endif
