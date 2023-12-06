#include <cmath>
#include <string>
#include <iostream>

#include "nidaq.h"

NIDAQAnalogInput::NIDAQAnalogInput() {
  if (DAQmxCreateTask("", &niDAQ_AI_) < 0) assert(!"niDAQ create task failed");
}

NIDAQAnalogInput::~NIDAQAnalogInput() {
  DAQmxStopTask(niDAQ_AI_);
  DAQmxClearTask(niDAQ_AI_);
}

int NIDAQAnalogInput::initializeDevice(const json& systemParameters) {
  int deviceNumber = systemParameters["hardwareParameters"]["niDAQDevice"].get<int>();
  double overlapTime_ms = systemParameters["cameraParameters"].get<double>();
  int numScansPerChannel = (int)ceil(niDAQRate_Hz_ * overlapTime_ms);

  std::string devNum = "Dev" + std::to_string(deviceNumber) + "/ai0:3";

  setNumScansPerChannel(numScansPerChannel);

  DAQmxCreateAIVoltageChan(niDAQ_AI_, devNum.c_str(), "", DAQmx_Val_Cfg_Default, -10.0, 10.0, DAQmx_Val_Volts, NULL);
  DAQmxCfgSampClkTiming(niDAQ_AI_, "", niDAQRate_Hz_, DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, numScansPerChannel_);
  DAQmxCfgDigEdgeStartTrig(niDAQ_AI_, "PFI0", DAQmx_Val_Rising); // Trigger in; [rising edge TTL]

  return DAQmxStartTask(niDAQ_AI_) == 0;
}

double NIDAQAnalogInput::getRate() {
  return double(niDAQRate_Hz_);
}

int NIDAQAnalogInput::setNumScansPerChannel(int numScans) {
  numScansPerChannel_ = numScans;
  AIOData_.resize(numScansPerChannel_ * numAIOChannels_);
  return numScansPerChannel_;
}

std::vector<double> NIDAQAnalogInput::getDataStats() {
  std::vector<double> laserMonitorStats(numLaserMonitorStats_, 0.0);
  double ch0Mean_V = 0, ch0Std_V = 0;
  double ch1Mean_V = 0, ch1Std_V = 0;
  double ch2Mean_V = 0, ch2Std_V = 0;
  double ch3Mean_V = 0, ch3Std_V = 0;

  getAllChannelData();

  for (int i = 0; i < numAIOChannels_; i++) {
    for (int j = 0; j < numScansPerChannel_; j++) {
      if (i == 0) {
        ch0Mean_V += AIOData_[j + numScansPerChannel_ * i];
      } else if (i == 1) {
        ch1Mean_V += AIOData_[j + numScansPerChannel_ * i];
      } else if (i == 2) {
        ch2Mean_V += AIOData_[j + numScansPerChannel_ * i];
      } else if (i == 3) {
        ch3Mean_V += AIOData_[j + numScansPerChannel_ * i];
      }
    }
  }

  // Calc Mean
  ch0Mean_V = ch0Mean_V / numScansPerChannel_;
  ch1Mean_V = ch1Mean_V / numScansPerChannel_;
  ch2Mean_V = ch2Mean_V / numScansPerChannel_;
  ch3Mean_V = ch3Mean_V / numScansPerChannel_;

  // Calc STD
  for (int i = 0; i < numAIOChannels_; i++) {
    for (int j = 0; j < numScansPerChannel_; j++) {
      if (i == 0) {
        ch0Std_V += pow((AIOData_[j + numScansPerChannel_ * i] - ch0Mean_V), 2.0);
      } else if (i == 1) {
        ch1Std_V += pow((AIOData_[j + numScansPerChannel_ * i] - ch1Mean_V), 2.0);
      } else if (i == 2) {
        ch2Std_V += pow((AIOData_[j + numScansPerChannel_ * i] - ch2Mean_V), 2.0);
      } else if (i == 3) {
        ch3Std_V += pow((AIOData_[j + numScansPerChannel_ * i] - ch3Mean_V), 2.0);
      }
    }
  }

  ch0Std_V = sqrt(ch0Std_V / numScansPerChannel_);
  ch1Std_V = sqrt(ch1Std_V / numScansPerChannel_);
  ch2Std_V = sqrt(ch2Std_V / numScansPerChannel_);
  ch3Std_V = sqrt(ch3Std_V / numScansPerChannel_);

  laserMonitorStats[0] = (ch0Mean_V);
  laserMonitorStats[1] = (ch1Mean_V);
  laserMonitorStats[2] = (ch2Mean_V);
  laserMonitorStats[3] = (ch3Mean_V);
  laserMonitorStats[4] = (ch0Std_V);
  laserMonitorStats[5] = (ch1Std_V);
  laserMonitorStats[6] = (ch2Std_V);
  laserMonitorStats[7] = (ch3Std_V);

  return laserMonitorStats;
}

int NIDAQAnalogInput::getAllChannelData() {
  int timeout_s = 5;
  DAQmxReadAnalogF64(niDAQ_AI_, numScansPerChannel_, timeout_s, DAQmx_Val_GroupByChannel, &AIOData_[0],
    numScansPerChannel_ * numAIOChannels_, NULL, NULL);
  DAQmxStopTask(niDAQ_AI_);

  return DAQmxStartTask(niDAQ_AI_) == 0;
}
