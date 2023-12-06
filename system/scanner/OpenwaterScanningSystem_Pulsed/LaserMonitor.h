#pragma once

#include <tuple>

#include "system/third_party/json-develop/single_include/nlohmann/json.hpp"

class LaserMonitor {
 public:
  using json = nlohmann::json;
  LaserMonitor() {}
  virtual ~LaserMonitor() {}

  virtual int initializeDevice(const json& systemParameters) = 0;

  virtual std::vector<double> getDataStats() = 0;

  static const int numLaserMonitorStats_ = 8;
};
