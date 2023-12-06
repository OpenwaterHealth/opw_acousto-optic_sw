#include <iostream>

#include "OctopusManager_USscanner.h"

using json = nlohmann::json;

OctopusManager_USscanner::~OctopusManager_USscanner() {
  DisableOutput(timer_SWtrigger_, pin_SWtrigger_);
  EnableUSTxTimer(false);
  delete octopus_;
}

bool OctopusManager_USscanner::init(const json& systemParameters) {
  octopus_ = new Octopus();
  int octopusOpened = octopus_->Open();
  if (octopusOpened == 0) {
    std::cout << "INFO: Connected to Octopus: " << octopus_->SerialNumber() << std::endl;
    return InitializeOctopus(systemParameters);
  }
  else {
    std::cout << "ERROR: Unable to open Octopus\n" << std::flush;
    return false;
  }
}

int OctopusManager_USscanner::Trigger() {
  octopus_->ConfigureTimer(timer_SWtrigger_);
  octopus_->EnableTimer(pin_SWtrigger_, true);
  return 1;
}

int OctopusManager_USscanner::EnableUSTxTimer(bool channelON) {
  if (channelON) {
    octopus_->ConfigureTimer(timer_USTx_);
    octopus_->EnableTimer(pin_USTx_, true);
  } else {
    DisableOutput(timer_USTx_, pin_USTx_);
  }
  return 1;
}

bool OctopusManager_USscanner::InitializeOctopus(const json& systemParameters) {
  ////////// System Dependent Delay Variables //////////
  double TTLPulseWidth_s = systemParameters["delayParameters"]["TTLPulseWidth_s"].get<double>();
  double ustxTriggerPeriod_s = systemParameters["ultrasoundParameters"]["ustxTriggerPeriod_s"].get<double>();
  int numFociPerSlice = systemParameters["ultrasoundParameters"]["numFociPerSlice"].get<int>();

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
  timer_SWtrigger_.state[2].next_state = 3;
  timer_SWtrigger_.state[2].period = 1e-6;
  timer_SWtrigger_.state[3].output_value = false;
  timer_SWtrigger_.state[3].use_trigger = false;
  timer_SWtrigger_.state[3].next_state = 2;
  timer_SWtrigger_.state[3].period = 1e-6;
  timer_SWtrigger_.state_transition_count = 3;
  timer_SWtrigger_.out_select = pin_SWtrigger_;

  // USTx [also split to Verasonics trigger]
  memset(&timer_USTx_, 0, sizeof(timer_USTx_));
  timer_USTx_.start_output_value = false;
  timer_USTx_.end_output_value = false;
  timer_USTx_.trigger.select = pin_SWtrigger_;
  timer_USTx_.trigger.sense = Octopus::Timer::Trigger::HIGH; // WORK AROUND FOR RISING EDGE BUG
  timer_USTx_.gate.select = Octopus::Pin::HIGH;
  timer_USTx_.gate.sense = Octopus::Timer::Gate::HIGH;
  timer_USTx_.state[1].output_value = false;
  timer_USTx_.state[1].use_trigger = true; // use software trigger to kick off N pulses at a given PRF
  timer_USTx_.state[1].next_state = 2;
  timer_USTx_.state[1].period = 1e-6; // Delay before trigger to USTx (arbitrary length)
  timer_USTx_.state[2].output_value = true;
  timer_USTx_.state[2].use_trigger = false;
  timer_USTx_.state[2].next_state = 3;
  timer_USTx_.state[2].period = TTLPulseWidth_s; // set in scanner.py (keep > 1us just to be safe)
  timer_USTx_.state[3].output_value = false;
  timer_USTx_.state[3].use_trigger = false;
  timer_USTx_.state[3].next_state = 2; // transition back to state 2 (only need state 1 to look for a "start" trigger out of state 0)
  timer_USTx_.state[3].period = ustxTriggerPeriod_s - TTLPulseWidth_s;
  timer_USTx_.state_transition_count =  2 * numFociPerSlice + 1 + 1; // 2 * number of pulses + 1 (state 0->1) + 1 (state 1->2)
  timer_USTx_.out_select = pin_USTx_;
  
  return true;
}

bool OctopusManager_USscanner::DisableOutput(Octopus::Timer timer, Octopus::Pin pin) {
  // EnableTimers(pin, false) does not return outputs to LOW
  // For safety, output of all channels should be set to low at the end of every scan
  timer.start_output_value = false;
  timer.end_output_value = false;
  timer.trigger.select = Octopus::Pin::HIGH;
  timer.trigger.sense = Octopus::Timer::Trigger::HIGH; // WORK AROUND FOR RISING EDGE BUG
  timer.gate.select = Octopus::Pin::HIGH;
  timer.gate.sense = Octopus::Timer::Gate::HIGH;
  timer.state[1].output_value = false;
  timer.state[1].use_trigger = false;
  timer.state[1].next_state = 2;
  timer.state[1].period = 1e-6;
  timer.state[2].output_value = false;
  timer.state[2].use_trigger = false;
  timer.state[2].next_state = 1;
  timer.state[2].period = 1e-6;
  timer.state_transition_count = 3;
  timer.out_select = pin;
  octopus_->ConfigureTimer(timer);
  octopus_->EnableTimer(pin, false);
  return true;
}
