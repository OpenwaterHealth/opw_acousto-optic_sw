#include <iostream>

#include "OctopusManager_Align.h"

OctopusManager_Align::~OctopusManager_Align() {
  delete octopus_;
}

bool OctopusManager_Align::init(bool pulsedSystem, bool pseudoPulsedSystem, bool bloodflowSystem,
                                int numCameras, AOM_Settings aomSettings, double USDelay_s, 
                                double overlap_s, double cameraFSINDelay_s, double AOMDelay_s) {
  octopus_ = new Octopus();
  int octopusOpened = octopus_->Open();
  return InitializeOctopus(pulsedSystem, pseudoPulsedSystem, bloodflowSystem,
    numCameras, aomSettings, USDelay_s, overlap_s, cameraFSINDelay_s, AOMDelay_s);
}

bool OctopusManager_Align::SetVoxelTriggerPin(Octopus::Pin triggerPin) {
  pin_voxelTrigger_ = triggerPin;
  return true;
}

bool OctopusManager_Align::EnableSystemChannels(bool channelON) {
  for (systemTimer& t : systemTimers_) {
    if (channelON) {
      octopus_->EnableTimer(t.pin, channelON);
    } else {
      DisableOutput(t.timer, t.pin);
    }
  }
  return true;
}

bool OctopusManager_Align::EnableAOM(AOM_ShiftDirection shiftDirection, bool channelON, double delay, double width) {
  Octopus::Timer timer;
  Octopus::Pin pin;
  if (shiftDirection == AOM_ShiftDirection::UPSHIFT) {
    timer = timer_AOMupshiftGate_;
    pin = pin_AOMupshiftGate_;
  } else if (shiftDirection == AOM_ShiftDirection::DOWNSHIFT) {
    timer = timer_AOMdownshiftGate_;
    pin = pin_AOMdownshiftGate_;
  } else if (shiftDirection == AOM_ShiftDirection::ULTRASOUND) {
    timer = timer_AOMultrasoundGate_;
    pin = pin_AOMultrasoundGate_;
  } else if (shiftDirection == AOM_ShiftDirection::CHOP) {
    timer = timer_AOMpulseChopGate_;
    pin = pin_AOMpulseChopGate_;
  }

  if (channelON) {
    timer.start_output_value = false;
    timer.end_output_value = false;
    timer.trigger.select = pin_voxelTrigger_;
    timer.trigger.sense = Octopus::Timer::Trigger::HIGH; // WORK AROUND FOR RISING EDGE BUG
    timer.gate.select = Octopus::Pin::HIGH;
    timer.gate.sense = Octopus::Timer::Gate::HIGH;
    timer.state[1].output_value = false;
    timer.state[1].use_trigger = true;
    timer.state[1].next_state = 2;
    timer.state[1].period = delay; // Delay before trigger to AOMs
    timer.state[2].output_value = true;
    timer.state[2].use_trigger = false;
    timer.state[2].next_state = 3;
    timer.state[2].period = width; // On time of AOMss
    timer.state[3].output_value = false;
    timer.state[3].use_trigger = false;
    timer.state[3].next_state = 1;
    timer.state[3].period = 1e-6;
    timer.state_transition_count = -1; // Infinite
    timer.out_select = pin;
    octopus_->ConfigureTimer(timer);
    octopus_->EnableTimer(pin, channelON);
  } else {
    DisableOutput(timer, pin);
  }
  return true;
}

bool OctopusManager_Align::SetAOM(AOM_ShiftDirection shiftDirection, double frequency_Hz, double voltage_Vp) {
  // Important usage note: AOM voltage cannot be resolved finer than ~7-8mV increments, see octopus.cpp for more details
  int channel;
  Octopus::Pin pin;
  Octopus::AOM AOM;
  if (shiftDirection == AOM_ShiftDirection::UPSHIFT) {
    AOM = AOMupshift_;
    pin = pin_AOMupshiftGate_;
    channel = channel_AOMupshift_;
  } else if (shiftDirection == AOM_ShiftDirection::DOWNSHIFT) {
    AOM = AOMdownshift_;
    pin = pin_AOMdownshiftGate_;
    channel = channel_AOMdownshift_;
  } else if (shiftDirection == AOM_ShiftDirection::ULTRASOUND) {
    AOM = AOMultrasound_;
    pin = pin_AOMultrasoundGate_;
    channel = channel_AOMultrasound_;
  } else if (shiftDirection == AOM_ShiftDirection::CHOP) {
    AOM = AOMpulseChop_;
    pin = pin_AOMpulseChopGate_;
    channel = channel_AOMpulseChop_;
  } else {
    std::cout << "WARNING: Incorrect AOM channel" << std::endl;
    return false;
  }
  AOM.channel = channel;
  AOM.frequency = frequency_Hz;
  AOM.amplitude = voltage_Vp;
  AOM.gate = pin;
  octopus_->ConfigureAOM(AOM);
  return true;
}

bool OctopusManager_Align::SetUltrasoundTriggerDelay(double delay_s, bool channelON) {
  // USTx [also used for Verasonics trigger in Curie]
  if (channelON) {
    memset(&timer_USTx_, 0, sizeof(timer_USTx_));
    timer_USTx_.start_output_value = false;
    timer_USTx_.end_output_value = false;
    timer_USTx_.trigger.select = pin_voxelTrigger_;
    timer_USTx_.trigger.sense = Octopus::Timer::Trigger::HIGH; // WORK AROUND FOR RISING EDGE BUG
    timer_USTx_.gate.select = Octopus::Pin::HIGH;
    timer_USTx_.gate.sense = Octopus::Timer::Gate::HIGH;
    timer_USTx_.state[1].output_value = false;
    timer_USTx_.state[1].use_trigger = true;
    timer_USTx_.state[1].next_state = 2;
    timer_USTx_.state[1].period = delay_s; // Delay before trigger to USTx
    timer_USTx_.state[2].output_value = true;
    timer_USTx_.state[2].use_trigger = false;
    timer_USTx_.state[2].next_state = 3;
    timer_USTx_.state[2].period = 100e-6;
    timer_USTx_.state[3].output_value = false;
    timer_USTx_.state[3].use_trigger = false;
    timer_USTx_.state[3].next_state = 1;
    timer_USTx_.state[3].period = 1e-6;
    timer_USTx_.state_transition_count = -1; // Infinite
    timer_USTx_.out_select = pin_USTx_;
    octopus_->ConfigureTimer(timer_USTx_);
    octopus_->EnableTimer(pin_USTx_, channelON);
    tempSystemTimer_.timer = timer_USTx_;
    tempSystemTimer_.pin = pin_USTx_;
    systemTimers_.push_back(tempSystemTimer_);
  } else {
    DisableOutput(timer_USTx_, pin_USTx_);
  }
  return true;
}

bool OctopusManager_Align::InitializeOctopus(bool pulsedSystem, bool pseudoPulsedSystem, bool bloodflowSystem,
                                            int numCameras, AOM_Settings aomSettings, double USDelay_s, double overlap_s, 
                                            double cameraFSINDelay_s, double AOMDelay_s) {
  octopus_ = new Octopus();
  int octopusOpened = octopus_->Open();
  if (octopusOpened != 0) {
    std::cout << "ERROR: Unable to open Octopus\n" << std::flush;
    return false;
  } else {
    std::cout << "INFO: Connected to Octopus: " << octopus_->SerialNumber() << std::endl;

    if (pulsedSystem) {
      // Pulsed system trigger from laser, SW gate
      SetVoxelTriggerPin(Octopus::Pin::IN1_BOTTOM);
    } else if (pseudoPulsedSystem) {
      // Pseudo-pulsed system uses internal SW trigger, currently at 10Hz
      SetVoxelTriggerPin(Octopus::Pin::OUT8_TOP);
    } else {
      // Synchronous system SW trigger, no gate
      std::cout << "WARNING: Octopus not implemented for CW systems" << std::endl;
      SetVoxelTriggerPin(Octopus::Pin::OUT8_TOP);
    }

    ////////// System Input Triggers //////////
    // SW Trigger
    if (pseudoPulsedSystem) {
      memset(&timer_SWtrigger_, 0, sizeof(timer_SWtrigger_));
      timer_SWtrigger_.start_output_value = false;
      timer_SWtrigger_.end_output_value = false;
      timer_SWtrigger_.trigger.select = Octopus::Pin::HIGH;
      timer_SWtrigger_.gate.select = Octopus::Pin::HIGH;
      timer_SWtrigger_.gate.sense = Octopus::Timer::Gate::HIGH;
      timer_SWtrigger_.trigger.sense = Octopus::Timer::Trigger::HIGH;
      timer_SWtrigger_.state[1].output_value = false;
      timer_SWtrigger_.state[1].use_trigger = false;
      timer_SWtrigger_.state[1].next_state = 2;
      timer_SWtrigger_.state[1].period = 0.0999;
      timer_SWtrigger_.state[2].output_value = true;
      timer_SWtrigger_.state[2].use_trigger = false;
      timer_SWtrigger_.state[2].next_state = 1;
      timer_SWtrigger_.state[2].period = 100e-6; // run at 10Hz for pseudo-pulsed
      timer_SWtrigger_.state_transition_count = -1;
      timer_SWtrigger_.out_select = pin_voxelTrigger_;
      octopus_->ConfigureTimer(timer_SWtrigger_);
      tempSystemTimer_.timer = timer_SWtrigger_;
      tempSystemTimer_.pin = pin_voxelTrigger_;
      systemTimers_.push_back(tempSystemTimer_);
    }

    memset(&timer_AOMupshiftGate_, 0, sizeof(timer_AOMupshiftGate_));
    timer_AOMupshiftGate_.start_output_value = false;
    timer_AOMupshiftGate_.end_output_value = false;
    timer_AOMupshiftGate_.trigger.select = pin_voxelTrigger_;
    timer_AOMupshiftGate_.trigger.sense = Octopus::Timer::Trigger::HIGH; // WORK AROUND FOR RISING EDGE BUG
    timer_AOMupshiftGate_.gate.select = Octopus::Pin::HIGH;
    timer_AOMupshiftGate_.gate.sense = Octopus::Timer::Gate::HIGH;
    timer_AOMupshiftGate_.state[1].output_value = false;
    timer_AOMupshiftGate_.state[1].use_trigger = true;
    timer_AOMupshiftGate_.state[1].next_state = 2;
    timer_AOMupshiftGate_.state[1].period = AOMDelay_s; // Delay before trigger to AOMs
    timer_AOMupshiftGate_.state[2].output_value = true;
    timer_AOMupshiftGate_.state[2].use_trigger = false;
    timer_AOMupshiftGate_.state[2].next_state = 3;
    timer_AOMupshiftGate_.state[2].period = overlap_s; // On time of AOMss
    timer_AOMupshiftGate_.state[3].output_value = false;
    timer_AOMupshiftGate_.state[3].use_trigger = false;
    timer_AOMupshiftGate_.state[3].next_state = 1;
    timer_AOMupshiftGate_.state[3].period = 1e-6;
    timer_AOMupshiftGate_.state_transition_count = -1; // Infinite
    timer_AOMupshiftGate_.out_select = pin_AOMupshiftGate_;
    octopus_->ConfigureTimer(timer_AOMupshiftGate_);
    tempSystemTimer_.timer = timer_AOMupshiftGate_;
    tempSystemTimer_.pin = pin_AOMupshiftGate_;
    systemTimers_.push_back(tempSystemTimer_);

    // AOM downshift gate
    memset(&timer_AOMdownshiftGate_, 0, sizeof(timer_AOMdownshiftGate_));
    timer_AOMdownshiftGate_.start_output_value = false;
    timer_AOMdownshiftGate_.end_output_value = false;
    timer_AOMdownshiftGate_.trigger.select = pin_voxelTrigger_;
    timer_AOMdownshiftGate_.trigger.sense = Octopus::Timer::Trigger::HIGH; // WORK AROUND FOR RISING EDGE BUG
    timer_AOMdownshiftGate_.gate.select = Octopus::Pin::HIGH;
    timer_AOMdownshiftGate_.gate.sense = Octopus::Timer::Gate::HIGH;
    timer_AOMdownshiftGate_.state[1].output_value = false;
    timer_AOMdownshiftGate_.state[1].use_trigger = true;
    timer_AOMdownshiftGate_.state[1].next_state = 2;
    timer_AOMdownshiftGate_.state[1].period = AOMDelay_s; // Delay before trigger to AOMs
    timer_AOMdownshiftGate_.state[2].output_value = true;
    timer_AOMdownshiftGate_.state[2].use_trigger = false;
    timer_AOMdownshiftGate_.state[2].next_state = 3;
    timer_AOMdownshiftGate_.state[2].period = overlap_s; // On time of AOMss
    timer_AOMdownshiftGate_.state[3].output_value = false;
    timer_AOMdownshiftGate_.state[3].use_trigger = false;
    timer_AOMdownshiftGate_.state[3].next_state = 1;
    timer_AOMdownshiftGate_.state[3].period = 1e-6;
    timer_AOMdownshiftGate_.state_transition_count = -1; // Infinite
    timer_AOMdownshiftGate_.out_select = pin_AOMdownshiftGate_;
    octopus_->ConfigureTimer(timer_AOMdownshiftGate_);
    tempSystemTimer_.timer = timer_AOMdownshiftGate_;
    tempSystemTimer_.pin = pin_AOMdownshiftGate_;
    systemTimers_.push_back(tempSystemTimer_);

    if (pseudoPulsedSystem) {
      memset(&timer_AOMpulseChopGate_, 0, sizeof(timer_AOMpulseChopGate_));
      timer_AOMpulseChopGate_.start_output_value = false;
      timer_AOMpulseChopGate_.end_output_value = false;
      timer_AOMpulseChopGate_.trigger.select = pin_voxelTrigger_;
      timer_AOMpulseChopGate_.trigger.sense = Octopus::Timer::Trigger::HIGH; // WORK AROUND FOR RISING EDGE BUG
      timer_AOMpulseChopGate_.gate.select = Octopus::Pin::HIGH;
      timer_AOMpulseChopGate_.gate.sense = Octopus::Timer::Gate::HIGH;
      timer_AOMpulseChopGate_.state[1].output_value = false;
      timer_AOMpulseChopGate_.state[1].use_trigger = true;
      timer_AOMpulseChopGate_.state[1].next_state = 2;
      timer_AOMpulseChopGate_.state[1].period = AOMDelay_s; // Delay before trigger to pulse chopping AOM
      timer_AOMpulseChopGate_.state[2].output_value = true;
      timer_AOMpulseChopGate_.state[2].use_trigger = false;
      timer_AOMpulseChopGate_.state[2].next_state = 3;
      timer_AOMpulseChopGate_.state[2].period = overlap_s; // On time of pulse chopping AOM
      timer_AOMpulseChopGate_.state[3].output_value = false;
      timer_AOMpulseChopGate_.state[3].use_trigger = false;
      timer_AOMpulseChopGate_.state[3].next_state = 1;
      timer_AOMpulseChopGate_.state[3].period = 1e-6;
      timer_AOMpulseChopGate_.state_transition_count = -1; // Infinite
      timer_AOMpulseChopGate_.out_select = pin_AOMpulseChopGate_;
      octopus_->ConfigureTimer(timer_AOMpulseChopGate_);
      tempSystemTimer_.timer = timer_AOMpulseChopGate_;
      tempSystemTimer_.pin = pin_AOMpulseChopGate_;
      systemTimers_.push_back(tempSystemTimer_);

      memset(&timer_AOMultrasoundGate_, 0, sizeof(timer_AOMultrasoundGate_));
      timer_AOMultrasoundGate_.start_output_value = false;
      timer_AOMultrasoundGate_.end_output_value = false;
      timer_AOMultrasoundGate_.trigger.select = pin_voxelTrigger_;
      timer_AOMultrasoundGate_.trigger.sense = Octopus::Timer::Trigger::HIGH; // WORK AROUND FOR RISING EDGE BUG
      timer_AOMultrasoundGate_.gate.select = Octopus::Pin::HIGH;
      timer_AOMultrasoundGate_.gate.sense = Octopus::Timer::Gate::HIGH;
      timer_AOMultrasoundGate_.state[1].output_value = false;
      timer_AOMultrasoundGate_.state[1].use_trigger = true;
      timer_AOMultrasoundGate_.state[1].next_state = 2;
      timer_AOMultrasoundGate_.state[1].period = AOMDelay_s; // Delay before trigger to pulse chopping AOM
      timer_AOMultrasoundGate_.state[2].output_value = true;
      timer_AOMultrasoundGate_.state[2].use_trigger = false;
      timer_AOMultrasoundGate_.state[2].next_state = 3;
      timer_AOMultrasoundGate_.state[2].period = overlap_s; // On time of pulse chopping AOM
      timer_AOMultrasoundGate_.state[3].output_value = false;
      timer_AOMultrasoundGate_.state[3].use_trigger = false;
      timer_AOMultrasoundGate_.state[3].next_state = 1;
      timer_AOMultrasoundGate_.state[3].period = 1e-6;
      timer_AOMultrasoundGate_.state_transition_count = -1; // Infinite
      timer_AOMultrasoundGate_.out_select = pin_AOMultrasoundGate_;
      octopus_->ConfigureTimer(timer_AOMultrasoundGate_);
      tempSystemTimer_.timer = timer_AOMultrasoundGate_;
      tempSystemTimer_.pin = pin_AOMultrasoundGate_;
      systemTimers_.push_back(tempSystemTimer_);
    }

    ////////// System Timers //////////
    // Camera FSIN
    for (int i = 0; i < numCameras; i++) {
      Octopus::Timer timer_cameraFSIN;
      memset(&timer_cameraFSIN, 0, sizeof(timer_cameraFSIN));
      timer_cameraFSIN.start_output_value = false;
      timer_cameraFSIN.end_output_value = false;
      timer_cameraFSIN.trigger.select = pin_voxelTrigger_;
      timer_cameraFSIN.trigger.sense = Octopus::Timer::Trigger::HIGH; // WORK AROUND FOR RISING EDGE BUG
      timer_cameraFSIN.gate.select = Octopus::Pin::HIGH;
      timer_cameraFSIN.gate.sense = Octopus::Timer::Gate::HIGH;
      timer_cameraFSIN.state[1].output_value = false;
      timer_cameraFSIN.state[1].use_trigger = true;
      timer_cameraFSIN.state[1].next_state = 2;
      timer_cameraFSIN.state[1].period = cameraFSINDelay_s; // Delay before FSIN pulse
      timer_cameraFSIN.state[2].output_value = true;
      timer_cameraFSIN.state[2].use_trigger = false;
      timer_cameraFSIN.state[2].next_state = 3;
      timer_cameraFSIN.state[2].period = 100e-6;
      timer_cameraFSIN.state[3].output_value = false;
      timer_cameraFSIN.state[3].use_trigger = false;
      timer_cameraFSIN.state[3].next_state = 1; // CR NOTE: Should eventually be set back to state 0, but that causes undefined failure (ends high or low and next trigger does not work)
      timer_cameraFSIN.state[3].period = 1e-6;
      timer_cameraFSIN.state_transition_count = -1; // Infinite
      timer_cameraFSIN.out_select = pins_cameraFSIN_[i];
      octopus_->ConfigureTimer(timer_cameraFSIN);
      tempSystemTimer_.timer = timer_cameraFSIN;
      tempSystemTimer_.pin = pins_cameraFSIN_[i];
      systemTimers_.push_back(tempSystemTimer_);
    }

    ////////// System Analog Outputs //////////
    // AOM 1 [upshift]
    AOMupshift_.channel = channel_AOMupshift_;
    AOMupshift_.frequency = aomSettings.AOM1Freq_Hz;
    AOMupshift_.amplitude = aomSettings.AOM1Volt_V;
    AOMupshift_.gate = pin_AOMupshiftGate_;

    // AOM 2 [downshift]
    AOMdownshift_.channel = channel_AOMdownshift_;
    AOMdownshift_.frequency = aomSettings.AOM2Freq_Hz;
    AOMdownshift_.amplitude = aomSettings.AOM2Volt_V;
    AOMdownshift_.gate = pin_AOMdownshiftGate_;

    // AOM 3 [Pulse Chopper [Gabor dual laser]]
    AOMultrasound_.channel = channel_AOMultrasound_;
    AOMultrasound_.frequency = aomSettings.AOM3Freq_Hz;
    AOMultrasound_.amplitude = aomSettings.AOM3Volt_V;
    AOMultrasound_.gate = pin_AOMultrasoundGate_; // Driven by timer_AOMultrasound_

    // AOM 4 [Pulse Chopper [Fessenden]]
    AOMpulseChop_.channel = channel_AOMpulseChop_;
    AOMpulseChop_.frequency = aomSettings.AOM4Freq_Hz;
    AOMpulseChop_.amplitude = aomSettings.AOM4Volt_V;
    AOMpulseChop_.gate = pin_AOMpulseChopGate_; // Driven by timer_AOMpulseChopGate_

    if (pseudoPulsedSystem) {
      if (bloodflowSystem) {
        if (aomSettings.AOM3Freq_Hz) {
          // Only configure channel 3 if it's been set [Gabor dual laser system]
          octopus_->ConfigureAOM(AOMultrasound_);
        }
        octopus_->ConfigureAOM(AOMpulseChop_);
      } else {
        octopus_->ConfigureAOM(AOMupshift_);
        octopus_->ConfigureAOM(AOMdownshift_);
      }
    } else {
      octopus_->ConfigureAOM(AOMupshift_);
      octopus_->ConfigureAOM(AOMdownshift_);
    }
    return true;
  }
}

bool OctopusManager_Align::DisableOutput(Octopus::Timer timer, Octopus::Pin pin) {
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
  timer.state[2].next_state = 0;
  timer.state[2].period = 1e-6;
  timer.state_transition_count = 3;
  timer.out_select = pin;
  octopus_->ConfigureTimer(timer);
  octopus_->EnableTimer(pin, false);
  return true;
}
