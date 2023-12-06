#include "system/component/inc/ustx.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <thread>

using json = nlohmann::json;

double USTx::ComputeDelay(double x, double z, int element) {
  double x_d;
  double z_d;
  if (probe_.at(0) == 'L' || probe_.at(0) == 'B') {
    // Standard linear array (typically 128 elements)
    x_d = x - (element - (double)(n_elements_ - 1) / 2) * pitch_;
    z_d = z;

    // Limit number of elements based on f# or max angle
    if (std::abs((x_d * 2) / z_d) > 1.0 / f_number_) return -1;

  } else if (probe_.at(0) == 'P') {
    // 64 element P4-2 array or 96 element P4-1 array
    // typically use all elements, but can use maxElements field to further limit side lobes
    x_d = x - (element - (double)(n_elements_ - 1) / 2) * pitch_;
    z_d = z;
    x_distances_[element] = abs(x_d);

  } else if (probe_.at(0) == 'C') {
    // 128 element curvilinear C5-2 array; limit number of elements based on maxElements field
    double scanAngle_rad = pitch_ * (double)(n_elements_ - 1) / (radius_);  // Angle subtended by curvilinear array
    double deltaAngle = scanAngle_rad / (double)(n_elements_ - 1);  // Angle subtended by individual element
    double elementAngle = (-1.0 * scanAngle_rad / 2.0) + deltaAngle * (double)element;  // Angluar location from edge of array with center = 0
    double elementXpos = radius_ * sin(elementAngle);
    double elementZpos = radius_ * cos(elementAngle) - radius_;
    x_d = x - elementXpos;
    z_d = z - elementZpos;
    x_distances_[element] = abs(x_d);

  } else {
    printf("Probe type not found: %s\n", probe_.c_str());
  }
  return sqrt(x_d * x_d + z_d * z_d) / speed_sound_;
}

template <typename T>
static inline void LoadIfPresent(const json& j, const char* s, T& v) {
  if (j.contains(s)) v = j[s].get<T>();
}

int USTx::LoadJson(const json& j) {
  LoadIfPresent<double>(j, "speed_sound", speed_sound_);
  LoadIfPresent<int>(j, "n_elements", n_elements_);
  LoadIfPresent<double>(j, "pitch", pitch_);
  LoadIfPresent<double>(j, "frequency", frequency_);
  LoadIfPresent<int>(j, "waveforms", waveforms_);
  LoadIfPresent<int>(j, "repeat_count", repeat_count_);
  LoadIfPresent<double>(j, "repeat_time", repeat_time_);
  LoadIfPresent<double>(j, "f_number", f_number_);
  LoadIfPresent<bool>(j, "cw_mode", cw_mode_);
  LoadIfPresent<std::string>(j, "probe", probe_);
  LoadIfPresent<double>(j, "radius", radius_);
  LoadIfPresent<int>(j, "max_elements", max_elements_);

  if (j.contains("foci")) {
    for (const json& focus : j["foci"]) {
      AddFocus(focus[0].get<double>(), focus[1].get<double>());
    }
  }

  return 0;
}

// Below this line is a lot of TX7332 register magic
// Please consult the TX7332 datasheet.
// https://drive.google.com/open?id=1AY-PbRCxCLR91AVlK_XrCCqThWXwBmy1

uint32_t SwapEndian(uint32_t val) {
  return (val >> 24) | ((val >> 8) & 0xFF00) | ((val << 8) & 0xFF0000) |
         ((val << 24));
}

// length address data structure
// USTx expects runs of (16-bit length, 16-bit address, 32-bit data)
//   in the flash structure.
// Since there are 4 chips, and each register is 32-bits, the length
//   is specified in 4x 32-bit chunks.
// This data is intended to be DMA-ed into the TX7332, so the order is
//   chip[0]data[0], chip[0]data[1], ... chip[0]data[n],
//   chip[1]data[0], chip[1]data[1], ... chip[1]data[n],
//                                   ... chip[3]data[n].
// Each word should be in the big-endian order the TI chip expects.
class USTx::LAD {
 public:
  uint16_t length_;
  uint16_t address_;
  std::vector<uint32_t> data_;

  LAD(uint16_t addr, const std::vector<uint32_t>& v, int count = 1) {
    length_ = (v.size() * count) >> 2;
    address_ = addr;
    for (int i = 0; i < count; ++i) {
      for (const uint32_t& d : v) {
        data_.push_back(SwapEndian(d));
      }
    }
  }

  int size() const { return 4 + (data_.size() << 2); }

  void Write(IntelHex& ih) const {
    ih.Write16(length_);
    ih.Write16(address_);
    ih.Write((uint8_t*)data_.data(), data_.size() * 4);
  }
};

// P4-1 element -> Verasonics pinmap
// ie: element position 1 is verasonics channel 23
static const uint8_t p4_1_elementmap[96] = {
    23,  22,  21,  41,  24,  42,  46,  43,  45,  44,  47,  18,  17,  48,  13,
    20,  19,  14,  15,  16,  49,  50,  54,  51,  53,  52,  9,   55,  56,  11,
    12,  5,   6,   7,   8,   10,  4,   3,   2,   1,   40,  39,  38,  37,  33,
    34,  35,  36,  93,  94,  95,  96,  92,  91,  90,  89,  128, 127, 126, 125,
    119, 121, 122, 123, 124, 117, 118, 73,  74,  120, 77,  76,  78,  75,  79,
    80,  113, 114, 115, 110, 109, 116, 81,  112, 111, 82,  85,  84,  86,  83,
    87,  105, 88,  108, 107, 106 };

// USTx TX7332 -> Verasonics pinmap
static const uint8_t verasonics_pinmap[128] = {
    35,  39,  34,  38,  46,  42,  47,  43,  51,  55,  50,  54,  62,  58,  63,
    59,  3,   7,   2,   6,   14,  10,  15,  11,  19,  23,  18,  22,  30,  26,
    31,  27,  28,  24,  29,  25,  17,  21,  16,  20,  12,  8,   13,  9,   1,
    5,   0,   4,   60,  56,  61,  57,  49,  53,  48,  52,  44,  40,  45,  41,
    33,  37,  32,  36,  91,  95,  90,  94,  86,  82,  87,  83,  75,  79,  74,
    78,  70,  66,  71,  67,  123, 127, 122, 126, 118, 114, 119, 115, 107, 111,
    106, 110, 102, 98,  103, 99,  100, 96,  101, 97,  105, 109, 104, 108, 116,
    112, 117, 113, 121, 125, 120, 124, 68,  64,  69,  65,  73,  77,  72,  76,
    84,  80,  85,  81,  89,  93,  88,  92};

// Tx7332 has a strange mapping between lanes and the register to be modified
static const int PROF_MAP[32][2] = {
    {0xF, 0},  {0xB, 0},  {0xF, 16}, {0xB, 16}, {0xE, 0},  {0xA, 0},  {0xE, 16},
    {0xA, 16}, {0xD, 0},  {0x9, 0},  {0xD, 16}, {0x9, 16}, {0xC, 0},  {0x8, 0},
    {0xC, 16}, {0x8, 16}, {0x7, 0},  {0x3, 0},  {0x7, 16}, {0x3, 16}, {0x6, 0},
    {0x2, 0},  {0x6, 16}, {0x2, 16}, {0x5, 0},  {0x1, 0},  {0x5, 16}, {0x1, 16},
    {0x4, 0},  {0x0, 0},  {0x4, 16}, {0x0, 16}};
static const int PDN_MAP[32] = {16, 24, 17, 25, 18, 26, 19, 27, 20, 28, 21,
                                29, 22, 30, 23, 31, 0,  8,  1,  9,  2,  10,
                                3,  11, 4,  12, 5,  13, 6,  14, 7,  15};

bool USTx::ComputeFoci(const char* hex_fname) {
  if (cw_mode_) {
    return ComputeCW(hex_fname);
  } else {
    return ComputePulsed(hex_fname);
  }
}

void USTx::WriteHeader(const std::vector<LAD>& init,
                       const std::vector<LAD>& focus, IntelHex& ih) {
  int init_len = 0;
  for (const LAD& record : init) {
    init_len += record.size();
  }
  int focus_len = 0;
  for (const LAD& record : focus) {
    focus_len += record.size();
  }

  // This must be kept in sync with how the USTx reads the intel hex file.
  ih.Write32(0x12345678);      // magic number
  ih.Write32(256);             // address of initialization registers
  ih.Write32(init_len);        // length of the initialization registers
  ih.Write32(256 + init_len);  // first focus address
  ih.Write32(focus_len);       // length of a focus
  ih.Write32(delays_.size());  // number of focii
  ih.Write32(cw_mode_);        // cw mode
  ih.SetAddress(256);

  for (const LAD& record : init) {
    record.Write(ih);
  }
}

bool USTx::ComputeCW(const char* hex_fname) {
  int clk_div = round(log2(CLK_FREQ / frequency_ / 16));
  if (clk_div < 0 || clk_div > 7) {
    printf("Unsupported clock frequency: %.1lf\n", frequency_);
    return false;
  }

  // actual divided clock used for delay calculations
  double div_clock = CLK_FREQ / pow(2, clk_div);

  std::vector<LAD> init;
  init.push_back(LAD(0x06, {0x00000010}, 4));
  init.push_back(LAD(0x0F, {0x00000010}, 4));
  // page 71 of TX7332 datasheet.  Assigns a constant 2us delay (for TR switch)
  //   and a CW_DAMP_COUNT of 3 to best approximate a sine wave
  init.push_back(LAD(0x18, {(uint32_t)0x008C0603 | (clk_div << 3)}, 4));
  // TR switches off
  init.push_back(LAD(0x1A, {0xFFFFFFFF}, 4));

  IntelHex ih;
  if (ih.Open(hex_fname) != 0) {
    printf("Could not open: %s\n", hex_fname);
    return false;
  }

  bool write_header = true;
  for (std::vector<double>& delay_profile : delays_) {
    // compute per-channel delays
    std::vector<uint32_t> profile(4 * 16, 0);
    std::vector<uint32_t> aperture(4, (uint32_t)0xFFFFFFFF);
    for (int i = 0; i < n_elements_; ++i) {
      int chip;
      int lane;
      if (probe_ == "P4-2" && i > 31) {
        // P4-2 probe has 64 adjacent elements, but they use Verasonics pins 1-32 and 97-128
        chip = verasonics_pinmap[i + 64] / 32;
        lane = verasonics_pinmap[i + 64] % 32;
      } else if (probe_ == "P4-1") {
        // P4-1 probe has 96 adjacent elements but they use the Verasonics pins in an order defined by p4_1_elementmap
        chip = verasonics_pinmap[p4_1_elementmap[i]] / 32;
        lane = verasonics_pinmap[p4_1_elementmap[i]] % 32;
      } else {
        chip = verasonics_pinmap[i] / 32;
        lane = verasonics_pinmap[i] % 32;
      }

      if (delay_profile[i] >= 0) {
        aperture[chip] &= ~(1 << PDN_MAP[lane]);
        uint32_t reg = ((int)round(-delay_profile[i] * div_clock) % 16) + 15;
        profile[chip * 16 + PROF_MAP[lane][0]] |= reg << PROF_MAP[lane][1];
      }
    }

    // make a focus with above registers
    std::vector<LAD> focus;
    focus.push_back(LAD(0x1B, aperture));
    focus.push_back(LAD(0x20, profile));

    if (write_header) {
      write_header = false;
      WriteHeader(init, focus, ih);
    }

    for (const LAD& record : focus) {
      record.Write(ih);
    }
  }

  if (ih.Close() != 0) {
    printf("Could not finish writing file %s\n", hex_fname);
    return false;
  }

  return true;
}

// This function translates between physical parameters (frequency, time)
//   and TX7332 registers.
bool USTx::ComputePulsed(const char* hex_fname) {
  // compute the pattern
  // a 3rd order harmonic elimination square wave has 6 parts
  // HHGLLG (H = high, G = ground, L = low)
  // Construct a waveform with these priorities:
  //   1. symmetric wave
  //   2. as close to the desired frequency as possible.
  //   3. as close to an ideal sine wave
  int period = floor(CLK_FREQ / frequency_);
  int high = period / 3 - 2;
  int gnd = period / 6 - 2;
  switch (period % 6) {
    case 2: {
      high += 1;
    } break;
    case 4: {
      gnd += 1;
    } break;
    case 5: {
      high += 1;
      gnd += 1;
    } break;
  }

  // if the requested period is too long, apply a clock division
  int clk_div = 0;
  while (high > 31) {
    ++clk_div;
    high = high >> 1;
    gnd = gnd >> 1;
    if (clk_div > 5) {
      printf("Requested frequency too low: %.1lfMHz\n", frequency_ / 1e6);
      return false;
    }
  }

  // checks on requested parameters
  if (gnd < 0) {
    printf("Requested frequency too high: %.1lfMHz\n", frequency_ / 1e6);
    return false;
  }
  if (waveforms_ > 4) {
    printf("Requested too many waveforms: %d\n", waveforms_);
    return false;
  }

  // Repeat time puts us in elastograpy mode, so we don't care about repeat_count
  //   or waveforms_
  if (repeat_time_ > 0) {
    waveforms_ = 1;
  }

  // construct the pattern registers
  std::vector<uint32_t> pattern;
  for (int cycle = 0; cycle < waveforms_; ++cycle) {
    uint32_t reg = ((high << 3) | 0b10) | (((gnd << 3) | 0b11) << 8) |
                   (((high << 3) | 0b01) << 16) | (((gnd << 3) | 0b11) << 24);
    pattern.push_back(reg);
  }
  // terminate the pattern if we didn't fill the 4 pattern registers
  if (pattern.size() < 4) {
    pattern.push_back(0b111);
  }

  // Initialization registers
  std::vector<LAD> init;
  init.push_back(LAD(0x06, {0x00000010}, 4));
  init.push_back(LAD(0x0F, {0x00000010}, 4));
  if (repeat_time_ > 0) {
    // repeat time in clock cycles (including x16 prescaler)
    repeat_count_ = (int)(repeat_time_ * CLK_FREQ / 16.0);
    if (repeat_count_ > (1 << 16) - 1) {
      printf("Repeat time greater than max allowable\n");
      return false;
    }
    // set LDO_MODE to high power
    init.push_back(LAD(0x0B, {0b1111 << 22}, 4));
    init.push_back(LAD(0x14, {0b1111 << 22}, 4));
    // put repeat count in the elastomeric repeat register
    // enable elastography mode
    init.push_back(
        LAD(0x19,
            {(31 << 6) | ((uint32_t)(repeat_count_) << 12) | ((uint32_t)1 << 11),
             0xFFFFFFFF}, 4));
  } else {
    if (repeat_count_ > 32) {
      printf("Repeat count greater than max 32\n");
      return false;
    }
    init.push_back(
        LAD(0x19, {(31 << 6) | ((uint32_t)(repeat_count_ - 1) << 1), 0xFFFFFFFF}, 4));
  }
  init.push_back(LAD(0x120, pattern, 4));

  IntelHex ih;
  if (ih.Open(hex_fname) != 0) {
    printf("Could not open: %s\n", hex_fname);
    return false;
  }

  bool write_header = true;
  for (std::vector<double>& delay_profile : delays_) {
    // determine the minimum delay for this focus
    double min_delay = 1e9;
    for (double& delay : delay_profile) {
      if (delay < 0) continue;  // should not proceed if delay is less than zero ie -1
      delay = TR_DEL + max_delay_ - delay;
      if (delay < min_delay) min_delay = delay;
    }

    // Load the minimum delay into the GLBL_DELAY regeister
    uint32_t glbl_delay = (min_delay * CLK_FREQ) / 8;
    if (glbl_delay > 0x1FF) glbl_delay = 0x1FF;
    double common_delay = (glbl_delay << 3) / CLK_FREQ;

    // compute per-channel delays
    std::vector<uint32_t> profile(4 * 16, 0);
    std::vector<uint32_t> aperture(4, (uint32_t)0xFFFFFFFF);
    for (int i = 0; i < n_elements_; ++i) {
      int chip;
      int lane;
      if (probe_ == "P4-2" && i > 31) {
        // P4-2 probe has 64 adjacent elements, but they use Verasonics pins 1-32 and 97-128
        chip = verasonics_pinmap[i + 64] / 32;
        lane = verasonics_pinmap[i + 64] % 32;
      } else if (probe_ == "P4-1") {
        // P4-1 probe has 96 adjacent elements but they use the Verasonics pins in an order defined by p4_1_elementmap
        chip = verasonics_pinmap[p4_1_elementmap[i]] / 32;
        lane = verasonics_pinmap[p4_1_elementmap[i]] % 32;
      } else {
        chip = verasonics_pinmap[i] / 32;
        lane = verasonics_pinmap[i] % 32;
      }

      if (delay_profile[i] >= 0) {
        aperture[chip] &= ~(1 << PDN_MAP[lane]);
        uint32_t reg = round((delay_profile[i] - common_delay) * CLK_FREQ);
        if (reg > 0x1FFF) {
          printf("Requsted delay too long: %d %e\n", i, delay_profile[i]);
          max_delay_ = -1;
          ih.Close();
          return false;
        }
        profile[chip * 16 + PROF_MAP[lane][0]] |= reg << PROF_MAP[lane][1];
      }
    }

    // make a focus with above registers
    std::vector<LAD> focus;
    focus.push_back(LAD(0x18, {(glbl_delay << 18) | (clk_div << 3) | 0x3}, 4));
    focus.push_back(LAD(0x1B, aperture));
    focus.push_back(LAD(0x20, profile));

    // write out the initalization data if we haven't already
    // this is done here in case the focus length changes
    if (write_header) {
      write_header = false;
      WriteHeader(init, focus, ih);
    }

    for (const LAD& record : focus) {
      record.Write(ih);
    }
  }

  if (ih.Close() != 0) {
    printf("Could not finish writing file %s\n", hex_fname);
    return false;
  }

  return true;
}

void USTx::AddFocus(double x, double z) {
  coordinates_.push_back(std::pair<double, double>(x, z));
  x_distances_.resize(n_elements_);

  std::vector<double> pattern(n_elements_);
  for (int i = 0; i < n_elements_; ++i) {
    pattern[i] = ComputeDelay(x, z, i);
    if (pattern[i] > max_delay_) max_delay_ = pattern[i];
  }

  // Limit number of elements used to create focus for curvilinear and phased arrays
  if (probe_.at(0) == 'C') { 
    int min_element_index = std::min_element(x_distances_.begin(), x_distances_.end()) - x_distances_.begin();
    int element_start = min_element_index - max_elements_ / 2;
    int element_end = min_element_index + max_elements_ / 2;
    if (element_start < 0) {
      element_start = 0;
      element_end = max_elements_ - 1;
    }
    if (element_end > n_elements_) {
      element_end = n_elements_;
      element_start = n_elements_ - max_elements_ - 1;
    }

    for (int i = 0; i < n_elements_; ++i) {
      if (i < element_start || i > element_end) {
        pattern[i] = -1; // turns off that element
      }
    }
  }
  else if (probe_.at(0) == 'B') {
    for (int i = 0; i < n_elements_; ++i) {
      if ((i == 0) || (i == 60) || (i == 67) || (i == 127)) {
       // do nothing 
      } else {
        pattern[i] = -1;  // turns off all but those 4 elements
      }
    }
  } 
  delays_.push_back(pattern);
}

double USTx::GetDelay() {
  // From datasheet Figure 54.
  return max_delay_ + 123.0 / CLK_FREQ + T_PROP + TR_DEL;
}

const std::pair<double, double>& USTx::GetFocusCoordinates(int idx) {
  return coordinates_[idx];
}

int USTx::Open(int port) {
  Serial::Init cfg;
  cfg.port = port;
  cfg.baud = BAUD_RATE;
  cfg.cts = true;
  int ret = serial_->Open(cfg);
  if (ret != 0) return ret;

  // The USTx sends a description of the CLI on boot, clear that
  ClearRx();
  return 0;
}

void USTx::Close() { serial_->Close(); }

int USTx::DownloadHex(const char* hex_fname) {
  FILE* fp = fopen(hex_fname, "r");
  if (fp == NULL) {
    printf("Could not open hex file for download: %s\n", hex_fname);
    return -1;
  }
  // check the length for download progress bar
  fseek(fp, 0, SEEK_END);
  int fsize = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  static const char PROGRESS_BAR_SIZE = 50;
  int bytes_sent = 0;
  int progress_printed = 0;
  double r = (double)fsize / PROGRESS_BAR_SIZE;

  printf("Sending Hex file, estimated time: %d seconds\n",
         fsize / (BAUD_RATE / 8));
  for (int i = 0; i < PROGRESS_BAR_SIZE; ++i) printf("_");
  printf("\n");
  char buf[80];
  while (fgets(buf, 80, fp)) {
    serial_->Transmit(buf);
    bytes_sent += strlen(buf);
    serial_->Receive((uint8_t*)buf, 80);
    if (bytes_sent > progress_printed * r) {
      printf("#");
      ++progress_printed;
    }
  }
  printf("#\nDownload Successful\n");
  ClearRx();

  fclose(fp);

  return 0;
}

void USTx::SetFocus(int focus, bool increment) {
  char buf[80];
  sprintf(buf, "%c %d\n", increment ? 'i' : 'f', focus);
  serial_->Transmit(buf);
  ClearRx();
}

void USTx::SetWaveformCycles(int cycles) {
  char buf[80];
  sprintf(buf, "c %d\n", cycles);
  serial_->Transmit(buf);
  ClearRx();
}

int USTx::GetFocus() {
  char buf[80];
  int focus;
  serial_->Transmit("f\n");

  for (int i = 0; i < 10000; ++i) {
    RxLine(buf);
    char* rsp = strstr(buf, "Focus:");
    if (!rsp) continue;
    if (sscanf(rsp, "Focus: %d", &focus)) return focus;
  }
  printf("USTx Serial Timout\n");
  assert(0);
  return -1;
}

int USTx::GetFocusCount() { return coordinates_.size(); }

bool USTx::ThermalShutdown() {
  char buf[80];
  serial_->Transmit("t\n");

  for (int i = 0; i < 10000; ++i) {
    RxLine(buf);
    if (strstr(buf, "OKAY")) return false;
    if (strstr(buf, "SHUTDOWN")) return true;
  }
  printf("USTx Serial Timout\n");
  assert(0);
  return false;
}

void USTx::RxLine(char* line, int max_len) {
  int b;
  while (max_len > 0) {
    b = serial_->Receive((uint8_t*)line, 1);
    if (b) {
      if (*line == '\n') {
        *line = '\0';
        return;
      }
      ++line;
      --max_len;
    }
  }
  assert(0);
}

void USTx::Reset() {
  serial_->Transmit("reset\n");
  ClearRx();
}

void USTx::ClearRx() {
  char c;
  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    if (serial_->Receive((uint8_t*)&c, 1) < 1) {
      break;
    } else {
      while (serial_->Receive((uint8_t*)&c, 1)) {
      }
    }
  }
}

double USTx::GetSpeedOfSound() {
  return speed_sound_;
}
