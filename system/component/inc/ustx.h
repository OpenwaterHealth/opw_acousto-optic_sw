#pragma once

#include <vector>
#include <tuple>

#include "system/third_party/json-develop/single_include/nlohmann/json.hpp"

#include "system/component/inc/intelhex.h"
#include "system/component/inc/serial.h"


// Class to control the USTx PCB
// USTx needs a configuration hex file to focus at an (x,z) coordinates.
// This class can generate this configuration file from a JSON file, 
//   by calling Addfocus(), or both.
// The JSON file can also be used to specify physical parameters of the system,
//   such as the speed of sound in the medium, or the element pitch of the
//   transducer.
// An example JSON is shown here, which loads all parameters that are available.
/*
{
  "frequency" : 5e6,
  "waveforms": 1,
  "repeat_count": 5,
  "repeat_time": 0.001,
  "cw_mode": false,
  "speed_sound": 1500.0,
  "n_elements": 128,
  "pitch": 3e-4,
  "max_angle": 0.78,
  "f_number": 1,
  "probe": "L11-5v",
  "radius": 0,
  "max_elements": 30,
  "foci": [
    [0.000, 0.025],
    [-0.010, 0.010],
    [0.010, 0.010]
    ]
}    
*/
// All units are in MKS / radians
// This corresponds to these internal variables:
// double speed_sound_ = 1500.0;  // speed of sound in the medium
// int n_elements_ = 128;    // number of elements in the array.  Must be <=128
// double pitch_ = 3e-4;     // element pitch of the array
// double frequency_ = 5e6;  // desired wave frequency
// int waveforms_ = 1;       // number of waveforms in one repetition
// double max_angle_ 0.78;   // maxiumum angle between an element
//                           // and a focus (radians)
// double f_number_ = 1.0;   // maximum aperature for an element
// int repeat_count_ = 5;    // number of times to repeat the waveform
// double repeat_time = .001 // how long the waveform should be on.
                             //   overrides repeat_count if present
// bool cw_mode_ = false;    // whether the device is in CW mode
// std::string probe_ = "L11-5v"; // probe type (used to determine pinout and linear vs curvilinear)
// double radius_ = 0;           // radius of curvature (only used for curvilinear probe, C5-2)
// int max_elements_ = 30;        // maximum number of elements to use for curvilinear (C5-2) or phased (P4-2) probe;

// A note on waveforms and repeat count:
// The TX7332 can generate an arbitrary waveform, and repeat that waveform
//   between 1-32 times.  The arbitrary waveform can include between 1-4
//   cosine waves, allowing for a maximum of 128 total ultrasound cycles.
//   While the repeat count can be changed on the fly, the number of waveforms
//   cannot.
class USTx {
 public:
  USTx(Serial* port) : serial_(port) {}
  ~USTx() { Close();  }

  // Open communication to a USTx
  // @param port COM port number
  // @returns 0 if successful
  int Open(int port);

  // Close the USTx
  void Close();

  // Load system parameters and foci from a JSON
  // @param j JSON to read
  // @returns 0 if successful
  int LoadJson(const nlohmann::json& j); 

  // Add an (x, z) coordinate to the focus list
  void AddFocus(double x, double z);

  // Compute a focus list, and write it to a hex file
  bool ComputeFoci(const char* hex_fname);

  // Download a hex file to the USTx
  // @param hex_fname hex file to download
  // @returns 0 if successful
  // @note May not actually download hex file if the current checksum
  //   on the USTx matches the one requested to download
  int DownloadHex(const char* hex_fname);

  // Get delay between the trigger to USTx and the pulse arriving at
  //   the specified locations in seconds.
  // Returns -1 if not yet computed, or requested foci set is not achievable.
  double GetDelay();

  // Set a focus.
  // If inrement is true, USTx will auto-incrment the focus number 
  //   on each trigger received.
  // @param focus_index initial focus number
  // @param increment whether to auto-increment or stay on the same focus
  void SetFocus(int focus_index, bool increment = false);

  // Get the currently active focus
  int GetFocus();

  // Get the number of Foci
  int GetFocusCount();

  // Set the cycle count
  // @param cycles number of waveform cycles
  // @note cycles must be <= 32
  void SetWaveformCycles(int cycles);

  // access the (x,z) coordinates 
  // @param focus_index focus index to find coordinates of
  const std::pair<double, double>&
    GetFocusCoordinates(int focus_index);

  // Check if the USTx is in thermal shutdown
  // @returns true if in thermal shutdown
  bool ThermalShutdown();

  // Reset the device
  void Reset();

  // Get speed of sound (returns speed of sound in medium in m/s)
  double GetSpeedOfSound();

 private:
  // Length-Address-Data structure
  class LAD;

  // Compute a single element's delay to an (x,y).
  // @returns delay in seconds, <0 if element should be switched off
  double ComputeDelay(double x, double z, int element);

  // Compute foci for pulsed mode
  bool ComputePulsed(const char* hex_fname);

  // Compute foci for CW mode
  bool ComputeCW(const char* hex_fname);

  // Write hex file header
  void WriteHeader(const std::vector<LAD>& init, const std::vector<LAD>& focus, IntelHex& ih);

  // Clear out the recieve buffer
  void ClearRx();

  // Recieve a line from the UART
  void RxLine(char* line, int max_len = 80);

  // physical parameters.  May be overridden by JSON
  double speed_sound_ = 1500.0; // speed of sound in the medium
  int n_elements_ = 128;        // number of elements in the array.  Must be <=128
  double pitch_ = 3e-4;         // element pitch of the array
  double frequency_ = 5e6;      // desired wave frequency
  int waveforms_ = 1;           // number of waveforms in one repetition (<=4)
  double max_angle_ = .78;      // maxiumum angle between an element and a focus (radians)
  double f_number_ = 1.0;       // maximum aperature for an element
  int repeat_count_ = 1;        // number of times to repeat the waveform (<= 32)
  double repeat_time_ = -1;     // time to repeat (sets device in elastography mode)
                                //   overrides repeat_count if > 0
  bool cw_mode_ = false;        // whether the device is in CW mode
  std::string probe_ = "L11-5v"; // probe type (used to determine pinout and linear vs curvilinear)
  double radius_ = 0;           // radius of curvature (only used for curvilinear probe, C5-2)
  int max_elements_ = 30;        // maximum number of elements to use for curvilinear (C5-2) or phased (P4-2) probe;

  // realities of the USTx board
  const double CLK_FREQ = 200e6; // master clock frequency
  const double T_PROP = 5e-9;    // internal propegation delay of the part
  const double TR_DEL = 2e-6;    // delay needed between trigger and any element firing
  const int BAUD_RATE = 921600;  // baud rate of the UART

  // focus data
  std::vector<std::vector<double>> delays_;
  std::vector<std::pair<double, double>> coordinates_;
  std::vector<double> x_distances_;

  // maximum delay between any focus requested and the trigger
  double max_delay_ = -1;

  Serial* serial_;
};
