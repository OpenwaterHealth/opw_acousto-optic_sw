#define _CRT_SECURE_NO_WARNINGS

#include "system/component/inc/octopus.h"

#include <conio.h>

#include <cstdio>
#include <cstring>
#include <fstream>

#include "system/component/inc/intelhex.h"
#include "system/component/inc/serial.h"
#include "system/third_party/json-develop/single_include/nlohmann/json.hpp"

using json = nlohmann::json;

int main() {
  Octopus octo;
  octo.Open();

  printf("Connected to octopus: %d\n", octo.SerialNumber());

  // Hardware test code.  Function generator / scope needed for verification
  // Tests commented out for now to prevent anything from running on these channels when this code is used to drive the AOMs continuously
  // Temporary (TODO): add actual continuous ON/OFF AOM code to fftutil or separate octo control exe
  //Octopus::Timer ot = {};
  //ot.start_output_value = false;
  //ot.end_output_value = false;
  //ot.trigger.select = Octopus::Pin::IN1_BOTTOM;
  //ot.trigger.sense = Octopus::Timer::Trigger::RISING;
  //ot.gate.select = Octopus::Pin::HIGH;
  //ot.state[1].output_value = true;
  //ot.state[1].use_trigger = true;
  //ot.state[1].next_state = 2;
  //ot.state[1].period = 10e-6;
  //ot.state[2].output_value = false;
  //ot.state[2].use_trigger = false;
  //ot.state[2].next_state = 3;
  //ot.state[2].period = 10e-6;
  //ot.state[3].output_value = true;
  //ot.state[3].use_trigger = false;
  //ot.state[3].next_state = 2;
  //ot.state[3].period = 10e-6;
  //ot.state_transition_count = 6;
  //ot.out_select = Octopus::Pin::OUT2_BOTTOM;
  //octo.ConfigureTimer(ot);
  //octo.EnableTimer(Octopus::Pin::OUT2_BOTTOM, true);

  //ot = {};
  //ot.start_output_value = false;
  //ot.end_output_value = false;
  //ot.trigger.select = Octopus::Pin::HIGH;
  //ot.gate.select = Octopus::Pin::HIGH;
  //ot.state[1].output_value = true;
  //ot.state[1].use_trigger = false;
  //ot.state[1].next_state = 2;
  //ot.state[1].period = 10e-6;
  //ot.state[2].output_value = false;
  //ot.state[2].use_trigger = false;
  //ot.state[2].next_state = 1;
  //ot.state[2].period = 10e-6;
  //ot.state_transition_count = 0xFFFFFFFF;
  //ot.out_select = Octopus::Pin::OUT1_BOTTOM;
  //octo.ConfigureTimer(ot);
  //octo.EnableTimer(Octopus::Pin::OUT1_BOTTOM, true);

  //ot = {};
  //ot.start_output_value = false;
  //ot.end_output_value = false;
  //ot.trigger.select = Octopus::Pin::OUT2_BOTTOM;
  //ot.gate.select = Octopus::Pin::HIGH;
  //ot.trigger.sense = Octopus::Timer::Trigger::RISING;
  //ot.state[1].output_value = false;
  //ot.state[1].use_trigger = true;
  //ot.state[1].next_state = 2;
  //ot.state[1].period = 1e-6;
  //ot.state[2].output_value = true;
  //ot.state[2].use_trigger = false;
  //ot.state[2].next_state = 3;
  //ot.state[2].period = 5e-6;
  //ot.state[3].output_value = false;
  //ot.state[3].use_trigger = false;
  //ot.state[3].next_state = 1;
  //ot.state[3].period = 0;
  //ot.state_transition_count = 0xFFFFFFFF;
  //ot.out_select = Octopus::Pin::OUT1_TOP;
  //octo.ConfigureTimer(ot);
  //octo.EnableTimer(Octopus::Pin::OUT1_TOP, true);


  // Configure octopus AOM channels
  // For each channel set frequency, voltage, gate
  Octopus::AOM oa;

  oa.channel = 1;
  oa.frequency = 95e6;
  oa.amplitude = 1;
  oa.gate = Octopus::Pin::LOW;  // gate LOW means channel will never output
  octo.ConfigureAOM(oa);

  oa.channel = 2;
  oa.frequency = 95e6;
  oa.amplitude = 3;
  oa.gate = Octopus::Pin::LOW;
  octo.ConfigureAOM(oa);

  oa.channel = 3;
  oa.frequency = 100e6;
  oa.amplitude = 0.24;
  oa.gate = Octopus::Pin::HIGH;  // gate HIGH means channel will be on continuously
  octo.ConfigureAOM(oa);

  oa.channel = 4;
  oa.frequency = 100e6;
  oa.amplitude = 0.24;
  oa.gate = Octopus::Pin::HIGH;
  octo.ConfigureAOM(oa);

  printf("Configured\n");

  system("pause");  // "Press any key to continue"

  oa.channel = 3;
  oa.frequency = 100e6;
  oa.amplitude = 0.24;
  oa.gate = Octopus::Pin::LOW; // Turn off channel when key pressed
  octo.ConfigureAOM(oa);

  oa.channel = 4;
  oa.frequency = 100e6;
  oa.amplitude = 0.24;
  oa.gate = Octopus::Pin::LOW;  // Turn off channel when key pressed
  octo.ConfigureAOM(oa);


  return 0;
}
