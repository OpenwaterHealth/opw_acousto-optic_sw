#include "system/component/inc/octopus.h"

#include <cassert>
#include <cstring>

#include "system/component/inc/octo_fw.h"
#include "system/component/inc/prettyPrintOctopusRegisters.h"

// Fullscale AOM output
static const double AOM_FULLSCALE = 8.00;

// AOM clock frequency
static const double AOM_CLK_FREQ = 400e6;

// Octopus clock frequency.  Clocked off the AOM's sync clock,
//   which is the AOM clock divided by 4
static const double OCTO_CLK_FREQ = AOM_CLK_FREQ / 4;

// Channel mapping between Aom::channel and DDS channel
static const int CH_MAP[5] = {-1, 2, 3, 0, 1};

// Uncomment this line to enable pretty print (debug info for Octopus register
// writes)
//#define __PRETTYPRINT

#define CHECK_RET(x)                                       \
  {                                                        \
    int ret = x;                                           \
    if (ret != 0) {                                        \
      printf("Octopus error: %s returned %d\n", #x, ret);  \
      return ret;                                          \
    }                                                      \
  }

int Octopus::Open(int n) {
  CHECK_RET(fx3_->Open(PID, n));

  CHECK_RET(fx3_->Flash(__octo_fw_len, __octo_fw));

  // set up AOM PLL
  WriteDDS(1, 0xC00000, 3);
  return 0;
}

int Octopus::SerialNumber() { return fx3_->SerialNumber(); }

void Octopus::Close() { fx3_->Close(); }

void Octopus::WriteReg(uint8_t device, uint8_t reg, uint16_t data) {
  uint8_t bytes[4];
  bytes[0] = reg;
  bytes[1] = device | 0x80;
  memcpy(bytes + 2, &data, 2);
  int xfer = fx3_->DataOut(4, bytes);
  assert(xfer == 4);

#ifdef __PRETTYPRINT
  prettyPrintOctopusReg(NULL, bytes[0] | (bytes[1] << 8), data);
#endif
}

uint16_t Octopus::ReadReg(uint8_t device, uint8_t reg) {
  uint16_t ret;
  ret = device << 8 | reg;
  int xfer = fx3_->DataOut(2, (uint8_t*)&ret);
  assert(xfer == 2);
  xfer = fx3_->DataIn(2, (uint8_t*)&ret);
  assert(xfer == 2);
  return ret;
}

void Octopus::ModReg(uint8_t device, uint8_t reg, uint16_t val,
                     uint8_t bit_addr, uint8_t bit_len) {
  uint16_t r = ReadReg(device, reg);
  r &= ~(((1 << bit_len) - 1) << bit_addr);
  r |= val << bit_addr;
  WriteReg(device, reg, r);
}

// Register map for Octo SPI module
// Bus address 0x1E
// Register: LSBYTE | MSBYTE
// 00: ADDR | LEN
// 01: BYTE4 | BYTE3
// 02: BYTE2 | BYTE1
// 03: RBYTE4 | RBYTE3
// 04: RBYTE2 | RBYTE1
// 06: 00F0; // turn on
// 07: Write 1 to start transaction, 2 to set the update bit

uint32_t Octopus::ReadDDS(uint8_t reg, int len) {
  WriteReg(DDS_ADDR, 0, (len << 8) | (reg | 0x80));  // issue read command
  WriteReg(DDS_ADDR, 7, 1);
  switch (len) {
    case 1: {
      return ReadReg(DDS_ADDR, 4) >> 8;
    } break;
    case 2: {
      return ReadReg(DDS_ADDR, 4);
    } break;
    case 3: {
      return (uint32_t)ReadReg(DDS_ADDR, 4) << 8 | (ReadReg(DDS_ADDR, 3) >> 8);
    } break;
    case 4: {
      return (uint32_t)ReadReg(DDS_ADDR, 4) << 16 | ReadReg(DDS_ADDR, 3);
    } break;
    default: {
      assert(0);
      return 0;
    } break;
  }
}

void Octopus::WriteDDS(uint8_t reg, uint32_t data, int len) {
  WriteReg(DDS_ADDR, 0, (len << 8) | reg);
  switch (len) {
    case 1: {
      WriteReg(DDS_ADDR, 2, data << 8);
    } break;
    case 2: {
      WriteReg(DDS_ADDR, 2, data);
    } break;
    case 3: {
      WriteReg(DDS_ADDR, 2, data >> 8);
      WriteReg(DDS_ADDR, 1, (data & 0xFF) << 8);
    } break;
    case 4: {
      WriteReg(DDS_ADDR, 1, data & 0xFFFF);
      WriteReg(DDS_ADDR, 2, data >> 16);
    } break;
    default: {
      assert(0);
    } break;
  }

  WriteReg(DDS_ADDR, 7, 1);
}

void Octopus::UpdateDDS() { WriteReg(DDS_ADDR, 7, 2); }

void Octopus::ConfigureTimer(const Timer& ot) {
  uint16_t regmap[17] = {0};
  uint32_t count;

  for (int i = 1; i < 7; ++i) {
    assert(ot.state[i].period < (((uint64_t)1 << 32) - 1) / OCTO_CLK_FREQ);  // TODO(CR) asserts don't work in Release
    assert(ot.state[i].next_state >= 0);
    assert(ot.state[i].next_state <= 6);
  }

  // disable timer to allow config
  ModReg(TIMER_CONTROL, 0, 0, ot.out_select - 8, 1);

  // register map from timer_top.v
  regmap[0] = (ot.trigger.select << 0) | (ot.trigger.sense << 5) |
              (ot.gate.select << 7) | (ot.gate.sense << 12) |
              (ot.start_output_value << 13) | (ot.end_output_value << 14);

  regmap[1] = (ot.state[1].output_value << 0) | (ot.state[1].use_trigger << 1) |
              (ot.state[1].next_state << 2) |
              (ot.state[2].output_value << 5) | (ot.state[2].use_trigger << 6) |
              (ot.state[2].next_state << 7) |
              (ot.state[3].output_value << 10) | (ot.state[3].use_trigger << 11) |
              (ot.state[3].next_state << 12);

  count = ot.state[1].period * OCTO_CLK_FREQ;
  regmap[2] = count & 0xFFFF;
  regmap[3] = count >> 16;
  count = ot.state[2].period * OCTO_CLK_FREQ;
  regmap[4] = count & 0xFFFF;
  regmap[5] = count >> 16;
  count = ot.state[3].period * OCTO_CLK_FREQ;
  regmap[6] = count & 0xFFFF;
  regmap[7] = count >> 16;

  regmap[8] = (ot.state[4].output_value << 0) | (ot.state[4].use_trigger << 1) |
              (ot.state[4].next_state << 2) |
              (ot.state[5].output_value << 5) |
              (ot.state[5].use_trigger << 6) | (ot.state[5].next_state << 7) |
              (ot.state[6].output_value << 10) |
              (ot.state[6].use_trigger << 11) | (ot.state[6].next_state << 12);

  count = ot.state[4].period * OCTO_CLK_FREQ;
  regmap[9] = count & 0xFFFF;
  regmap[10] = count >> 16;
  count = ot.state[5].period * OCTO_CLK_FREQ;
  regmap[11] = count & 0xFFFF;
  regmap[12] = count >> 16;
  count = ot.state[6].period * OCTO_CLK_FREQ;
  regmap[13] = count & 0xFFFF;
  regmap[14] = count >> 16;

  regmap[15] = ot.state_transition_count & 0xFFFF;
  regmap[16] = ot.state_transition_count >> 16;

  // write to the timer device
  uint8_t module_address = (ot.out_select - 8);
  for (int i = 0; i < 17; ++i) {
    WriteReg(module_address, i, regmap[i]);
  }
}

void Octopus::EnableTimer(Pin pin, bool enable) {
  if (pin < OUT1_BOTTOM || pin > OUT8_TOP) return;
  ModReg(TIMER_CONTROL, 0, 1, pin - 8, 1);
}

void Octopus::ConfigureAOM(AOM& oa) {
  assert(oa.channel >= 1);
  assert(oa.channel <= 4);
  assert(oa.frequency > 0);
  assert(oa.frequency < 125e6);
  assert(oa.amplitude >= 0);
  // higher voltages cause harmonic distortion on v1 Octo PCBs
  assert(oa.amplitude <= 7.25);

  // select the channel
  int ch = CH_MAP[oa.channel];
  WriteDDS(0, (1 << ch) << 4, 1);

  // 2-level modulation, full scale DAC, sine wave output
  WriteDDS(3, 0x400301, 3);

  // desired frequency
  WriteDDS(4, (uint32_t)(oa.frequency / AOM_CLK_FREQ * ((int64_t)1 << 32)), 4);

  // amplitude is 0 when off
  WriteDDS(6, 0, 3);

  // amplitude when on
  WriteDDS(0x0A, (uint32_t)(oa.amplitude / AOM_FULLSCALE * 0x3FF) << 22, 4);

  // Set the Px pin to the specified gate
  switch (ch) {
    case 0: {
      ModReg(DDS_ADDR, 6, oa.gate, 0, 5);
    } break;
    case 1: {
      ModReg(DDS_ADDR, 5, oa.gate, 10, 5);
    } break;
    case 2: {
      ModReg(DDS_ADDR, 5, oa.gate, 5, 5);
    } break;
    case 3: {
      ModReg(DDS_ADDR, 5, oa.gate, 0, 5);
    } break;
  }

  // enable the output amplifier
  ModReg(DDS_ADDR, 6, 1, oa.channel + 11, 1);

  // set the update bit to make the above changes take effect
  WriteReg(DDS_ADDR, 7, 2);
}
