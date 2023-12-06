#pragma once

#include "laser.h"

#include <string>

#include "rs232_wrapper.h"

class MoglabsLaserDriver : public Laser {
 public:
  MoglabsLaserDriver() {}
  ~MoglabsLaserDriver();

  // Open the comPort here, rather than in the constructor.
  virtual bool init(int comPort);

  bool initializeLaser(const json& systemParameters) override;

  // ToDo (CR): Test new moglabs communication/drivers and make read function for current,volt,temp

 protected:
  // Send a string out through the interface.
  int sendString(const std::string& str);

  RS232* rs232_;
};
