#pragma once

#include "laser.h"

#include <string>

#include "rs232_wrapper.h"

class Verdi : public Laser {
 public:
  Verdi() {}
  virtual ~Verdi();

  // Open the comPort here, rather than in the constructor.
  virtual bool init(int comPort);

  bool initializeLaser(const json& systemParameters);

  // Set channel 1 laser power in Watts
  bool setLaserPower(double laserPower_W);

  // Switch laser between standby and run
  bool setLaserRun(bool laserON);

  // Used to turn shutter off for channel 1 or channel 2
  bool enableChannel(int channelNum, bool channelON);

 protected:
  // Send a string out through the interface.
  bool sendString(const std::string& str);

  RS232* rs232_ = nullptr;
};
