#pragma once

#include "system/component/inc/octopus.h"

#include "system/third_party/json-develop/single_include/nlohmann/json.hpp"

// Encapsulate all the expertise on octopus timers for the align tool
class OctopusManager_Align {
 public:
  OctopusManager_Align() {}
  ~OctopusManager_Align();

  enum AOM_ShiftDirection {
    UPSHIFT,
    DOWNSHIFT,
    ULTRASOUND,  // Note: channel 3 of AOM output controls ultrasound in structural scanners, and 2nd laser for bloodflow systems
    CHOP
  };

  struct AOM_Settings {
    double AOM1Freq_Hz = NULL;
    double AOM1Volt_V = NULL;
    double AOM2Freq_Hz = NULL;
    double AOM2Volt_V = NULL;
    double AOM3Freq_Hz = NULL;
    double AOM3Volt_V = NULL;
    double AOM4Freq_Hz = NULL;
    double AOM4Volt_V = NULL;
  };

  // Create octopus manager to drive realtime FFT signals for pulsed and pseudo-pulsed systems
  bool init(bool pulsedSystem, bool pseudoPulsedSystem, bool bloodflowSystem, int numCameras, AOM_Settings aomSettings,
    double USDelay_s, double overlap_s, double cameraFSINDelay_s, double AOMDelay_s);

  // Initialize octopus to drive AOMs, ultrasound and camera for pulsed and pseudo-pulsed system align tool
  bool InitializeOctopus(bool pulsedSystem, bool pseudoPulsedSystem, bool bloodflowSystem, int numCameras, AOM_Settings aomSettings,
    double USDelay_s, double overlap_s, double cameraFSINDelay_s, double AOMDelay_s);

  // Turn on and off all octopus pins in systemTimers_ vector
  bool EnableSystemChannels(bool channelON);

  // Turn off single octopus pin
  bool DisableOutput(Octopus::Timer timer, Octopus::Pin pin);

  // Set pin used to trigger voxel acquisition (may be software timer or external input signal)
  bool SetVoxelTriggerPin(Octopus::Pin triggerPin);

  // Turn on or off AOM pin, used for aligning AOM plate and changing pseudo-pulse widths in fftutil
  bool EnableAOM(AOM_ShiftDirection shiftDirection, bool channelON, double delay, double width);

  // Set frequency of AOM, set voltage of AOM 
  // In bloodlfow systems this is used for optimizing the chopping aom
  // In structural systems this is used for checking ROI without bypass mirrors by changing relative frequency shift of reference beam
  bool SetAOM(AOM_ShiftDirection shiftDirection, double frequency_Hz, double voltage_Vp);

  // Change delay to ultrasound trigger, and turn on/off (used in fftutil)
  bool SetUltrasoundTriggerDelay(double delay_s, bool channelON);

 private:
  Octopus* octopus_;
  struct systemTimer {
    Octopus::Timer timer;
    Octopus::Pin pin;
  };
  systemTimer tempSystemTimer_;
  std::vector<systemTimer> systemTimers_;  // Vector of all timers and pins, used to turn on&off

  // Timers/pins that are for internally triggering and gating other stuff
  Octopus::Timer timer_SWtrigger_;  // In pseudo-pulsed system, this continuously drives voxel acquisition instead of laser to IN1_TOP
  // Timers and pins for AOM gates
  Octopus::AOM AOMupshift_;  // Upshift AOM [100MHz]
  Octopus::AOM AOMdownshift_;  // Downshift AOM
  Octopus::AOM AOMultrasound_;  // Drives amplifier for ultrasound, or for 2nd pseudo-pulse chopping AOM in dual laser system
  Octopus::AOM AOMpulseChop_;  // Drives amplifier for pseudo-pulse chopping AOM
  Octopus::Timer timer_AOMdownshiftGate_; // SW gate for AOM on time
  Octopus::Timer timer_AOMupshiftGate_; // SW gate for AOM on time
  Octopus::Timer timer_AOMpulseChopGate_; // SW gate for AO on time
  Octopus::Timer timer_AOMultrasoundGate_; // SW gate for AO on time
  Octopus::Pin pin_AOMpulseChopGate_ = Octopus::Pin::OUT5_TOP;
  Octopus::Pin pin_AOMultrasoundGate_ = Octopus::Pin::OUT5_BOTTOM;
  Octopus::Pin pin_AOMupshiftGate_ = Octopus::Pin::OUT6_TOP;
  Octopus::Pin pin_AOMdownshiftGate_ = Octopus::Pin::OUT7_TOP;

  // Pins that are actually connected to stuff
  std::vector<Octopus::Pin> pins_cameraFSIN_ = { Octopus::Pin::OUT1_BOTTOM, Octopus::Pin::OUT2_BOTTOM,
  Octopus::Pin::OUT3_BOTTOM, Octopus::Pin::OUT4_BOTTOM };
  Octopus::Timer timer_USTx_; // Trigger to ultrasound
  Octopus::Pin pin_USTx_ = Octopus::Pin::OUT2_BOTTOM;
  // AOM channel connections
  int channel_AOMupshift_ = 1;
  int channel_AOMdownshift_ = 2;
  int channel_AOMultrasound_ = 3;  //Used to drive fixed-focus ultrasound amplifier or 2nd laser in dual-laser bloodflow system
  int channel_AOMpulseChop_ = 4;

  // Asynchronous (Pulsed Laser) related pins
  Octopus::Pin pin_voxelTrigger_ = Octopus::Pin::LOW;  // Default to no triggers for safety
};
