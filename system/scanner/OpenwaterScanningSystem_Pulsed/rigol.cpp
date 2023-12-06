#include "rigol.h"

#include <string>
#include <iostream>
#include <thread>

// Until Rigol is pure virtual
#ifndef _MSC_VER
ViStatus _VI_FUNC  viOpen          (ViSession sesn, ViConstRsrc name, ViAccessMode mode,
                                    ViUInt32 timeout, ViPSession vi) { return 0; }
ViStatus _VI_FUNC  viOpenDefaultRM (ViPSession vi) { return 0; }
ViStatus _VI_FUNC  viReadToFile    (ViSession vi, ViConstString filename, ViUInt32 cnt,
                                    ViPUInt32 retCnt) { return 0; }
ViStatus _VI_FUNC  viSetAttribute  (ViObject vi, ViAttr attrName, ViAttrState) { return 0; }
ViStatus _VI_FUNC  viWrite         (ViSession, ViConstBuf, ViUInt32, ViPUInt32) { return 0; }
#endif

bool Rigol::Open(const char* serialNumber, RigolType deviceType) {
  // Open session with the resource manager
  viOpenDefaultRM(&resourceManagerSession_);
  ViStatus check = VI_SUCCESS;

  // Open session to the resource
  // Opening arbitrary number of rigol devices requires single resourceManagerSession.
  switch (deviceType) {
    case FUNCTION_GENERATOR:
      check = viOpen(resourceManagerSession_, ViString(serialNumber),
        VI_NULL, VI_NULL, &functionGeneratorSession_);
      if (check != VI_SUCCESS) throw "viOpen Function Generator failed";
      break;
    case OSCILLOSCOPE:
      check = viOpen(resourceManagerSession_, ViString(serialNumber), VI_NULL, VI_NULL,
        &oscilloscopeSession_);
      if (check != VI_SUCCESS) throw "viOpen Oscilloscope failed";
      viSetAttribute(oscilloscopeSession_, (ViAttr)VI_ATTR_FILE_APPEND_EN, VI_TRUE);
      break;
  }
  return check == VI_SUCCESS;
}

///// Oscilloscope Functions /////

bool RigolOscilloscope::init(const char* serialNumber) {
  rigolScope_ = new Rigol();
  return rigolScope_->Open(serialNumber, OSCILLOSCOPE);
}

bool RigolOscilloscope::write(const std::string& s) {
  bool status = rigolScope_->Write(s, OSCILLOSCOPE);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  return status == VI_SUCCESS;
}

bool RigolOscilloscope::readWaveform(const std::string& filename) {
  write(":WAV:DATA?");
  bool status = rigolScope_->ReadWaveform(filename);
  std::this_thread::sleep_for(std::chrono::milliseconds(10)); // TODO(CR): sleep thread as fn of scope window
  write("SING");
  return status == VI_SUCCESS;
}

int RigolOscilloscope::enableSingleTrigger() {
  return write("SING");
}

int RigolOscilloscope::initWaveformReadTrigger(int channelNum) {
  bool statusSource = write("WAV:SOUR CHAN" + std::to_string(channelNum));
  bool statusMode = write("WAV:MODE NORM");
  bool statusAscii = write("WAV:FORM ASC");
  return statusSource && statusMode && statusAscii;
}

int RigolOscilloscope::setTriggerChannel(int channelNum, double triggerLevel) {
  bool statusSource = write("TRIG:EDG:SOUR CHAN" + std::to_string(channelNum));
  bool statusSlope = write("TRIG:EDG:SLOP POS");
  bool statusLevel = write(":TRIG:EDG:LEV " + std::to_string(triggerLevel));
  return statusSource && statusSlope && statusLevel;
}

int RigolOscilloscope::enableScope(bool runON) {
  bool status = false;
  if (runON) {
    status = write("RUN");
  } else {
    status = write("STOP");
  }
  return status;
}

int RigolOscilloscope::getData(const std::string& filename) {
  return readWaveform(filename);
}

///// Function Generator Functions /////

bool RigolFunctionGenerator::init(const char* serialNumber) {
  rigolFG_ = new Rigol();
  return rigolFG_->Open(serialNumber, FUNCTION_GENERATOR);
}

bool RigolFunctionGenerator::write(const std::string& s) {
  bool status = rigolFG_->Write(s, FUNCTION_GENERATOR);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  return status == VI_SUCCESS;
}

int RigolFunctionGenerator::setVoltage(int channelNum, double Vpp_V) {
	return write(
      "SOUR" + std::to_string(channelNum) + ":VOLT:LEV:IMM:AMPL " + std::to_string(Vpp_V) + "\n");
}

int RigolFunctionGenerator::setVoltageHighLowLevel(int channelNum, double Vhigh_V, double Vlow_V) {
  std::string prefix = "SOUR" + std::to_string(channelNum);
  return write(prefix + ":VOLT:LEV:IMM:HIGH " + std::to_string(Vhigh_V) + "\n")
      && write(prefix + ":VOLT:LEV:IMM:LOW " + std::to_string(Vlow_V) + "\n");
}

int RigolFunctionGenerator::setFrequency(int channelNum, double frequency_Hz) {
  return write(
      "SOUR" + std::to_string(channelNum) + ":FREQ:FIX " + std::to_string(frequency_Hz) + "\n");
}

int RigolFunctionGenerator::setExternalTriggerMode(int channelNum, TriggerMode triggerMode) {
  const char* const mode[3] = { "EXT", "MAN", "INT" };
  return write("SOUR" + std::to_string(channelNum) + ":BURS:TRIG:SOUR " + mode[triggerMode] + "\n");
}

int RigolFunctionGenerator::enableTriggerOut(int channelNum, bool triggerON) {
  return write(
      "SOUR" + std::to_string(channelNum) + ":BURS:TRIG:TRIGO " + (triggerON ? "POS\n" : "OFF\n"));
}

int RigolFunctionGenerator::setPulseMode(int channelNum, RigolPulseMode pulseMode, int numCycles) {
  std::string prefix = "SOUR" + std::to_string(channelNum) + ":BURS:";
  bool sendBufOK1, sendBufOK2 = true, sendBufOK3 = true;
  switch (pulseMode) {
    case CONTINUOUS_RIGOL:
      sendBufOK1 = write(prefix + "STAT OFF\n");
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      break;
    case SINGLE_RIGOL:
      sendBufOK1 = write(prefix + "MODE TRIG\n");
      sendBufOK2 = write(prefix + "NCYC 1\n");
      sendBufOK3 = write(prefix + "STAT ON\n");
      break;
    case BURST_RIGOL:
      sendBufOK1 = write(prefix + "MODE TRIG\n");
      sendBufOK2 = write(prefix + "NCYC " + std::to_string(numCycles) + "\n");
      sendBufOK3 = write(prefix + "STAT ON\n");
      break;
    case GATE_RIGOL:
      sendBufOK1 = write(prefix + "MODE GAT\n");
      sendBufOK2 = write(prefix + "STAT ON\n");
      break;
  }
  return sendBufOK1 && sendBufOK2 && sendBufOK3;
}

int RigolFunctionGenerator::setPulseDelay(int channelNum, double pulseDelay_s) {
  return write(
      "SOUR" + std::to_string(channelNum) + ":BURS:TDEL " + std::to_string(pulseDelay_s) + "\n");
}

int RigolFunctionGenerator::setPulsePhase(int channelNum, double phase_deg) {
  return write(
      "SOUR" + std::to_string(channelNum) + ":BURS:PHAS " + std::to_string(phase_deg) + "\n");
}

int RigolFunctionGenerator::setWaveform(int channelNum, WaveformMode waveform) {
  const char* const mode[3] = { "SIN", "SQU", "PULS" };
  return write("SOUR" + std::to_string(channelNum) + ":APPL:" + mode[waveform] + "\n");
}

int RigolFunctionGenerator::enableChannel(int channelNum, bool channelON) {
  return write("OUTPUT" + std::to_string(channelNum) + (channelON ? " ON\n" : " OFF\n"));
}

int RigolFunctionGenerator::triggerChannel(int channelNum) {
  return write("SOUR" + std::to_string(channelNum) + ":BURS:TRIG IMM\n");
}
