// Scanner class, encapsulating the various device modules (laser, etc.)

#pragma once

#include "system/third_party/json-develop/single_include/nlohmann/json.hpp"

#include "system/component/inc/ustx.h"
#include "system/component/inc/intelhex.h"
#include "system/component/inc/serial.h"

#include "CameraManager.h"
#include "delays.h"
#include "laser.h"
#include "trigger.h"

class OctopusManager;

class Scanner : public Trigger {
 public:
  using json = nlohmann::json;
  Scanner(
      const json& systemParameters, Laser* laser, BerkeleyNucleonics* delays, 
      USTx* ustx, CameraManager* cameras, OctopusManager* octopusManager);
  virtual ~Scanner();

  // Initialize all the things.
  virtual bool init();

  // Run a scan.
  virtual bool scan() = 0;

 protected:
  struct Range {
    double init, length, step;
    std::vector<double> values;

    Range(const json& params, const char* initName, const char* lengthName, const char* stepName)
      : init(params[initName].get<double>()), length(params[lengthName].get<double>()),
        step(params[stepName].get<double>()) {
      for (int i = 0, n = (int)floor(length / step); i <= n; i++) {  // means len(v) = n+1
        values.push_back(init + (i * step));
      }
    }
    double operator[](int i) { return values[i]; }
  };

  // Initialize components.
  bool initializeDelayGenerator();
  bool initializeUSTx();
  void triggerAcquisition();

  // Start/End a scan.
  bool startScan();
  bool endScan();

  // Check for a cancel "signal".
  bool CheckForCancel();

  json systemParameters_;
  Laser* laser_;
  BerkeleyNucleonics* delayGenerator_;
  USTx *ustx_;
  CameraManager* cameras_;
  OctopusManager* octopusManager_;
  time_t lastCancelCheckTime_ = 0;
  int numFociPerSlice_ = 1;
  int numAxialFoci_ = 1;
  std::ofstream imageInfoLocal_;
  std::ofstream imageInfoSynced_;
  std::ofstream repeatedVoxelLog_;
};
