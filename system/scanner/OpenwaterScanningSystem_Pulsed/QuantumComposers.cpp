#include "QuantumComposers.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <thread>

#include "rs232_wrapper.h"

QuantumComposers::~QuantumComposers() {
  delete rs232_;
  rs232_ = NULL;
}

bool QuantumComposers::init(int port) {
  if (!rs232_) {  // in case it's mocked
    rs232_ = new RS232();
  }
  return rs232_->Open(port - 1, 38400, "8N1") == 0;
}

bool QuantumComposers::sendString(const std::string& str) {
  int sendBufOK = rs232_->SendString(str);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  return sendBufOK > 0;
}

bool QuantumComposers::setUnitChannelGateMode(bool gatingON, double gateVoltage_V) {
  return sendString(gatingON ? ":PULS0:GAT:MOD CHAN\r\n" : ":PULS0:GAT:MOD DIS\r\n")
    && sendString(":PULS0:GAT:LEV " + std::to_string(gateVoltage_V) + "\r\n");
}

bool QuantumComposers::setPulseWidth(int channelNum, std::string pulseWidth_s) {
  return sendString(":PULS" + std::to_string(channelNum) + ":WIDT " + pulseWidth_s + "\r\n");
}

bool QuantumComposers::setPulseDelay(int channelNum, std::string pulseDelay_s) {
  return sendString(":PULS" + std::to_string(channelNum) + ":DEL " + pulseDelay_s + "\r\n");
}

bool QuantumComposers::gateChannel(int channelNum, bool gatingON) {
  if (gatingON) {
    return sendString(":PULS" + std::to_string(channelNum) + ":CGAT PULS\r\n")
      && sendString(":PULS" + std::to_string(channelNum) + ":CLOG HIGH\r\n");
  } else {
    return sendString(":PULS" + std::to_string(channelNum) + ":CGAT DIS\r\n");
  }
}

bool QuantumComposers::enableChannel(int channelNum, bool channelON) {
  return sendString(":PULS" + std::to_string(channelNum) + (channelON ? ":STAT ON\r\n" : ":STAT OFF\r\n"));
}

bool QuantumComposers::enableUnit(bool unitON) {
  return sendString(unitON ? ":PULS0:STAT ON\r\n" : ":PULS0:STAT OFF\r\n");
}

bool QuantumComposers::initializeLaser(const json& systemParameters) {
  std::string laser = systemParameters["laserParameters"]["laser"].get<std::string>();
  int amplitudeLaserCOM = systemParameters["hardwareParameters"]["laserCOM"].get<int>();
  std::string  chHWidth_s = systemParameters["delayParameters"]["QCchHWidth_s"].get<std::string>();
  std::string  chHDelay_s = systemParameters["delayParameters"]["QCchHDelay_s"].get<std::string>();

  bool channelON = true;

  // Initialize Unit
  init(amplitudeLaserCOM);
  //TODO(CR?): something is wrong with the Quantum Composers box at Explora. For now, programming manually.
  // Value of channel G delay is not being set correctly.
  // Have checked menu settings for usb vs rs232 comm as well as baud rate, neither fixed immediately. Will
  // need more thorough debug.

  if (laser.compare("Amplitude-Continuum-V2") == 0) {
    //std::string  chGWidth_s = systemParameters["delayParameters"]["QCchGWidth_s"].get<std::string>();
    //std::string  chGDelay_s = systemParameters["delayParameters"]["QCchGDelay_s"].get<std::string>();

    //// Channel G: To Himax FSIN [until FV moved to beginning of frame]
    //setPulseWidth(7, chGWidth_s);
    //setPulseDelay(7, chGDelay_s);
    enableChannel(7, channelON);
    //// Channel H: To Berkeley Nuclonics Trigger
    //setPulseWidth(8, chHWidth_s);
    //setPulseDelay(8, chHDelay_s);
    enableChannel(8, channelON);

    return sendString(":DISP:UPD ?\r\n");
  }
  std::cerr << "ERROR: Unknown laser type (" << laser << ")\n";
  return false;
}
