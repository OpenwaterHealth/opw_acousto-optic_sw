#pragma once
#include "system/component/inc/octopus.h"

#include "system/third_party/json-develop/single_include/nlohmann/json.hpp"


// Encapsulate all the expertise on octopus timers for scanning
class OctopusManager_USscanner {
 public:
  OctopusManager_USscanner() {}
  
  virtual ~OctopusManager_USscanner();

  using json = nlohmann::json;

  virtual bool init(const json& systemParameters);

  int Trigger();

  int EnableUSTxTimer(bool channelON);

  bool InitializeOctopus(const json& systemParameters);

  bool DisableOutput(Octopus::Timer timer, Octopus::Pin pin);

 private:
  Octopus* octopus_;
  
  // Timers/pins that are for internally triggering and gating other stuff
  Octopus::Timer timer_SWtrigger_;  // SW trigger to initiate synchronous voxel collection
  Octopus::Pin pin_SWtrigger_ = Octopus::Pin::OUT8_TOP;
  
  // Pins that are actually connected to stuff
  Octopus::Timer timer_USTx_;  // SW trigger to initiate synchronous voxel collection
  Octopus::Pin pin_USTx_ = Octopus::Pin::OUT2_BOTTOM; 
};
