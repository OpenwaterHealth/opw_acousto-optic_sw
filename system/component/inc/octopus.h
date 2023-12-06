#pragma once

#include <cstdint>

#include "system/component/inc/fx3.h"

class Octopus {
 public:
  Octopus() {}
  Octopus(FX3* fx3_device) : fx3_(fx3_device) {}
  ~Octopus() { fx3_->Close(); }

  // Number of octopi connected to the computer
  static int NumOctopi();

  // Get the ID of the octopus
  int SerialNumber();

  // Open the octopus by arbitrary number (0 -> NumOctopi() - 1)
  // If empty or less than zero, open any octopus connected
  // @returns 0 on success
  int Open(int id = 0);

  // Close connection to the octopus
  void Close();

  // Octopus Pins
  enum Pin {
    IN1_BOTTOM = 0,
    IN2_BOTTOM = 1,
    IN3_BOTTOM = 2,
    IN4_BOTTOM = 3,
    IN1_TOP = 4,
    IN2_TOP = 5,
    IN3_TOP = 6,
    IN4_TOP = 7,
    OUT1_BOTTOM = 8,
    OUT2_BOTTOM = 9,
    OUT3_BOTTOM = 10,
    OUT4_BOTTOM = 11,
    OUT5_BOTTOM = 12,
    OUT6_BOTTOM = 13,
    OUT7_BOTTOM = 14,
    OUT8_BOTTOM = 15,
    OUT1_TOP = 16,
    OUT2_TOP = 17,
    OUT3_TOP = 18,
    OUT4_TOP = 19,
    OUT5_TOP = 20,
    OUT6_TOP = 21,
    OUT7_TOP = 22,
    OUT8_TOP = 23,
    LOW = 30,
    HIGH = 31
  };

  // See
  // https://drive.google.com/a/openwater.cc/file/d/1DB6KkDIDf8nf-vmhpdV4v0LWQi2UIXaE/view?usp=sharing
  struct Timer {
    struct Trigger {
      Pin select;
      enum { LOW = 0b00, RISING = 0b01, FALLING = 0b10, HIGH = 0b11 } sense;
    } trigger;

    struct Gate {
      Pin select;
      enum { LOW = 0, HIGH = 1 } sense;
    } gate;

    bool start_output_value;
    bool end_output_value;

    struct {
      bool output_value;
      bool use_trigger;
      double period;
      int next_state;
    } state[7];

    uint32_t state_transition_count;

    Pin out_select;
  };

  // Setup an Octopus Timer
  // @param timer which timer to configure
  // @param ot timer configuration parameters
  void ConfigureTimer(const Timer& ot);

  // Enable / disable a timer
  // @param pin pin to enable/disable
  // @param enable whether the timer is enabled
  void EnableTimer(Pin pin, bool enable);

  struct AOM {
    int channel;       // which channel (1-4)
    double frequency;  // Frequency in Hz
    double amplitude;  // Amplitude in Volts (Vpp will be 2x amplitude)
    Pin gate;          // Which pin gates the AOM output
  };

  // Setup an Octopus AOM
  // @param oa AOM parameters
  void ConfigureAOM(AOM& oa);

 private:
  // Raw write register to Octopus
  // @param device device within the octo
  // @param reg register within the device
  // @param data data to write
  void WriteReg(uint8_t device, uint8_t reg, uint16_t data);

  // Raw read register from Octopus
  // @param device decvice within the octo
  // @param reg register within the deice
  uint16_t ReadReg(uint8_t device, uint8_t reg);

  // modifiy bits specified
  // @param device device within the octo
  // @param reg register within the device
  // @param val value to write
  // @param bit_ddr lowest bit of the bits to write
  // @param bit_len number of bits to modify
  void ModReg(uint8_t device, uint8_t reg, uint16_t val, uint8_t bit_addr,
              uint8_t bit_len);

  // Write to the DDS (sine wave generator)
  // @param reg register to write
  // @param data data to write
  // @param len number of bytes to write
  void WriteDDS(uint8_t reg, uint32_t data, int len = 4);

  // Read from the DDS
  // @param reg register to read
  // @param len number of bytes to read
  uint32_t ReadDDS(uint8_t reg, int len);

  // Send and update signal to the DDS
  // See https://www.analog.com/media/en/technical-documentation/data-sheets/AD9959.pdf
  void UpdateDDS();

  FX3 fx3_inst_;
  FX3* fx3_ = &fx3_inst_;

  static const int PID = 0x4F12;

  static const int DDS_ADDR = 0x1E;
  static const int TIMER_CONTROL = 0x1F;
};
