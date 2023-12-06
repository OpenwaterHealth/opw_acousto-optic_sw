#pragma once

#include "system/third_party/json-develop/single_include/nlohmann/json.hpp"

#include "system/component/inc/octopus.h"

#include "trigger.h"

// Encapsulate all the expertise on octopus timers for scanning
class OctopusManager {
 public:
  OctopusManager() {}
  virtual ~OctopusManager();

  using json = nlohmann::json;

  virtual bool init(const json& systemParameters, int numAxialFoci, Trigger* trigger);

  int TriggerVoxel();

  bool InitializeOctopus(const json& systemParameters, int numAxialFoci);

  bool EnableSystemChannels(bool channelON);

  bool SetVoxelTriggerPins(Octopus::Pin triggerPin, Octopus::Pin gatePin);

  bool DisableOutput(Octopus::Timer timer, Octopus::Pin pin);

 private:
  Octopus* octopus_;
  Trigger* trigger_;
  struct systemTimer {
    Octopus::Timer timer;
    Octopus::Pin pin;
  };
  systemTimer tempSystemTimer_;
  std::vector<systemTimer> systemTimers_;  // Vector of all timers and pins, used to turn on&off

  // Timers/pins that are for internally triggering and gating other stuff
  Octopus::Timer timer_SWtrigger_;  // SW trigger to initiate synchronous voxel collection
  Octopus::Pin pin_SWtrigger_ = Octopus::Pin::OUT8_TOP;
  Octopus::Pin pin_AOMgate_ = Octopus::Pin::OUT6_TOP;
  Octopus::Pin pin_AOMpulseChopGate_ = Octopus::Pin::OUT5_TOP;

  // Pins that are actually connected to stuff
  Octopus::Pin pin_cameraFSIN_ =  Octopus::Pin::OUT1_BOTTOM;
  Octopus::Pin pin_cameraFrameValid_ = Octopus::Pin::OUT1_TOP;
  Octopus::Pin pin_USTx_ = Octopus::Pin::OUT2_BOTTOM;
  int channel_AOMupshift_ = 1;
  int channel_AOMdownshift_ = 2;
  int channel_AOMultrasound_ = 3;
  int channel_AOMpulseChop_ = 4;

  // Asynchronous (Pulsed Laser) related pins
  Octopus::Pin pin_voxelTrigger_ = Octopus::Pin::LOW;  // Default to no triggers for safety
  Octopus::Pin pin_voxelGate_ = Octopus::Pin::LOW;  // Default to no gate for safety
};
