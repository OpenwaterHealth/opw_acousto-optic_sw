#include <iostream>

#include "OctopusManager.h"

using json = nlohmann::json;

OctopusManager::~OctopusManager() {
  delete octopus_;
}

bool OctopusManager::init(const json& systemParameters, int numAxialFoci, Trigger* trigger) {
  octopus_ = new Octopus();
  int octopusOpened = octopus_->Open();
  if (octopusOpened == 0) {
    std::cout << "INFO: Connected to Octopus: " << octopus_->SerialNumber() << std::endl;
    trigger_ = trigger;
    return InitializeOctopus(systemParameters, numAxialFoci);
  } else {
    std::cout << "ERROR: Unable to open Octopus\n" << std::flush;
    return false;
  }
}


int OctopusManager::TriggerVoxel() {
  octopus_->ConfigureTimer(timer_SWtrigger_);
  octopus_->EnableTimer(pin_SWtrigger_, true);
  return 1;
}

bool OctopusManager::EnableSystemChannels(bool channelON) {
  for (systemTimer& t : systemTimers_) {
    if (channelON) {
      octopus_->EnableTimer(t.pin, channelON);
    } else {
      DisableOutput(t.timer, t.pin);
    }
  }
  return true;
}

bool OctopusManager::SetVoxelTriggerPins(Octopus::Pin triggerPin, Octopus::Pin gatePin) {
  pin_voxelTrigger_ = triggerPin;
  pin_voxelGate_ = gatePin;
  return true;
}

bool OctopusManager::InitializeOctopus(const json& systemParameters, int numAxialFoci) {
  ////////// System Dependent Delay Variables //////////
  double ms2s = 0.001; // milliseconds to seconds
  double mhz2hz = 1000000.0; // megahertz to hertz
  std::string ultrasoundAmp = systemParameters["ultrasoundParameters"]["ultrasoundAmp"].get<std::string>();
  double ultrasoundFreq_MHz = systemParameters["ultrasoundParameters"]["ultrasoundFreq_MHz"].get<double>() * mhz2hz;
  double ultrasoundVoltage_Vp = systemParameters["ultrasoundParameters"]["ultrasoundVoltage_V"].get<double>() / 2.0; // Convert from Vpp to Vp
  double frameLength_s = systemParameters["cameraParameters"]["frameLength_ms"].get<double>() * ms2s;
  double AOM1Freq_Hz = systemParameters["AOMParameters"]["AOM1Freq_MHz"].get<double>() * mhz2hz;
  double AOM2Freq_Hz = systemParameters["AOMParameters"]["AOM2Freq_MHz"].get<double>() * mhz2hz;
  double AOM4Freq_Hz = 0.0;
  double AOM1Volt_V = systemParameters["AOMParameters"]["AOM1Volt_V"].get<double>();
  double AOM2Volt_V = systemParameters["AOMParameters"]["AOM2Volt_V"].get<double>();
  double AOM4Volt_V = 0.0;
  double laserClockPeriod_s = systemParameters["laserParameters"]["laserClockPeriod_ms"].get<double>() * ms2s;
  double frameGatePeriod_s = double(numAxialFoci) / (1.0 / laserClockPeriod_s);

  const auto& delayParameters = systemParameters["delayParameters"];
  double TTLPulseWidth_s = delayParameters["TTLPulseWidth_s"].get<double>();
  double chADelay_s = std::stod(delayParameters["chADelay_s"].get<std::string>());  // Camera FSIN in cw systems
  double chBDelay_s = std::stod(delayParameters["chBDelay_s"].get<std::string>());  // Ultrasound
  // double chBWidth_s = std::stod(delayParameters["chBWidth_s"].get<std::string>());
  double chCDelay_s = std::stod(delayParameters["chCDelay_s"].get<std::string>());  // AOMs
  double chCWidth_s = std::stod(delayParameters["chCWidth_s"].get<std::string>());
  double chDDelay_s = std::stod(delayParameters["chDDelay_s"].get<std::string>());  // Camera Frame Valid in pulsed systems
  double chDWidth_s = std::stod(delayParameters["chDWidth_s"].get<std::string>());
  double chEDelay_s = std::stod(delayParameters["chEDelay_s"].get<std::string>());  // Pulse chopping AOM in pseudo-pulsed systems
  double chEWidth_s = std::stod(delayParameters["chEWidth_s"].get<std::string>());

  int pulsedSystem = systemParameters["laserParameters"]["pulsed"].get<int>();
  int pseudoPulsedSystem = systemParameters["laserParameters"]["pseudoPulsed"].get<int>();
  if (pulsedSystem) {
    // Pulsed system trigger from laser, SW gate
    SetVoxelTriggerPins(Octopus::Pin::IN1_BOTTOM, Octopus::Pin::OUT8_BOTTOM);
  } else if (pseudoPulsedSystem) {
    // Pseudo-pulsed system uses timer_SWclock as master clock, SW gate
    SetVoxelTriggerPins(Octopus::Pin::OUT7_TOP, Octopus::Pin::OUT8_BOTTOM);
  } else {
    std::cout << "Synchronous systems no longer supported" << std::endl;
    return false;
  }

  if (pseudoPulsedSystem) {
    // Voltage to AOM chopper
    AOM4Volt_V = systemParameters["AOMParameters"]["AOM4Volt_V"].get<double>();
    AOM4Freq_Hz = systemParameters["AOMParameters"]["AOM4Freq_MHz"].get<double>() * mhz2hz;
  }

  ////////// System Input Triggers //////////
  // SW Trigger
  memset(&timer_SWtrigger_, 0, sizeof(timer_SWtrigger_));
  timer_SWtrigger_.start_output_value = false;
  timer_SWtrigger_.end_output_value = false;
  timer_SWtrigger_.trigger.select = Octopus::Pin::HIGH;
  timer_SWtrigger_.gate.select = Octopus::Pin::HIGH;
  timer_SWtrigger_.gate.sense = Octopus::Timer::Gate::HIGH;
  timer_SWtrigger_.trigger.sense = Octopus::Timer::Trigger::HIGH;
  timer_SWtrigger_.state[1].output_value = true;
  timer_SWtrigger_.state[1].use_trigger = false;
  timer_SWtrigger_.state[1].next_state = 2;
  timer_SWtrigger_.state[1].period = 100e-6;
  timer_SWtrigger_.state[2].output_value = false;
  timer_SWtrigger_.state[2].use_trigger = false;
  timer_SWtrigger_.state[2].next_state = 0;
  timer_SWtrigger_.state[2].period = 1e-6;
  timer_SWtrigger_.state_transition_count = 3;
  timer_SWtrigger_.out_select = pin_SWtrigger_;
  tempSystemTimer_.timer = timer_SWtrigger_;
  tempSystemTimer_.pin = pin_SWtrigger_;
  systemTimers_.push_back(tempSystemTimer_);

  // SW Clock [used for pseudopulsed async systems]
  if (pseudoPulsedSystem) {
    Octopus::Timer timer_SWclock;
    memset(&timer_SWclock, 0, sizeof(timer_SWclock));
    timer_SWclock.start_output_value = false;
    timer_SWclock.end_output_value = false;
    timer_SWclock.trigger.select = Octopus::Pin::HIGH;
    timer_SWclock.trigger.sense = Octopus::Timer::Trigger::HIGH;
    timer_SWclock.gate.select = Octopus::Pin::HIGH;
    timer_SWclock.gate.sense = Octopus::Timer::Gate::HIGH;
    timer_SWclock.state[1].output_value = false;
    timer_SWclock.state[1].use_trigger = false;
    timer_SWclock.state[1].next_state = 2;
    timer_SWclock.state[1].period = 1e-6; // Delay before FSIN pulse
    timer_SWclock.state[2].output_value = true;
    timer_SWclock.state[2].use_trigger = false;
    timer_SWclock.state[2].next_state = 3;
    timer_SWclock.state[2].period = TTLPulseWidth_s;
    timer_SWclock.state[3].output_value = false;
    timer_SWclock.state[3].use_trigger = false;
    timer_SWclock.state[3].next_state = 1;
    timer_SWclock.state[3].period = laserClockPeriod_s - 1e-6; // voxel acquisition rate
    timer_SWclock.state_transition_count = -1; // Infinite
    timer_SWclock.out_select = pin_voxelTrigger_;
    octopus_->ConfigureTimer(timer_SWclock);
    tempSystemTimer_.timer = timer_SWclock;
    tempSystemTimer_.pin = pin_voxelTrigger_;
    systemTimers_.push_back(tempSystemTimer_);
  }

  // SW Gate [used for pulsed and pseudopulsed async systems]
  if (pulsedSystem) {
    Octopus::Timer timer_SWgate; // SW gate for asynchronous slice acquisition
    memset(&timer_SWgate, 0, sizeof(timer_SWgate));
    timer_SWgate.start_output_value = false;
    timer_SWgate.end_output_value = false;
    timer_SWgate.trigger.select = pin_SWtrigger_; // Each slice triggered by SW trigger for analogous operation to synchronous scanning
    timer_SWgate.gate.select = Octopus::Pin::HIGH;
    timer_SWgate.gate.sense = Octopus::Timer::Gate::HIGH;
    timer_SWgate.trigger.sense = Octopus::Timer::Trigger::HIGH; // WORK AROUND FOR RISING EDGE BUG
    timer_SWgate.state[1].output_value = true;
    timer_SWgate.state[1].use_trigger = true;
    timer_SWgate.state[1].next_state = 2;
    timer_SWgate.state[1].period = frameGatePeriod_s;
    timer_SWgate.state[2].output_value = false;
    timer_SWgate.state[2].use_trigger = false;
    timer_SWgate.state[2].next_state = 1;
    timer_SWgate.state[2].period = 1e-6;
    timer_SWgate.state_transition_count = -1;
    timer_SWgate.out_select = pin_voxelGate_;
    octopus_->ConfigureTimer(timer_SWgate);
    octopus_->EnableTimer(pin_voxelGate_, true);
    tempSystemTimer_.timer = timer_SWgate;
    tempSystemTimer_.pin = pin_voxelGate_;
    systemTimers_.push_back(tempSystemTimer_);
  } else if (pseudoPulsedSystem) {
    Octopus::Timer timer_SWgate; // SW gate for asynchronous slice acquisition
    memset(&timer_SWgate, 0, sizeof(timer_SWgate));
    timer_SWgate.start_output_value = false;
    timer_SWgate.end_output_value = false;
    timer_SWgate.trigger.select = pin_SWtrigger_; // Each slice triggered by SW trigger for analogous operation to synchronous scanning
    timer_SWgate.gate.select = Octopus::Pin::HIGH;
    timer_SWgate.gate.sense = Octopus::Timer::Gate::HIGH;
    timer_SWgate.trigger.sense = Octopus::Timer::Trigger::HIGH; // WORK AROUND FOR RISING EDGE BUG
    timer_SWgate.state[1].output_value = true;
    timer_SWgate.state[1].use_trigger = true;
    timer_SWgate.state[1].next_state = 2;
    timer_SWgate.state[1].period = frameGatePeriod_s;
    timer_SWgate.state[2].output_value = false;
    timer_SWgate.state[2].use_trigger = false;
    timer_SWgate.state[2].next_state = 1;
    timer_SWgate.state[2].period = 1e-6;
    timer_SWgate.state_transition_count = -1;
    timer_SWgate.out_select = pin_voxelGate_;
    octopus_->ConfigureTimer(timer_SWgate);
    octopus_->EnableTimer(pin_voxelGate_, true);
    tempSystemTimer_.timer = timer_SWgate;
    tempSystemTimer_.pin = pin_voxelGate_;
    systemTimers_.push_back(tempSystemTimer_);
  }

  // AOM gate
  Octopus::Timer timer_AOMgate; // SW gate for AOM on time
  memset(&timer_AOMgate, 0, sizeof(timer_AOMgate));
  timer_AOMgate.start_output_value = false;
  timer_AOMgate.end_output_value = false;
  timer_AOMgate.trigger.select = pin_voxelTrigger_;
  timer_AOMgate.trigger.sense = Octopus::Timer::Trigger::HIGH; // WORK AROUND FOR RISING EDGE BUG
  timer_AOMgate.gate.select = pin_voxelGate_;
  timer_AOMgate.gate.sense = Octopus::Timer::Gate::HIGH;
  timer_AOMgate.state[1].output_value = false;
  timer_AOMgate.state[1].use_trigger = true;
  timer_AOMgate.state[1].next_state = 2;
  if (pseudoPulsedSystem) {
    chCDelay_s = chEDelay_s;     // Use same gating for regular AOMs as for chopper in pseudo-pulsed systems
    chCWidth_s = chEWidth_s;
  } else if (pulsedSystem) {
    chCDelay_s = chBDelay_s; // Currently use same as ultrasound, may need to trigger sooner if signal is noisy
  }
  timer_AOMgate.state[1].period = chCDelay_s; // Delay before trigger to AOMs
  timer_AOMgate.state[2].output_value = true;
  timer_AOMgate.state[2].use_trigger = false;
  timer_AOMgate.state[2].next_state = 3;
  timer_AOMgate.state[2].period = chCWidth_s; // On time of AOMss
  timer_AOMgate.state[3].output_value = false;
  timer_AOMgate.state[3].use_trigger = false;
  timer_AOMgate.state[3].next_state = 1;
  timer_AOMgate.state[3].period = 1e-6;
  timer_AOMgate.state_transition_count = -1; // Infinite
  timer_AOMgate.out_select = pin_AOMgate_;
  octopus_->ConfigureTimer(timer_AOMgate);
  tempSystemTimer_.timer = timer_AOMgate;
  tempSystemTimer_.pin = pin_AOMgate_;
  systemTimers_.push_back(tempSystemTimer_);

  if (pseudoPulsedSystem) {
    Octopus::Timer timer_AOMpulseChopGate; // SW gate for pulse chopping AOM time
    memset(&timer_AOMpulseChopGate, 0, sizeof(timer_AOMpulseChopGate));
    timer_AOMpulseChopGate.start_output_value = false;
    timer_AOMpulseChopGate.end_output_value = false;
    timer_AOMpulseChopGate.trigger.select = pin_voxelTrigger_;
    timer_AOMpulseChopGate.trigger.sense = Octopus::Timer::Trigger::HIGH; // WORK AROUND FOR RISING EDGE BUG
    timer_AOMpulseChopGate.gate.select = pin_voxelGate_;
    timer_AOMpulseChopGate.gate.sense = Octopus::Timer::Gate::HIGH;
    timer_AOMpulseChopGate.state[1].output_value = false;
    timer_AOMpulseChopGate.state[1].use_trigger = true;
    timer_AOMpulseChopGate.state[1].next_state = 2;
    timer_AOMpulseChopGate.state[1].period = chEDelay_s; // Delay before trigger to pulse chopping AOM
    timer_AOMpulseChopGate.state[2].output_value = true;
    timer_AOMpulseChopGate.state[2].use_trigger = false;
    timer_AOMpulseChopGate.state[2].next_state = 3;
    timer_AOMpulseChopGate.state[2].period = chEWidth_s; // On time of pulse chopping AOM
    timer_AOMpulseChopGate.state[3].output_value = false;
    timer_AOMpulseChopGate.state[3].use_trigger = false;
    timer_AOMpulseChopGate.state[3].next_state = 1;
    timer_AOMpulseChopGate.state[3].period = 1e-6;
    timer_AOMpulseChopGate.state_transition_count = -1; // Infinite
    timer_AOMpulseChopGate.out_select = pin_AOMpulseChopGate_;
    octopus_->ConfigureTimer(timer_AOMpulseChopGate);
    tempSystemTimer_.timer = timer_AOMpulseChopGate;
    tempSystemTimer_.pin = pin_AOMpulseChopGate_;
    systemTimers_.push_back(tempSystemTimer_);
  }

  ////////// System Timers //////////
  if (pseudoPulsedSystem) {
    // Camera FSIN single continuous pulse for async + frame valid systems
    Octopus::Timer timer_cameraFSIN;
    memset(&timer_cameraFSIN, 0, sizeof(timer_cameraFSIN));
    timer_cameraFSIN.start_output_value = false;
    timer_cameraFSIN.end_output_value = false;
    timer_cameraFSIN.trigger.select = pin_voxelTrigger_;
    timer_cameraFSIN.trigger.sense = Octopus::Timer::Trigger::HIGH;
    timer_cameraFSIN.gate.select = Octopus::Pin::HIGH;
    timer_cameraFSIN.gate.sense = Octopus::Timer::Gate::HIGH;
    timer_cameraFSIN.state[1].output_value = false;
    timer_cameraFSIN.state[1].use_trigger = true;
    timer_cameraFSIN.state[1].next_state = 2;
    timer_cameraFSIN.state[1].period = chADelay_s; // Delay before FSIN pulse
    timer_cameraFSIN.state[2].output_value = true;
    timer_cameraFSIN.state[2].use_trigger = false;
    timer_cameraFSIN.state[2].next_state = 3;
    timer_cameraFSIN.state[2].period = TTLPulseWidth_s;
    timer_cameraFSIN.state[3].output_value = false;
    timer_cameraFSIN.state[3].use_trigger = false;
    timer_cameraFSIN.state[3].next_state = 1;
    timer_cameraFSIN.state[3].period = 1e-6;
    timer_cameraFSIN.state_transition_count = -1; // Infinite
    timer_cameraFSIN.out_select = pin_cameraFSIN_;
    octopus_->ConfigureTimer(timer_cameraFSIN);
    tempSystemTimer_.timer = timer_cameraFSIN;
    tempSystemTimer_.pin = pin_cameraFSIN_;
    systemTimers_.push_back(tempSystemTimer_);
  }

  // Camera Frame Valid (pulsed and aysnc pseudopulsed systems)]
  Octopus::Timer timer_cameraFrameValid;
  memset(&timer_cameraFrameValid, 0, sizeof(timer_cameraFrameValid));
  timer_cameraFrameValid.start_output_value = true;
  timer_cameraFrameValid.end_output_value = true;
  timer_cameraFrameValid.trigger.select = pin_voxelTrigger_;
  timer_cameraFrameValid.trigger.sense = Octopus::Timer::Trigger::HIGH; // WORK AROUND FOR RISING EDGE BUG
  timer_cameraFrameValid.gate.select = pin_voxelGate_;
  timer_cameraFrameValid.gate.sense = Octopus::Timer::Gate::HIGH;
  timer_cameraFrameValid.state[1].output_value = true;
  timer_cameraFrameValid.state[1].use_trigger = true;
  timer_cameraFrameValid.state[1].next_state = 2;
  timer_cameraFrameValid.state[1].period = chDDelay_s; // Frame valid delay
  timer_cameraFrameValid.state[2].output_value = false;
  timer_cameraFrameValid.state[2].use_trigger = false;
  timer_cameraFrameValid.state[2].next_state = 3;
  timer_cameraFrameValid.state[2].period = chDWidth_s;
  timer_cameraFrameValid.state[3].output_value = true;
  timer_cameraFrameValid.state[3].use_trigger = false;
  timer_cameraFrameValid.state[3].next_state = 1;
  timer_cameraFrameValid.state[3].period = 1e-6;
  timer_cameraFrameValid.state_transition_count = -1; // Infinite
  timer_cameraFrameValid.out_select = pin_cameraFrameValid_;
  octopus_->ConfigureTimer(timer_cameraFrameValid);
  tempSystemTimer_.timer = timer_cameraFrameValid;
  tempSystemTimer_.pin = pin_cameraFrameValid_;
  systemTimers_.push_back(tempSystemTimer_);

  // USTx [also split to Verasonics trigger]
  Octopus::Timer timer_USTx;
  memset(&timer_USTx, 0, sizeof(timer_USTx));
  timer_USTx.start_output_value = false;
  timer_USTx.end_output_value = false;
  timer_USTx.trigger.select = pin_voxelTrigger_;
  timer_USTx.trigger.sense = Octopus::Timer::Trigger::HIGH; // WORK AROUND FOR RISING EDGE BUG
  timer_USTx.gate.select = pin_voxelGate_;
  timer_USTx.gate.sense = Octopus::Timer::Gate::HIGH;
  timer_USTx.state[1].output_value = false;
  timer_USTx.state[1].use_trigger = true;
  timer_USTx.state[1].next_state = 2;
  timer_USTx.state[1].period = chBDelay_s; // Delay before trigger to USTx
  timer_USTx.state[2].output_value = true;
  timer_USTx.state[2].use_trigger = false;
  timer_USTx.state[2].next_state = 3;
  timer_USTx.state[2].period = TTLPulseWidth_s;
  timer_USTx.state[3].output_value = false;
  timer_USTx.state[3].use_trigger = false;
  timer_USTx.state[3].next_state = 1;
  timer_USTx.state[3].period = 1e-6;
  timer_USTx.state_transition_count = -1; // Infinite
  timer_USTx.out_select = pin_USTx_;
  octopus_->ConfigureTimer(timer_USTx);
  tempSystemTimer_.timer = timer_USTx;
  tempSystemTimer_.pin = pin_USTx_;
  systemTimers_.push_back(tempSystemTimer_);

  ////////// System Analog Outputs //////////
  // AOM 1 [upshift]
  Octopus::AOM AOMupshift;
  AOMupshift.channel = channel_AOMupshift_;
  AOMupshift.frequency = AOM1Freq_Hz;
  AOMupshift.amplitude = AOM1Volt_V; // TODO: measure gain on AOM amplifiers
  AOMupshift.gate = pin_AOMgate_; // Driven by timer_AOMgate_
  octopus_->ConfigureAOM(AOMupshift);

  // AOM 2 [downshift]
  Octopus::AOM AOMdownshift;
  AOMdownshift.channel = channel_AOMdownshift_;
  AOMdownshift.frequency = AOM2Freq_Hz;
  AOMdownshift.amplitude = AOM2Volt_V; // TODO: measure gain on AOM amplifiers
  AOMdownshift.gate = pin_AOMgate_; // Driven by timer_AOMgate_
  octopus_->ConfigureAOM(AOMdownshift);

  // AOM 3 [FF-ultrasound]
  if (ultrasoundAmp.compare("USTx") != 0 && ultrasoundAmp.compare("Verasonics") != 0) {
    Octopus::AOM AOMultrasound;
    AOMultrasound.channel = channel_AOMultrasound_;
    AOMultrasound.frequency = ultrasoundFreq_MHz;
    AOMultrasound.amplitude = ultrasoundVoltage_Vp;
    AOMultrasound.gate = pin_AOMgate_; // Driven by timer_AOMgate_
    octopus_->ConfigureAOM(AOMultrasound);
  }

  // AOM 4 [Pulse Chopper]
  Octopus::AOM AOMpulseChop;
  AOMpulseChop.channel = channel_AOMpulseChop_;
  AOMpulseChop.frequency = AOM4Freq_Hz;
  AOMpulseChop.amplitude = AOM4Volt_V; // TODO: measure gain on AOM amplifiers
  AOMpulseChop.gate = pin_AOMpulseChopGate_; // Driven by timer_AOMpulseChopGate_
  if (pseudoPulsedSystem) {
    octopus_->ConfigureAOM(AOMpulseChop);
  }
  return true;
}

bool OctopusManager::DisableOutput(Octopus::Timer timer, Octopus::Pin pin) {
  // EnableTimers(pin, false) does not return outputs to LOW
  // For safety, output of all channels should be set to low at the end of every scan
  timer.start_output_value = false;
  timer.end_output_value = false;
  timer.trigger.select = Octopus::Pin::HIGH;
  timer.trigger.sense = Octopus::Timer::Trigger::HIGH; // WORK AROUND FOR RISING EDGE BUG
  timer.gate.select = pin_voxelGate_;
  timer.gate.sense = Octopus::Timer::Gate::HIGH;
  timer.state[1].output_value = false;
  timer.state[1].use_trigger = false;
  timer.state[1].next_state = 2;
  timer.state[1].period = 1e-6;
  timer.state[2].output_value = false;
  timer.state[2].use_trigger = false;
  timer.state[2].next_state = 0;
  timer.state[2].period = 1e-6;
  timer.state_transition_count = 3;
  timer.out_select = pin;
  octopus_->ConfigureTimer(timer);
  octopus_->EnableTimer(pin, false);
  return true;
}
