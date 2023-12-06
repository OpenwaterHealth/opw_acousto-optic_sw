#pragma once

#include <string>

#include "rs232_wrapper.h"

enum PulseMode {
  CONTINUOUS,
  SINGLE,
  BURST,
  DUTY_CYCLE
};

class BerkeleyNucleonics {
 public:
  BerkeleyNucleonics();
  virtual ~BerkeleyNucleonics();

  // Open the comPort here, rather than in the constructor.
  virtual bool init(int comPort);

  bool setUnitPulseMode(PulseMode pulseMode);

  bool setUnitExternalTriggerMode(bool trigON);

  bool setPulseDelay(int channelNum, const std::string&);

  bool setPulseWidth(int channelNum, const std::string& pulseWidth_s);

  bool setPulseAmplitude(int channelNum, double pulseAmplitude_V);

  bool muxChannels(int channelNum1, int channelNum2);

  bool enableChannel(int channelNum, bool channelON);

  bool enableUnit(bool unitON);

  bool displayUpdate(void);

  bool setChannelActiveHigh(int channelNum, bool activeHighON);

  bool setUnitPulseRate(double pulseRepRate_Hz);

  bool setUnitExternalClock(int externalClockRate_MHz, bool externalClockON);

  int getModelNumber();

  bool setModelNumber();

  bool resetUnit();

  bool setUnitChannelGateMode(bool gatingON);

  bool gateChannel(int channelNum, bool gatingON);

  bool setNumberBurstPulses(int numPulses);

  // Return error count (for error checking after a long sequence of setup calls).
  int errors() { return errors_; }

 protected:
  // Send a string out through the interface.
  bool sendString(const std::string& str);

  int modelNumber_ = 0;  // BNC 577 vs 525
  int errors_ = 0;

  RS232 *rs232_;
};
