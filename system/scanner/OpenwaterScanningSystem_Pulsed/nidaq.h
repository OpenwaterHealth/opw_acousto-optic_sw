#pragma once

#include "LaserMonitor.h"

#include "system/third_party/ni/inc/NIDAQmx.h"

class NIDAQAnalogInput : public LaserMonitor {
 public:
   NIDAQAnalogInput();
   ~NIDAQAnalogInput();

  int initializeDevice(const json& systemParameters);

  double getRate();

  int setNumScansPerChannel(int numScansPerChannel);

  // This function calls getAllChannelData, parses it by channel and returns stats about the signal
  // in the format of mean and std for each channel. This is called by the scanner at each voxel.
  virtual std::vector<double> getDataStats();

  // Function to query actual niDAQ device; returns all channel data in a single vector
  // This function is called internally by getDataStats
  int getAllChannelData();

 private:
  TaskHandle niDAQ_AI_;
  int numAIOChannels_ = 4; // Total number of channels on DAQ
  float64 niDAQRate_Hz_ = 50000 / numAIOChannels_; // 50kSample DAQ divided by number of channels
  int numScansPerChannel_ = 1;
  std::vector<float64> AIOData_; // (numScansPerChannel_* numAIOChannels_);
};
