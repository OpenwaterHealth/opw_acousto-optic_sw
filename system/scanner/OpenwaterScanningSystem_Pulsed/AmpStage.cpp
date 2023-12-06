//
// AmpStage driver
//
// Reference: https://appliedmotion.s3.amazonaws.com/Host-Command-Reference_920-0002V.pdf
//

#undef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1

#include "AmpStage.h"

#include <cmath>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <thread>

#include "system/component/inc/time.h"
#include "system/third_party/rs232/rs232.h"

AmpStage::AmpStage() {
}

AmpStage::~AmpStage() {
  delete rs232_;
  rs232_ = NULL;
}

bool AmpStage::sendCommand(const char *cmd) {
  char buf[64];
  buf[0] = 0;
  buf[1] = 7;
  int cmdLen = (int)strlen(cmd);
  strncpy(buf + 2, cmd, cmdLen);
  buf[2 + cmdLen] = '\r';
  return RS232_SendBuf(rs232_->Port(), (unsigned char*)buf, 2 + cmdLen + 1);
}

bool AmpStage::init(int port) {
  if (!rs232_) {  // in case it's mocked
    rs232_ = new RS232();
  }
  return rs232_->Open(port - 1, 9600, "8N1") == 0;
}

int AmpStage::stageMoving() {
  #if 0 // see fixes on jfs's win laptop, robot branch
  // Check for N successive equal returns.
  unsigned char lastResponse[64] = {0};
  for(int i = 0; i < 3; i++) {
    sendCommand("IP");
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    unsigned char response[64] = {0};
    int poll = rs232_->Poll(response, sizeof response);
    if (poll == 0) {
      std::cerr << "ERROR: No response from stage on COM" << rs232_->Port() + 1 << ".\n";
      return -1;
    }
    // Expecting IP=FFFFD8F0 or some such
    if (memcmp(response, lastResponse, sizeof(response)) != 0) return true;
    memcpy(lastResponse, response, sizeof(response));
  }
  #endif

  return false;
}

double AmpStage::getStageLocation(void) {
  return 0;
}

int AmpStage::resetController() {
  sendCommand("AR");  // Alarm Reset [todo: check if this screws up homing/encoder location]
  sendCommand("ME");  // Motor enable
  sendCommand("EG20000"); // Electronic gearing [set steps per revolution of stepper motor]
  sendCommand("AC25");  // Acceleration rate [revolutions per second / second]
  sendCommand("DC25"); // Deceleration rate [revolutions per second / second]
  sendCommand("VE0.5");  // Velocity [revolutions/second, 0.0042-80 in steps of 0.0042], note: if too slow motor will catch
  sendCommand("SH0H");  // Seek home
  std::cout << "INFO: Rotational stage homing" << std::endl;
  Component::SleepMs(4000);  // Homing should be max 2 seconds (4 revolutions of motor per 1 revolution of phantom at 0.5 rev/sec)
  sendCommand("EP0");  // Encoder position [note: EP0 must be followed by SP0]
  sendCommand("SP0");  // Set position [note: SP0 should follow EP0]
  Component::SleepMs(10);
  while (stageMoving()) {}
  return 1;
}

int AmpStage::moveHome() {
  sendCommand("FP0");  // Go to 0 location (should be sample home, not just motor home)
  std::cout << "INFO: sending rotational stage to 0 counts" << std::endl;
  Component::SleepMs(4000);
  return 1;
}

int AmpStage::disableController() {
  sendCommand("MD");
  Component::SleepMs(10);
  return 1;
}

int AmpStage::moveRelative(double angle_deg) {
  // use DI, FL or FL w/ argument. units: counts
  return 1;
}

int AmpStage::moveAbsolute(double angle_deg) {
  char buf[64];
  const double countsPerDegree = 80000. / 360.;
  int counts = int(angle_deg * countsPerDegree);
  snprintf(buf, sizeof(buf), "FP%d", counts);
  sendCommand(buf);
  // Take a longish pause, pending a working stageMoving().
  Component::SleepMs(4000);
  while (stageMoving()) {
  }
  return 1;
}
