#pragma once

#include <string>

#include "system/third_party/Win64/Include/visa.h"

enum TriggerMode {
	EXTERNAL,
	MANUAL,
	INTERNAL
};

enum RigolPulseMode {
	CONTINUOUS_RIGOL,
	SINGLE_RIGOL,
	BURST_RIGOL,
	GATE_RIGOL
};

enum WaveformMode {
	SINE,
	SQUARE,
	PULSE
};

enum RigolType {
  FUNCTION_GENERATOR,
  OSCILLOSCOPE
};

// Rigol visa.h wrapper
class Rigol {
 public:
  Rigol() {}
  virtual ~Rigol() {}

  virtual bool Open(const char* serialNumber, RigolType deviceType);
  virtual bool Write(const std::string& s, RigolType deviceType) {
    switch (deviceType) {
      case FUNCTION_GENERATOR:
        return viWrite(functionGeneratorSession_, (ViBuf)s.c_str(), (ViUInt32)s.length(), VI_NULL) == VI_SUCCESS;
      case OSCILLOSCOPE:
        return viWrite(oscilloscopeSession_, (ViBuf)s.c_str(), (ViUInt32)s.length(), VI_NULL) == VI_SUCCESS;
    }
    return false;
  }

  virtual bool ReadWaveform(const std::string& filename) {
    return viReadToFile(oscilloscopeSession_, (ViConstString)filename.c_str(), 25000000, VI_NULL) == VI_SUCCESS;
  }

 private:
  ViSession resourceManagerSession_;
  ViSession functionGeneratorSession_;
  ViSession oscilloscopeSession_;
};

class RigolOscilloscope {
 public:
  // Connects to Rigol oscilloscope using shared resource Manager Session. Currently only functions
  // exist to read a triggered waveform from the scope and write it directly to a text file.
  // Future To-do (CR): enumerate additional functions to read/write X&Y spacing/limits/offset.
  // Note: read-out is time dependent (requires neccsesary wait time proportional to time-window of
   // actual data collection on the scope, or will return an empty header).
  RigolOscilloscope() {}
  ~RigolOscilloscope() { delete rigolScope_; }

  bool init(const char* serialNumber);

  int enableSingleTrigger();

  int initWaveformReadTrigger(int channelNum);

  int setTriggerChannel(int channelNum, double triggerLevel);

  int enableScope(bool runON);

  int getData(const std::string& filename);

 protected:
  bool write(const std::string&);

  bool readWaveform(const std::string&);

  Rigol* rigolScope_;
};

class RigolFunctionGenerator {
 public:
  RigolFunctionGenerator() {}
  ~RigolFunctionGenerator() { delete rigolFG_; }

  bool init(const char* serialNumber);

  int setVoltage(int channelNum, double Vpp_V);

	int setVoltageHighLowLevel(int channelNum, double Vhigh_V, double Vlow_V = 0.0);

	int setFrequency(int channelNum, double frequency_Hz);

	int setExternalTriggerMode(int channelNum, TriggerMode externalTriggerMode);

	int enableTriggerOut(int channelNum, bool triggerON);

	int setPulseMode(int channelNum, RigolPulseMode pulseMode, int numCycles = 0);

	int setPulseDelay(int channelNum, double pulseDelay_s);

	int setPulsePhase(int channelNum, double phase_deg);

	int setWaveform(int channelNum, WaveformMode waveform);

	int enableChannel(int channelNum, bool channelON);

	int triggerChannel(int channelNum);

 protected:
  // Write a string to the interface and pause briefly.
  bool write(const std::string&);

  Rigol* rigolFG_;
};
