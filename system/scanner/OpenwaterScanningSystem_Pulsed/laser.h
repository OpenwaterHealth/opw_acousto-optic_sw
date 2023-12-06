#pragma once

#include "system/third_party/json-develop/single_include/nlohmann/json.hpp"

class Laser {
 public:
  using json = nlohmann::json;

  Laser() {}
  virtual ~Laser() {}

  virtual bool initializeLaser(const json& systemParameters) = 0;

  virtual bool enableChannel(int channelNum, bool channelON) = 0;
};
