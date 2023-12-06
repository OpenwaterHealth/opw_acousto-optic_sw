#pragma once
#include "system/component/inc/ustx.h"

#include "system/third_party/json-develop/single_include/nlohmann/json.hpp"


// Encapsulate all the expertise on ustx for scanning
class ustxManager_USscanner {
 public:
  ustxManager_USscanner();
  virtual ~ustxManager_USscanner();

  using json = nlohmann::json;

  virtual bool init(const json& systemParameters);
  void resetFocus();
  void isThermalShutdown();

 private:
  USTx* ustx_ = NULL;

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
};
