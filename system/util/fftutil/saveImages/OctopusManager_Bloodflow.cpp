#include <iostream>

#include <glog/logging.h>

#include "OctopusManager_Bloodflow.h"

using json = nlohmann::json;

OctopusManager_Bloodflow::~OctopusManager_Bloodflow() {
  delete octopus_;
}

bool OctopusManager_Bloodflow::init(const json& systemParameters) {
  octopus_ = new Octopus();
  int octopusOpened = octopus_->Open();
  if (octopusOpened == 0) {
    LOG(INFO) << "Connected to Octopus: " << octopus_->SerialNumber();
    return InitializeOctopus(systemParameters);
  } else {
    LOG(ERROR) << "Unable to open Octopus";
    return false;
  }
}

void OctopusManager_Bloodflow::TriggerDataCollection() {
  // Software trigger to initiate software gate to Frame Valid and AOM Chopper
  octopus_->ConfigureTimer(timer_SWtrigger_);
  octopus_->EnableTimer(pin_dataCollectionTrigger_, true);
}

bool OctopusManager_Bloodflow::EnableSystemChannels(bool channelON) {
  if (channelON) {
    for (const systemTimer& t : systemTimers_) {
      octopus_->EnableTimer(t.pin, channelON);
    }
    for (const systemTimer& t : timers_cameraFrameValid_) {
      octopus_->EnableTimer(t.pin, channelON);
    }
    for (const systemTimer& t : timers_cameraFSIN_) {
      octopus_->EnableTimer(t.pin, channelON);
    }
    octopus_->EnableTimer(swClock_.pin, channelON);
    octopus_->EnableTimer(swGate_.pin, channelON);
    octopus_->EnableTimer(wandLED_.pin, channelON);
  } else {
    for (const systemTimer& t : systemTimers_) {
      DisableOutput(t.timer, t.pin);
    }
    for (const systemTimer& t : timers_cameraFrameValid_) {
      DisableOutput(t.timer, t.pin);
    }
    for (const systemTimer& t : timers_cameraFSIN_) {
      DisableOutput(t.timer, t.pin);
    }
    DisableOutput(swClock_.timer, swClock_.pin);
    DisableOutput(swGate_.timer, swGate_.pin);
    DisableOutput(wandLED_.timer, wandLED_.pin);
    DisableOutput(timer_SWtrigger_, pin_dataCollectionTrigger_);
  }
  return true;
}

bool OctopusManager_Bloodflow::ConfigureSystemChannels() {
  for (systemTimer& t : systemTimers_) {
    octopus_->ConfigureTimer(t.timer);
  }
  for (systemTimer& t : timers_cameraFrameValid_) {
    octopus_->ConfigureTimer(t.timer);
  }
  for (systemTimer& t : timers_cameraFSIN_) {
    octopus_->ConfigureTimer(t.timer);
  }
  octopus_->ConfigureTimer(swClock_.timer);
  octopus_->ConfigureTimer(swGate_.timer);
  octopus_->ConfigureTimer(wandLED_.timer);
  return true;
}

bool OctopusManager_Bloodflow::SetDataCollectionTriggerPin(Octopus::Pin triggerPin) {
  // Sets pin used to initiate data collection. This may be software or hardware
  // Future pin setting may be needed to replace camera fsin with pulsed tapered amplifier sync
  pin_dataCollectionTrigger_ = triggerPin;
  return true;
}

bool OctopusManager_Bloodflow::SetAOMPulsewidth(int numImages, double delay1, double delay2, double width, double freq_Hz, double volt_V, AOMChopper aomChopper, bool hwTriggered) {
  Octopus::Pin pin;
  int channel;
  if (aomChopper == AOMChopper::FIRST_LASER) {
    pin = pin_AOMpulseChopGate_;
    channel = 4;
  } else if (aomChopper == AOMChopper::SECOND_LASER) {
    pin = pin_AOMpulseChopGate2_;
    channel = 3;
  }

  Octopus::Timer timer_AOMpulseChopGate; // SW gate for pulse chopping AOM time
  memset(&timer_AOMpulseChopGate, 0, sizeof(timer_AOMpulseChopGate));
  timer_AOMpulseChopGate.start_output_value = false;
  timer_AOMpulseChopGate.end_output_value = false;
  timer_AOMpulseChopGate.trigger.select = swClock_.pin;
  timer_AOMpulseChopGate.trigger.sense = Octopus::Timer::Trigger::HIGH;  // ToDo(CR): if rising edge bug fixed, change back to RISING
  if (hwTriggered) {
    timer_AOMpulseChopGate.gate.select = pin_dataCollectionTrigger_;
  } else {
    timer_AOMpulseChopGate.gate.select = swGate_.pin;
  }
  timer_AOMpulseChopGate.gate.sense = Octopus::Timer::Gate::HIGH;
  timer_AOMpulseChopGate.state[1].output_value = false;
  timer_AOMpulseChopGate.state[1].use_trigger = true;
  timer_AOMpulseChopGate.state[1].next_state = 2;
  timer_AOMpulseChopGate.state[1].period = delay1; // Delay before trigger to pulse chopping AOM
  timer_AOMpulseChopGate.state[2].output_value = true;
  timer_AOMpulseChopGate.state[2].use_trigger = false;
  timer_AOMpulseChopGate.state[2].next_state = 3;
  timer_AOMpulseChopGate.state[2].period = width; // On time of pulse chopping AOM
  timer_AOMpulseChopGate.state[3].output_value = false;
  timer_AOMpulseChopGate.state[3].use_trigger = false;
  timer_AOMpulseChopGate.state[3].next_state = 4;
  timer_AOMpulseChopGate.state[3].period = delay2;
  timer_AOMpulseChopGate.state[4].output_value = false;
  timer_AOMpulseChopGate.state[4].use_trigger = false;
  timer_AOMpulseChopGate.state[4].next_state = 1;
  timer_AOMpulseChopGate.state[4].period = 1e-6;
  timer_AOMpulseChopGate.state_transition_count = 4 * (numImages + 1) + 1;  // (num states * (num bright/dark pairs + 1 for FSIN init) + 1 for state 0
  timer_AOMpulseChopGate.out_select = pin;
  octopus_->ConfigureTimer(timer_AOMpulseChopGate);
  tempSystemTimer_.timer = timer_AOMpulseChopGate;
  tempSystemTimer_.pin = pin;
  systemTimers_.push_back(tempSystemTimer_);
  octopus_->EnableTimer(pin, true);

  ////////// System Analog Outputs //////////
  // AOM 3 or 4 [Pulse Chopper]
  Octopus::AOM AOMpulseChop;
  AOMpulseChop.channel = channel;
  AOMpulseChop.gate = pin;  // Driven by timer_AOMpulseChopGate_
  AOMpulseChop.frequency = freq_Hz;
  AOMpulseChop.amplitude = volt_V;
  octopus_->ConfigureAOM(AOMpulseChop);

  return true;
}

bool OctopusManager_Bloodflow::InitializeOctopus(const json& systemParameters) {
  ////////// System Dependent Delay Variables //////////
  double TTLPulseWidth_s = 1e-6;

  // Camera Parameters
  int numImages = systemParameters["cameraParameters"]["numImages"].get<int>();
  double frameAcquisitionRate_Hz = systemParameters["cameraParameters"]["frameAcquisitionRate_Hz"].get<double>();
  double triggerOffset_s = std::stod(systemParameters["delayParameters"]["triggerOffset_s"].get<std::string>());
  double dataCollectionGatePeriod_s = double(2.0 * (numImages + 1)) / (frameAcquisitionRate_Hz);  // Double numImages because of alternating dark image collection, +1 for initial FSIN pulses
  int numCameras = systemParameters["cameraParameters"]["numCameras"].get<int>();
  double frameValidDelay_s = systemParameters["delayParameters"]["frameValidDelay_s"].get<double>();
  double frameValidWidth_s = systemParameters["delayParameters"]["frameValidWidth_s"].get<double>();
  int hwTriggeredSystem = systemParameters["hardwareParameters"]["hwTrigger"].get<int>();  // Is system triggered by TTL button or sw trig
  int dualLaserSystem = systemParameters["hardwareParameters"]["dualLaserSystem"].get<int>();

  if (dualLaserSystem) {
    dataCollectionGatePeriod_s = double(3.0 * (numImages + 2)) / (frameAcquisitionRate_Hz);  // Triple numImages because of alternating dark image collection, +2 for initial FSIN pulses (and odd/even)
  }

  if (hwTriggeredSystem) {
    // Hardware triggered system gets input from push button switch
    // Note: SW gate could also be a hardware gate if we want to run for 'arbitrary' time periods
    SetDataCollectionTriggerPin(Octopus::Pin::IN1_BOTTOM);
  } else {
    // If no hardware trigger, system uses timer_SWclock as master clock
    SetDataCollectionTriggerPin(Octopus::Pin::OUT8_BOTTOM);
  }

  ////////// System Input Triggers //////////
  // SW Trigger
  // Used initiate data collection
  // Re-enabled & re-configured after each pulse width
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
  timer_SWtrigger_.state[1].period = TTLPulseWidth_s;
  timer_SWtrigger_.state[2].output_value = false;
  timer_SWtrigger_.state[2].use_trigger = false;
  timer_SWtrigger_.state[2].next_state = 0;
  timer_SWtrigger_.state[2].period = TTLPulseWidth_s;
  timer_SWtrigger_.state_transition_count = 3;
  timer_SWtrigger_.out_select = pin_dataCollectionTrigger_;

  // SW Clock
  // Used as internal clock to ensure syncing of camera & AOMs
  // Note: This could be replaced in the future to sync to an external HW clock (ie: pulsed tapered amplifier)
  Octopus::Timer timer_SWclock;
  memset(&timer_SWclock, 0, sizeof(timer_SWclock));
  timer_SWclock.start_output_value = false;
  timer_SWclock.end_output_value = false;
  timer_SWclock.trigger.select = pin_dataCollectionTrigger_;
  timer_SWclock.trigger.sense = Octopus::Timer::Trigger::HIGH;
  timer_SWclock.gate.select = Octopus::Pin::HIGH;
  timer_SWclock.gate.sense = Octopus::Timer::Gate::HIGH;
  timer_SWclock.state[1].output_value = true;
  timer_SWclock.state[1].use_trigger = true;  // Trigger for initial start to data collection (state 0->1)
  timer_SWclock.state[1].next_state = 2;
  timer_SWclock.state[1].period = TTLPulseWidth_s;
  timer_SWclock.state[2].output_value = false;
  timer_SWclock.state[2].use_trigger = false;
  timer_SWclock.state[2].next_state = 3;
  timer_SWclock.state[2].period = (1.0 / frameAcquisitionRate_Hz) - TTLPulseWidth_s;  // voxel acquisition rate
  timer_SWclock.state[3].output_value = true;
  timer_SWclock.state[3].use_trigger = false;
  timer_SWclock.state[3].next_state = 2;  //  No trigger, fixed number of pulses (loop through states 2->3 for N cycles)
  timer_SWclock.state[3].period = TTLPulseWidth_s;
  timer_SWclock.state_transition_count = 2 * (2*(numImages + 1)) + 1;
  timer_SWclock.out_select = pin_SWclock_;
  swClock_.timer = timer_SWclock;
  swClock_.pin = pin_SWclock_;

  // SW Gate
  // Triggered by pin_dataCollectionTrigger_ (either hardware or software) and gates Frame Valid and AOM pulse chopper
  std::vector<double> swGatePeriods(5, 1e-6);  // Maximum of 5 states, max period for each state based on Octopus clock.
  std::vector<bool> swGateLevels(5, false);  // Can daisy chain states to get ~3.5min continuous acquisition
  int idx = 0;
  const double maxOctopusPeriod = 42.9;  // Max octopus on time 42.9497s (see Octopus.cpp ConfigureTimer)
  while (dataCollectionGatePeriod_s > maxOctopusPeriod) {
    swGatePeriods[idx] = maxOctopusPeriod;
    dataCollectionGatePeriod_s -= maxOctopusPeriod;
    swGateLevels[idx] = true;
    idx ++;
  }
  swGatePeriods[idx] = dataCollectionGatePeriod_s;
  swGateLevels[idx] = true;

  Octopus::Timer timer_SWgate;
  memset(&timer_SWgate, 0, sizeof(timer_SWgate));
  timer_SWgate.start_output_value = false;
  timer_SWgate.end_output_value = false;
  timer_SWgate.trigger.select = pin_dataCollectionTrigger_;
  timer_SWgate.gate.select = Octopus::Pin::HIGH;
  timer_SWgate.gate.sense = Octopus::Timer::Gate::HIGH;
  timer_SWgate.trigger.sense = Octopus::Timer::Trigger::HIGH;  // ToDo(CR): if rising edge bug fixed, change back to RISING
  timer_SWgate.state[1].output_value = swGateLevels[0];
  timer_SWgate.state[1].use_trigger = true;
  timer_SWgate.state[1].next_state = 2;
  timer_SWgate.state[1].period = swGatePeriods[0];
  timer_SWgate.state[2].output_value = swGateLevels[1];
  timer_SWgate.state[2].use_trigger = false;
  timer_SWgate.state[2].next_state = 3;
  timer_SWgate.state[2].period = swGatePeriods[1];
  timer_SWgate.state[3].output_value = swGateLevels[2];
  timer_SWgate.state[3].use_trigger = false;
  timer_SWgate.state[3].next_state = 4;
  timer_SWgate.state[3].period = swGatePeriods[2];
  timer_SWgate.state[4].output_value = swGateLevels[3];
  timer_SWgate.state[4].use_trigger = false;
  timer_SWgate.state[4].next_state = 5;
  timer_SWgate.state[4].period = swGatePeriods[3];
  timer_SWgate.state[5].output_value = swGateLevels[4];
  timer_SWgate.state[5].use_trigger = false;
  timer_SWgate.state[5].next_state = 6;
  timer_SWgate.state[5].period = swGatePeriods[4];
  timer_SWgate.state[6].output_value = false;
  timer_SWgate.state[6].use_trigger = false;
  timer_SWgate.state[6].next_state = 1;
  timer_SWgate.state[6].period = 1e-6;
  timer_SWgate.state_transition_count = 7;  // gate is only used once per data collection: 7 transitions (states 0->6->end)
  timer_SWgate.out_select = pin_SWgate_;
  swGate_.timer = timer_SWgate;
  swGate_.pin = pin_SWgate_;

  // Wand LED
  Octopus::Timer timer_wandLED;
  memset(&timer_wandLED, 0, sizeof(timer_wandLED));
  timer_wandLED.start_output_value = true;
  timer_wandLED.end_output_value = false;
  timer_wandLED.trigger.select = pin_dataCollectionTrigger_;
  timer_wandLED.gate.select = Octopus::Pin::HIGH;
  timer_wandLED.gate.sense = Octopus::Timer::Gate::HIGH;
  timer_wandLED.trigger.sense = Octopus::Timer::Trigger::HIGH;  // ToDo(CR): if rising edge bug fixed, change back to RISING
  timer_wandLED.state[1].output_value = swGateLevels[0];
  timer_wandLED.state[1].use_trigger = true;
  timer_wandLED.state[1].next_state = 2;
  timer_wandLED.state[1].period = swGatePeriods[0];
  timer_wandLED.state[2].output_value = swGateLevels[1];
  timer_wandLED.state[2].use_trigger = false;
  timer_wandLED.state[2].next_state = 3;
  timer_wandLED.state[2].period = swGatePeriods[1];
  timer_wandLED.state[3].output_value = swGateLevels[2];
  timer_wandLED.state[3].use_trigger = false;
  timer_wandLED.state[3].next_state = 4;
  timer_wandLED.state[3].period = swGatePeriods[2];
  timer_wandLED.state[4].output_value = swGateLevels[3];
  timer_wandLED.state[4].use_trigger = false;
  timer_wandLED.state[4].next_state = 5;
  timer_wandLED.state[4].period = swGatePeriods[3];
  timer_wandLED.state[5].output_value = swGateLevels[4];
  timer_wandLED.state[5].use_trigger = false;
  timer_wandLED.state[5].next_state = 6;
  timer_wandLED.state[5].period = swGatePeriods[4];
  timer_wandLED.state[6].output_value = false;
  timer_wandLED.state[6].use_trigger = false;
  timer_wandLED.state[6].next_state = 1;
  timer_wandLED.state[6].period = 1e-6;
  timer_wandLED.state_transition_count = 7;  // gate is only used once per data collection: 7 transitions (states 0->6->end)
  timer_wandLED.out_select = pin_wandLED_;
  wandLED_.pin = pin_wandLED_;
  wandLED_.timer = timer_wandLED;

  ////////// System Timers //////////
  // Camera Frame Valid
  for (int i = 0; i < numCameras; i++) {
    Octopus::Timer timer_cameraFrameValid;
    memset(&timer_cameraFrameValid, 0, sizeof(timer_cameraFrameValid));
    timer_cameraFrameValid.start_output_value = true;
    timer_cameraFrameValid.end_output_value = true;
    timer_cameraFrameValid.trigger.select = swClock_.pin;
    timer_cameraFrameValid.trigger.sense = Octopus::Timer::Trigger::HIGH;  // ToDo(CR): if rising edge bug fixed, change back to RISING
    timer_cameraFrameValid.gate.select = swGate_.pin;
    timer_cameraFrameValid.gate.sense = Octopus::Timer::Gate::HIGH;
    timer_cameraFrameValid.state[1].output_value = true;
    timer_cameraFrameValid.state[1].use_trigger = true;
    timer_cameraFrameValid.state[1].next_state = 2;
    timer_cameraFrameValid.state[1].period = frameValidDelay_s;
    timer_cameraFrameValid.state[2].output_value = false;
    timer_cameraFrameValid.state[2].use_trigger = false;
    timer_cameraFrameValid.state[2].next_state = 3;
    timer_cameraFrameValid.state[2].period = frameValidWidth_s;
    timer_cameraFrameValid.state[3].output_value = true;
    timer_cameraFrameValid.state[3].use_trigger = false;
    timer_cameraFrameValid.state[3].next_state = 1;
    timer_cameraFrameValid.state[3].period = TTLPulseWidth_s;
    timer_cameraFrameValid.state_transition_count = 3 * (2 * (numImages + 1)) + 1;
    timer_cameraFrameValid.out_select = pins_cameraFrameValid_[i];
    octopus_->ConfigureTimer(timer_cameraFrameValid);
    tempFrameValidTimer_.timer = timer_cameraFrameValid;
    tempFrameValidTimer_.pin = pins_cameraFrameValid_[i];
    timers_cameraFrameValid_.push_back(tempFrameValidTimer_);
  }

  // Camera FSIN
  for (int i = 0; i < numCameras; i++) {
    Octopus::Timer timer_cameraFSIN;
    memset(&timer_cameraFSIN, 0, sizeof(timer_cameraFSIN));
    timer_cameraFSIN.start_output_value = false;
    timer_cameraFSIN.end_output_value = false;
    timer_cameraFSIN.trigger.select = swClock_.pin;
    timer_cameraFSIN.trigger.sense = Octopus::Timer::Trigger::HIGH;
    timer_cameraFSIN.gate.select = Octopus::Pin::HIGH;
    timer_cameraFSIN.gate.sense = Octopus::Timer::Gate::HIGH;
    timer_cameraFSIN.state[1].output_value = false;
    timer_cameraFSIN.state[1].use_trigger = true;
    timer_cameraFSIN.state[1].next_state = 2;
    timer_cameraFSIN.state[1].period = triggerOffset_s;  // Delay before FSIN pulse
    timer_cameraFSIN.state[2].output_value = true;
    timer_cameraFSIN.state[2].use_trigger = false;
    timer_cameraFSIN.state[2].next_state = 3;
    timer_cameraFSIN.state[2].period = TTLPulseWidth_s;
    timer_cameraFSIN.state[3].output_value = false;
    timer_cameraFSIN.state[3].use_trigger = false;
    timer_cameraFSIN.state[3].next_state = 1;
    timer_cameraFSIN.state[3].period = TTLPulseWidth_s;
    timer_cameraFSIN.state_transition_count = 3 * (2 * (numImages + 1)) + 1;
    timer_cameraFSIN.out_select = pins_cameraFSIN_[i];
    octopus_->ConfigureTimer(timer_cameraFSIN);
    tempSystemTimer_.timer = timer_cameraFSIN;
    tempSystemTimer_.pin = pins_cameraFSIN_[i];
    systemTimers_.push_back(tempSystemTimer_);
  }
  return true;
}

bool OctopusManager_Bloodflow::DisableOutput(Octopus::Timer timer, Octopus::Pin pin) {
  // EnableTimers(pin, false) does not return outputs to LOW
  // For safety, output of all channels should be set to low at the end of every scan
  timer.start_output_value = false;
  timer.end_output_value = false;
  timer.trigger.select = Octopus::Pin::HIGH;
  timer.trigger.sense = Octopus::Timer::Trigger::HIGH;  // ToDo(CR): if rising edge bug fixed, change back to RISING
  timer.gate.select = Octopus::Pin::LOW;
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

bool OctopusManager_Bloodflow::EnableFrameValid(bool validHIGH) {
  for (const systemTimer& t : timers_cameraFrameValid_) {
    if (validHIGH) {
      octopus_->EnableTimer(t.pin, validHIGH);
    } else {
      DisableOutput(t.timer, t.pin);
    }
  }
  return true;
}
