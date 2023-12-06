#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <thread>

#include <glog/logging.h>

#include "system/component/inc/time.h"

#include "rs232_wrapper.h"
#include "Verdi.h"

Verdi::~Verdi() {
  delete rs232_;
  rs232_ = NULL;
}

bool Verdi::init(int port) {
  if (!rs232_) {
    rs232_ = new RS232();
  }
  return rs232_->Open(port - 1, 19200, "8N1") == 0;
}

bool Verdi::sendString(const std::string& str) {
  int sendBufOK = rs232_->SendString(str);
  Component::SleepMs(1000);
  return sendBufOK;
}

bool Verdi::setLaserPower(double laserPower_W) {
  if (laserPower_W >= 0.01 && laserPower_W <= 4.0) {
    LOG(INFO) << "Setting laser power to: " << laserPower_W << std::flush;
    return sendString("P:" + std::to_string(laserPower_W) + "\r\n");
  } else {
    LOG(ERROR) << "Laser power must be between 0.01W - 4.0W: " << laserPower_W << std::flush;
    return false;
  }
}

bool Verdi::setLaserRun(bool laserON) {
  if (laserON) {
    LOG(INFO) << "Turning laser from STANDBY to ON" << std::flush;
    return sendString("L:1\r\n");
  } else {
    LOG(INFO) << "Reducing laser power and turning laser from ON to STANDBY" << std::flush;
    return sendString("P:0.01\r\n") && sendString("L:0\r\n");
  }
}

bool Verdi::initializeLaser(const json& systemParameters) {
  int verdiLaserCOM = systemParameters["hardwareParameters"]["laserCOM"].get<int>();
  double laserPower_W = systemParameters["laserParameters"]["laserPower_W"].get<double>();

  // Initialize Unit
  if (!init(verdiLaserCOM)) return false;

  // Turn laser up to power
  if (!setLaserPower(laserPower_W)) return false;

  // Turn laser from STANDBY to ON
  return setLaserRun(true);
}

bool Verdi::enableChannel(int channelNum, bool channelON) {
  if (channelON) {
    LOG(INFO) << "Opening laser shutter" << std::flush;
    return sendString("S:1\r\n");
  } else {
    LOG(INFO) << "Closing laser shutter" << std::flush;
    return sendString("S:0\r\n");
  }
}
