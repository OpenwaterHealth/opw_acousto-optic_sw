//
// ConexStage controller
//
// Reference: https://www.newport.com/mam/celum/celum_assets/resources/CONEX-CC_-_Controller_Documentation.pdf?1

#include "ConexStage.h"

#include <cmath>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <thread>

#include "system/third_party/rs232/rs232.h"

ConexStage::ConexStage() {
}

ConexStage::~ConexStage() {
}

bool ConexStage::init(int port) {
  if (!rs232_) {  // in case it's mocked
    rs232_ = new RS232();
  }
  return rs232_->Open(port - 1, 921600, "8N1") == 0;
}

int ConexStage::stageMoving() {
  const unsigned char readyFromHoming = '2';
  const unsigned char readyFromMoving = '3';
  const unsigned char nrFromReset = 'A';
  const unsigned char disFromReady = 'C';
  const unsigned char disFromMoving = 'D';  // Note: could also be NR from Disabled
  const unsigned char nrFromMoving = 'F';
  const unsigned char nrFromReady = 'E';
  const unsigned char nr = '0';

  for (bool moving = true; moving; ) {
    rs232_->SendString("1TS\r\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    unsigned char stagePollBuf[10] = {};
    int poll = rs232_->Poll(stagePollBuf, sizeof stagePollBuf);
    if (poll == 10) {
      // Warn on detected errors (per ref PDF, above). The last two, the "end of run" errors,
      // are likely fatal. TODO(cregan/jfs): Kill the scan on these errors?
      unsigned int // B = std::stoul(std::to_string(stagePollBuf[4]), NULL, 16),
                   // C = std::stoul(std::to_string(stagePollBuf[5]), NULL, 16),
                   D = std::stoul(std::to_string(stagePollBuf[6]), NULL, 16);
      int port = this->getCOMPortNum() + 1;
      std::string prefix = "WARNING: Stage error (COM" + std::to_string(port) + "): ";
      #if 0  // These occur often and are not worth logging.
      if (B & 2) { std::cerr << prefix << "80 W output power exceeded.\n" << std::flush; }
      if (B & 1) { std::cerr << prefix << "DC voltage too low.\n" << std::flush; }
      if (C & 8) { std::cerr << prefix << "Wrong ESP stage.\n" << std::flush; }
      if (C & 4) { std::cerr << prefix << "Homing time out.\n" << std::flush; }
      if (C & 2) { std::cerr << prefix << "Following error.\n" << std::flush; }
      if (C & 1) { std::cerr << prefix << "Short circuit detection.\n" << std::flush; }
      if (D & 8) { std::cerr << prefix << "RMS current limit.\n" << std::flush; }
      if (D & 4) { std::cerr << prefix << "Peak current limit.\n" << std::flush; }
      #endif
      if (D & 2) { std::cerr << prefix << "Positive end of run.\n" << std::flush; }
      if (D & 1) { std::cerr << prefix << "Negative end of run.\n" << std::flush; }

      stagePollBuf[9] = 0;  // Null terminate strings
      if (stagePollBuf[8] == readyFromHoming ||
          stagePollBuf[8] == readyFromMoving ||
          stagePollBuf[8] == nrFromReset) {
        moving = false;
      } else if (stagePollBuf[8] == disFromReady ||
        stagePollBuf[8] == disFromMoving ||
        stagePollBuf[8] == nrFromMoving ||
        stagePollBuf[8] == nr) {
        return -1;
      } else if (stagePollBuf[7] == nr &&
        stagePollBuf[8] == nrFromReady) {
        return -1;
      }
    }
    //  RS232 library recommends 100ms pause between com port reads
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    rs232_->FlushRXTX();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }

  return false;
}

double ConexStage::getStageLocation(void) {
  // Note: still not working great, not used in actual scan code until more
  // carefully debugged
  unsigned char stagePollBuf[10] = {};
  double location_mm;

  rs232_->SendString("1TP\r\n");
  // Magic pause; 10ms recommended in documentation, but appears insufficient
  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  int len = rs232_->Poll(stagePollBuf, sizeof stagePollBuf);
  if (len != sizeof stagePollBuf) {
    printf("WARNING: Poll returns wrong length (%d).", len);
  }
  stagePollBuf[9] = 0;  // Null terminate strings

  // Scan 5 digits + decimal place [skip 1st 3 characters: 1TP]
  location_mm = atof((char*)stagePollBuf + 3);
  rs232_->FlushRXTX();  // Flush buffer or it will interfere with stageMoving()
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  return location_mm;
}

int ConexStage::resetController() {
  rs232_->SendString("1RS\r\n");
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  while (stageMoving()) {}
  return 1;
}

int ConexStage::moveHome() {
  rs232_->SendString("1OR\r\n");
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  while (stageMoving()) {}
  return 1;
}

int ConexStage::disableController() {
  rs232_->SendString("1MM0\r\n");
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  return 1;
}

int ConexStage::moveRelative(double distance_mm) {
  char stageWrite[15];
  snprintf(stageWrite, sizeof(stageWrite), "1PR%g\r\n", distance_mm);
  rs232_->SendString(stageWrite);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  while (stageMoving()) {}
  return 1;
}

int ConexStage::moveAbsolute(double location_mm) {
  char stageWrite[15];
  snprintf(stageWrite, sizeof(stageWrite), "1PA%g\r\n", location_mm);
  rs232_->SendString(stageWrite);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  int isMoving = stageMoving();
  while (isMoving) {
    if (isMoving == -1) {
      // Reset from error
      resetController();
      std::this_thread::sleep_for(std::chrono::milliseconds(500));  // Required after reset
      std::cout << "ERROR: Resetting stage on COM " << this->getCOMPortNum() + 1 << " after error.\n" << std::flush;
      moveHome();
      rs232_->SendString(stageWrite);  // If stage errors, after reset move back to desired location
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    isMoving = stageMoving();
  }
  return 1;
}
