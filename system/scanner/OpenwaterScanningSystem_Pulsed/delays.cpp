#include "delays.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <thread>

#define GLOG_NO_ABBREVIATED_SEVERITIES 1  // handle windows.h ERROR redefinition
#include <glog/logging.h>

#include "system/component/inc/time.h"

BerkeleyNucleonics::BerkeleyNucleonics() {
}

BerkeleyNucleonics::~BerkeleyNucleonics() {
  delete rs232_;
  rs232_ = NULL;
}

bool BerkeleyNucleonics::init(int port) {
  if (!rs232_) rs232_ = new RS232();
  int status = rs232_->Open(port - 1, 115200, "8N1");
  if (status == 1) {
    LOG(ERROR) << "BNC: error initializing";
    return false;
  }
  return setModelNumber();
}

bool BerkeleyNucleonics::sendString(const std::string& str) {
  int stat = rs232_->SendString(str);
  Component::SleepMs(10);
  if (stat <= 0) {
    LOG(ERROR) << "BNC: error sending \'" << str << "\'";
    ++errors_;
    return false;
  }
  return true;
}

bool BerkeleyNucleonics::setUnitPulseMode(PulseMode pulseMode) {
  std::string boxWrite;
  switch (pulseMode) {
    case CONTINUOUS: boxWrite = ":PULS0:MOD NORM\r\n"; break;
    case SINGLE:     boxWrite = ":PULS0:MOD SING\r\n"; break;
    case BURST:      boxWrite = ":PULS0:MOD BURS\r\n"; break;
    case DUTY_CYCLE: boxWrite = ":PULS0:MOD DCYC\r\n"; break;
  }
  return sendString(boxWrite);
}

bool BerkeleyNucleonics::setUnitExternalTriggerMode(bool trigON) {
  if (modelNumber_ == 577) {
    return sendString(trigON ? ":PULS0:TRIG:MOD TRIG\r\n" : ":PULS0:TRIG:MOD DIS\r\n");
  } else if (modelNumber_ == 525) {
    return sendString(trigON ? ":PULS0:EXT:MOD TRIG\r\n" : ":PULS0:EXT:MOD DIS\r\n");
  }
  LOG(ERROR) << "Unknown BNC model (" << modelNumber_ << ")";
  ++errors_;
  return false;
}

bool BerkeleyNucleonics::setPulseDelay(int channelNum, const std::string& pulseDelay_s) {
  return sendString(":PULS" + std::to_string(channelNum) + ":DEL " + pulseDelay_s + "\r\n");
}

bool BerkeleyNucleonics::setPulseWidth(int channelNum, const std::string& pulseWidth_s) {
  return sendString(":PULS" + std::to_string(channelNum) + ":WIDT " + pulseWidth_s + "\r\n");
}

bool BerkeleyNucleonics::setPulseAmplitude(int channelNum, double pulseAmplitude_V) {
  return sendString(":PULS" + std::to_string(channelNum) + ":OUTP:MOD ADJ\r\n")
      && sendString(":PULS" + std::to_string(channelNum) + ":OUTP:AMPL "
                    + std::to_string(pulseAmplitude_V) + "\r\n");
}

bool BerkeleyNucleonics::muxChannels(int channel, int decimal) {
  return sendString(":PULS" + std::to_string(channel) + ":MUX " + std::to_string(decimal) + "\r\n");
}

bool BerkeleyNucleonics::enableChannel(int channelNum, bool channelON) {
  return sendString(
      ":PULS" + std::to_string(channelNum) + ":STAT " + (channelON ? "ON" : "OFF") + "\r\n")
      && sendString(":PULS" + std::to_string(channelNum) + ":POL NORM\r\n");
}

bool BerkeleyNucleonics::enableUnit(bool unitON) {
  return sendString(unitON ? ":PULS0:STAT ON\r\n" : ":PULS0:STAT OFF\r\n");
}

bool BerkeleyNucleonics::displayUpdate(void) {
  return sendString(":DISP:UPD?\r\n") > 0;
}

bool BerkeleyNucleonics::setChannelActiveHigh(int channelNum, bool activeHighON) {
  return sendString(":PULS" + std::to_string(channelNum) + ":POL " + (activeHighON ? "NORM" : "INV")
    + "\r\n");
}

bool BerkeleyNucleonics::setUnitPulseRate(double pulseRepRate_Hz) {
  double period_s = 1.0 / pulseRepRate_Hz;
  return sendString(":PULS0:PER " + std::to_string(period_s) + "\r\n");
}

bool BerkeleyNucleonics::setUnitExternalClock(int externalClockRate_MHz, bool externalClockON) {
  return sendString(":PULS0:ICL " + (externalClockON ? std::to_string(externalClockRate_MHz) : "S")
      + "\r\n");
}

int BerkeleyNucleonics::getModelNumber() {
  return modelNumber_;
}

bool BerkeleyNucleonics::setModelNumber() {
  sendString("*IDN?\r\n");

  unsigned char bncPollBuf[7] = {0};
  int poll = rs232_->Poll(bncPollBuf, sizeof bncPollBuf);
  if ( poll < 7 ) {
    LOG(ERROR) << "BNC poll returns '" << bncPollBuf << "'\n" << std::flush;
    ++errors_;
    return false;
  }

  modelNumber_ = atoi((char*)bncPollBuf + 4);
  rs232_->FlushRXTX();  // Flush buffer
  Component::SleepMs(20);

  return true;
}

bool BerkeleyNucleonics::resetUnit() {
  return sendString("*RST\r\n");
}

bool BerkeleyNucleonics::setUnitChannelGateMode(bool gatingON) {
  return sendString(
        ":PULS" + std::to_string(0) + ":GAT:MOD " + (gatingON ? "ENABLE" : "DIS") + "\r\n")
    && sendString(
        ":PULS" + std::to_string(0) + ":GAT:MOD " + (gatingON ? "CHPU" : "DIS") + "\r\n")
    && sendString(":PULS0:GAT:LOG HIGH\r\n")
    && sendString(":PULS0:GAT:LEV 0.5\r\n");
}

bool BerkeleyNucleonics::gateChannel(int channelNum, bool gatingON) {
  // Currently only set up for 577.
  return sendString(":PULS" + std::to_string(channelNum) + ":CGAT " + (gatingON ? "HIGH" : "DIS")
      + "\r\n");
}

bool BerkeleyNucleonics::setNumberBurstPulses(int numPulses) {
  return sendString(":PULS0:BCO " + std::to_string(numPulses) + "\r\n");
}
