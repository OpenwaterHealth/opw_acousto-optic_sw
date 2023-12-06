#include "moglabs.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <thread>

#include "rs232_wrapper.h"

MoglabsLaserDriver::~MoglabsLaserDriver() {
  delete rs232_;
  rs232_ = NULL;
}

// TODO(jfs): Init via shared_ptr to constructor
// TODO(jfs): Add init() to unit test.
bool MoglabsLaserDriver::init(int port) {
  if (!rs232_) {  // in case it's mocked
    rs232_ = new RS232();
  }
  return rs232_->Open(port - 1, 38400, "8N1") == 0;
}

int MoglabsLaserDriver::sendString(const std::string& str) {
  int sendBufOK = rs232_->SendString(str);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  return sendBufOK > 0;
}

bool MoglabsLaserDriver::initializeLaser(const json& systemParameters) {
  int moglabsLaserCOM = systemParameters["hardwareParameters"]["laserCOM"].get<int>();

  // Initialize Unit
  return init(moglabsLaserCOM);
}
