#pragma once
#include "OctopusManager_USscanner.h"
#include "ustxManager.h"

#include "system/third_party/json-develop/single_include/nlohmann/json.hpp"
#include "system/scanner/OpenwaterScanningSystem_Pulsed/robot.h"

using json = nlohmann::json;

class RobotScanner_USscanner {
 public:
  RobotScanner_USscanner();
     
  ~RobotScanner_USscanner();

  // Implement
  bool init(const json& systemParameters);
  bool scan(const json& systemParameters, ustxManager_USscanner* ustxManager, OctopusManager_USscanner* octopusManager);
  bool CheckForCancel(const json& systemParameters);

 private:
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
  
  Robot* robot_ = NULL;
  time_t lastCancelCheckTime_ = 0;
};
