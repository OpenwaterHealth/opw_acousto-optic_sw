#pragma once

#include "laser.h"

#include <string>

#include "rs232_wrapper.h"

class QuantumComposers : public Laser {
 public:
  QuantumComposers() {}
  ~QuantumComposers();

  // Open the comPort here, rather than in the constructor.
  // TODO(jfs) : Init via shared_ptr in _ctand add init() to unit test.
  virtual bool init(int comPort);

  bool setUnitChannelGateMode(bool gatingON, double gateVoltage_V = 0.5);

  bool setPulseDelay(int channelNum, std::string pulseDelay_s);

  bool setPulseWidth(int channelNum, std::string pulseWidth_s);

  bool gateChannel(int channelNum, bool gatingON);

  bool enableChannel(int channelNum, bool channelON);

  bool enableUnit(bool unitON);

  bool initializeLaser(const json& systemParameters);

 protected:
  // Send a string out through the interface.
  bool sendString(const std::string& str);

  RS232* rs232_;
};
