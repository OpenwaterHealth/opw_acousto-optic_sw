// fftutilExplora.cpp
// Caitlin Regan, Carsten Jensen [Openwater]

#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <tuple>

#include "system/component/inc/cli.h"
#include "system/component/inc/fftt.h"
#include "system/component/inc/frame_draw.h"
#include "system/component/inc/fx3.h"
#include "system/component/inc/rcam.h"
#include "system/component/inc/roi.h"
#include "system/component/inc/stddev.h"
#include "system/component/inc/time.h"
#include "system/scanner/OpenwaterScanningSystem_Pulsed/framesave.h"
#include "system/scanner/OpenwaterScanningSystem_Pulsed/laser.h"
#include "system/scanner/OpenwaterScanningSystem_Pulsed/QuantumComposers.h"
#include "system/third_party/json-develop/single_include/nlohmann/json.hpp"
#include "system/util/fftutil/framestats.h"

#define SFML_STATIC
#include "SFML/Graphics.hpp"
#include "SFML/Window.hpp"
#pragma comment(lib, "sfml-graphics-s.lib")
#pragma comment(lib, "sfml-window-s.lib")
#pragma comment(lib, "sfml-system-s.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "freetype.lib")

#pragma comment(lib, "libfftw3-3.lib")

#include "windows.h"

#include "OctopusManager_Align.h"

using json = nlohmann::json;

enum class ScanningSystem {
  CW,
  CW_FF,
  PULSED,
  PSEUDOPULSED,
  PSEUDOPULSED_BF
};

ScanningSystem scanningSystem = ScanningSystem::CW_FF;
OctopusManager_Align* OctopusManager = NULL;
int cameraXResolution = 2712;
int cameraYResolution = 2080;

// simple decimation node, passes every nth frame
class Decimate : public ExecNode {
public:
  Decimate(int n) : n_(n) {}
  ~Decimate() {}

private:
  void* Exec(void* data) override {
    std::lock_guard<std::mutex> lock(m_);
    if ((++frame_count_) % n_ == 0) return data;
    return NULL;
  }

  int frame_count_ = 0;
  int n_ = 0;
  std::mutex m_;
};

const char* Echo(void* param, const char* s) {
  printf("String: %s\n", s);
  printf("Len: %zd\n", strlen(s));
  return "OKAY";
}

const char* CommandHelp(void* param, const char* s) {
  static char buf[800];
  int menuPage = 1;
  sscanf_s(s, "%i", &menuPage);
  if (menuPage == 1) {
    snprintf(buf, 800, "\
      aom <AO channel> <1|0> [Octopus only]\n\
      freq <AO channel> <freq_MHz> [Octopus only]\n\
      gain <1-32>\n\
      help <1|2>\n\
      id <camera ID>\n\
      overlap <pseudo-pulse width_s> [Octopus only]\n\
      save <path/name> <num Images>\n\
      temp\n\
      volt <AO channel> <voltage_Vpeak> [Octopus only]\n");
  } else if (menuPage == 2) {
    snprintf(buf, 800, "\
      blc <on|off>\n\
      del <OUT2BOTTOM delay_s> [structural only]\n\
      exp <exposureTime_s> [CW only]\n\
      fft <lin|log> <min_val> <max_val>\n\
      r <addr> <bytes (optional)>\n\
      reset [warning: tested only in CW]\n\
      roi (on|off) <x center> <y center> <radius> [warning: bug?]\n\
      w <addr> <val> <bytes (optional)>\n");
  }
  return buf;
}

const char* Exposure(void* param, const char* s) {
  static char buf[80];
  double exp;
  if (sscanf_s(s, "%lf", &exp) == 1) {
    if (scanningSystem == ScanningSystem::CW || scanningSystem == ScanningSystem::CW_FF) {
      ((Rcam*)param)->SetExposure(exp / 1000);
    } else {
      snprintf(buf, 80, "I'm sorry Dave, I'm afraid I can't do that...");
    }
  }
  else {
    snprintf(buf, 80, "Exposure: %.1lf", ((Rcam*)param)->GetExposure() * 1000);
  }

  return buf;
}

const char* setAOMFreq(void* param, const char* s) {
  static char buf[80];
  double freq_MHz;
  int aomChannel;
  double mHz2Hz = 1000000;
  OctopusManager_Align::AOM_Settings* aomSettings = (OctopusManager_Align::AOM_Settings*)param;
  if (sscanf(s, "%i %lf", &aomChannel, &freq_MHz) == 2) {
    if (scanningSystem == ScanningSystem::PULSED or scanningSystem == ScanningSystem::PSEUDOPULSED or scanningSystem == ScanningSystem::PSEUDOPULSED_BF) {
      if (aomChannel == 1) {
        aomSettings->AOM1Freq_Hz = freq_MHz * mHz2Hz;
        OctopusManager->SetAOM(OctopusManager_Align::AOM_ShiftDirection::UPSHIFT, aomSettings->AOM1Freq_Hz, aomSettings->AOM1Volt_V);
      } else if (aomChannel == 2) {
        aomSettings->AOM2Freq_Hz = freq_MHz * mHz2Hz;
        OctopusManager->SetAOM(OctopusManager_Align::AOM_ShiftDirection::DOWNSHIFT, aomSettings->AOM2Freq_Hz, aomSettings->AOM2Volt_V);
      } else if (aomChannel == 3) {
        aomSettings->AOM3Freq_Hz = freq_MHz * mHz2Hz;
        OctopusManager->SetAOM(OctopusManager_Align::AOM_ShiftDirection::ULTRASOUND, aomSettings->AOM3Freq_Hz, aomSettings->AOM3Volt_V);
      } else if (aomChannel == 4) {
        aomSettings->AOM4Freq_Hz = freq_MHz * mHz2Hz;
        OctopusManager->SetAOM(OctopusManager_Align::AOM_ShiftDirection::CHOP, aomSettings->AOM4Freq_Hz, aomSettings->AOM4Volt_V);
      } else {
        snprintf(buf, 80, "freq <AO channel> <freq_MHz>");
      }
    }
    else {
      snprintf(buf, 80, "You dont look like an octopus to me");
    }
  }

  return buf;
}

const char* setAOMVolt(void* param, const char* s) {
  static char buf[80];
  double voltage_Vp;
  int aomChannel;
  OctopusManager_Align::AOM_Settings* aomSettings = (OctopusManager_Align::AOM_Settings*)param;
  if (sscanf(s, "%i %lf", &aomChannel, &voltage_Vp) == 2) {
    if (scanningSystem == ScanningSystem::PULSED or scanningSystem == ScanningSystem::PSEUDOPULSED or scanningSystem == ScanningSystem::PSEUDOPULSED_BF) {
      if (aomChannel == 1) {
        aomSettings->AOM1Volt_V = voltage_Vp;
        OctopusManager->SetAOM(OctopusManager_Align::AOM_ShiftDirection::UPSHIFT, aomSettings->AOM1Freq_Hz, aomSettings->AOM1Volt_V);
      } else if (aomChannel == 2) {
        aomSettings->AOM2Volt_V = voltage_Vp;
        OctopusManager->SetAOM(OctopusManager_Align::AOM_ShiftDirection::DOWNSHIFT, aomSettings->AOM2Freq_Hz, aomSettings->AOM2Volt_V);
      } else if (aomChannel == 3) {
        aomSettings->AOM3Volt_V = voltage_Vp;
        OctopusManager->SetAOM(OctopusManager_Align::AOM_ShiftDirection::ULTRASOUND, aomSettings->AOM3Freq_Hz, aomSettings->AOM3Volt_V);
      } else if (aomChannel == 4) {
        aomSettings->AOM4Volt_V = voltage_Vp;
        OctopusManager->SetAOM(OctopusManager_Align::AOM_ShiftDirection::CHOP, aomSettings->AOM4Freq_Hz, aomSettings->AOM4Volt_V);
      } else {
        snprintf(buf, 80, "volt <AOM channel> <voltage_Vp>");
      }
    } else {
      snprintf(buf, 80, "You dont look like an octopus to me");
    }
  }

  return buf;
}

const char* toggleAOM(void* param, const char* s) {
  static char buf[80];
  int aomChannel;
  double* currentOverlap_s;
  double* currentAOMDelay_s;
  double* currentPulsewidth_s;
  std::tie(currentAOMDelay_s, currentOverlap_s, currentPulsewidth_s) = *((std::tuple<double*, double*, double*>*)param);
  bool state;
  if (sscanf(s, "%i %i", &aomChannel, &state) == 2) {
    if (scanningSystem == ScanningSystem::PULSED or scanningSystem == ScanningSystem::PSEUDOPULSED or scanningSystem == ScanningSystem::PSEUDOPULSED_BF) {
      if (aomChannel == 1) {
        OctopusManager->EnableAOM(OctopusManager_Align::AOM_ShiftDirection::UPSHIFT, state, currentAOMDelay_s[0], currentOverlap_s[0]);
      } else if (aomChannel == 2) {
        OctopusManager->EnableAOM(OctopusManager_Align::AOM_ShiftDirection::DOWNSHIFT, state, currentAOMDelay_s[0], currentOverlap_s[0]);
      } else if (aomChannel == 3) {
        OctopusManager->EnableAOM(OctopusManager_Align::AOM_ShiftDirection::ULTRASOUND, state, currentAOMDelay_s[0], currentPulsewidth_s[0]);
      } else if (aomChannel == 4) {
        OctopusManager->EnableAOM(OctopusManager_Align::AOM_ShiftDirection::CHOP, state, currentAOMDelay_s[0], currentPulsewidth_s[0]);
      } else {
        snprintf(buf, 80, "Invalid channel, please choose 1-4");
      }
    } else {
      snprintf(buf, 80, "You dont look like an octopus to me...");
    }
  }

  return buf;
}

const char* USDelay(void* param, const char* s) {
  static  char buf[80];
  if (scanningSystem != ScanningSystem::PSEUDOPULSED) {
    snprintf(buf, 80, "Feature only enabled in pseudo-pulsed mode");
    return buf;
  } else if (scanningSystem == ScanningSystem::PSEUDOPULSED) {
    // del  <delay in seconds> allows the user to change the delay to OUT2_BOTTOM driving the ultrasound
    bool channelON = true;
    bool channelOFF = false;
    double usDelayInput;
    double* currentUSDelay_s;
    double* currentOverlap_s;
    double* cameraFSINDelay_s;
    double* currentAOMDelay_s;
    double* currentPulsewidth_s;
    std::tie(currentUSDelay_s, currentOverlap_s, currentPulsewidth_s, cameraFSINDelay_s, currentAOMDelay_s) = *(
      (std::tuple<double*, double*, double*, double*, double*>*)param);

    if (sscanf_s(s, "%lf", &usDelayInput) == 1) {
      OctopusManager->SetUltrasoundTriggerDelay(usDelayInput, true);
      currentUSDelay_s[0] = usDelayInput;
      snprintf(buf, 80, "OUT2 BOTTOM delay: %s", std::to_string(currentUSDelay_s[0]).c_str());
      return buf;
    } else {
      // del with no value turns off channel B
      OctopusManager->SetUltrasoundTriggerDelay(*currentUSDelay_s, false);
      snprintf(buf, 80, "OUT2 BOTTOM delay was: %s. OUT2 BOTTOM off", std::to_string(currentUSDelay_s[0]).c_str());
      return buf;
    }
  }
}

const char* USOverlap(void* param, const char* s) {
  static  char buf[80];
  if (scanningSystem != ScanningSystem::PSEUDOPULSED && scanningSystem != ScanningSystem::PSEUDOPULSED_BF) {
    snprintf(buf, 80, "Feature only enabled in pseudo-pulsed mode");
    return buf;
  }
  else {
    // overlap  <overlap in seconds> allows the user to change effective pulse width to
    // OUT2_BOTTOM triggering the ultrasound, the AOM plate AOMs, and the AOM driving the pulse chopper
    bool channelON = true;
    bool channelOFF = false;
    double overlapInput;
    double* currentUSDelay_s;
    double* currentOverlap_s;
    double* cameraFSINDelay_s;
    double* currentAOMDelay_s;
    double* currentPulsewidth_s;
    Rcam* cameraObject;
    std::tie(currentOverlap_s, currentPulsewidth_s, cameraObject, currentUSDelay_s, cameraFSINDelay_s, currentAOMDelay_s) = *(
      (std::tuple<double*, double*, Rcam*, double*, double*, double*>*)param);

    if (sscanf_s(s, "%lf", &overlapInput) == 1) {
      if (overlapInput > 0.01) {
        snprintf(buf, 80, "Effective pulse width must be less than 10ms");
        return buf;
      }
      if (scanningSystem == ScanningSystem::PSEUDOPULSED) {
        // Start time of AOM choppers fixed in pesudo-pulsed bloodflow systems
        // TODO(CR) update pseudopulsed structural systems to use fixed overlap and variable pulsewidth
        currentUSDelay_s[0] = cameraFSINDelay_s[0] - overlapInput;
        currentAOMDelay_s[0] = cameraFSINDelay_s[0] - overlapInput;
        double currentExposureTime_s = cameraObject->GetExposure();
        cameraObject->SetExposure(currentExposureTime_s - currentOverlap_s[0] + overlapInput);
        currentOverlap_s[0] = overlapInput;
      }
      currentPulsewidth_s[0] = overlapInput;
      OctopusManager->SetUltrasoundTriggerDelay(currentUSDelay_s[0], true);  // Note: this will turn on the ultrasound trigger
      OctopusManager->EnableAOM(OctopusManager_Align::AOM_ShiftDirection::UPSHIFT, true, currentAOMDelay_s[0], overlapInput);
      OctopusManager->EnableAOM(OctopusManager_Align::AOM_ShiftDirection::DOWNSHIFT, true, currentAOMDelay_s[0], overlapInput);
      OctopusManager->EnableAOM(OctopusManager_Align::AOM_ShiftDirection::ULTRASOUND, true, currentAOMDelay_s[0], overlapInput);
      OctopusManager->EnableAOM(OctopusManager_Align::AOM_ShiftDirection::CHOP, true, currentAOMDelay_s[0], overlapInput);

      snprintf(buf, 80, "Effective pulse width: %s", std::to_string(currentPulsewidth_s[0]).c_str());
      return buf;
    }
    else {
      // overlap with no value returns current settings
      snprintf(buf, 80, "Effective pulse width: %s", std::to_string(currentPulsewidth_s[0]).c_str());
      return buf;
    }
  }
}

const char* CameraID(void* param, const char* s) {
  // id <id number> allows user to switch between any camera that was connected to
  // the computer when the program was opened
  static char buf[80];
  int cameraIDinput;
  int cameraUSBPort;
  int cameraID;
  Rcam* cameraObject;
  std::map<int, int>* cameraPortMap;
  double cameraExposureTime;
  std::tie(cameraObject, cameraPortMap, cameraExposureTime) = *((std::tuple<Rcam*, std::map<int, int>*, double>*)param);

  if (sscanf_s(s, "%i", &cameraIDinput) == 1) {
    cameraID = cameraIDinput;

    if (cameraPortMap->count(cameraID)) {
      cameraUSBPort = cameraPortMap->at(cameraID); // Determine what USB port desired camera
      cameraObject->Close(); // Close current camera object
      cameraObject->Open(cameraUSBPort); // Open desired camera object
      cameraObject->SetExposure(cameraExposureTime); // Ensure exposure time is set correctly
      cameraObject->SetBLC(false); // Default BLC off
      cameraObject->SubWindow2Point(0, 0, cameraXResolution, cameraYResolution);
      if (scanningSystem == ScanningSystem::CW or scanningSystem == ScanningSystem::CW_FF) {
        cameraObject->SetStream(true);
      }
      Component::SleepMs(1000);
      cameraObject->Start();
      // for Explora system
      snprintf(buf, 80, "Now connected to: %i", cameraID);
    }
    else {
      // If camera<id number> is not connectd, do nothing
      snprintf(buf, 80, "%i is not connected", cameraID);
    }
  }
  else {
    // id command with no number returns the ID of the camera currently running
    int cameraID = cameraObject->SerialNumber();
    snprintf(buf, 80, "Currently connected to: camera %i", cameraID);
  }
  return buf;
}

const char* RegSet(void* param, const char* s) {
  static char buf[80];
  int addr, val;
  int bytes = 1;
  if (sscanf_s(s, "%x %x %d", &addr, &val, &bytes) >= 2) {
    if (bytes > 2 || bytes <= 0) {
      snprintf(buf, 80, "Can only write 2 bytes max");
    }
    else {
      ((Rcam*)param)->WriteCFG(addr, val, bytes);
      snprintf(buf, 80, "Wrote %04x: %*x", addr, bytes * 2, val);
    }
  }
  else {
    snprintf(buf, 80, "Usage: w <addr> <val> <bytes (optional)>");
  }

  return buf;
}

const char* RegGet(void* param, const char* s) {
  static char buf[80];
  int addr;
  int bytes = 1;
  if (sscanf_s(s, "%x %d", &addr, &bytes) >= 1) {
    if (bytes > 2 || bytes <= 0) {
      snprintf(buf, 80, "Can only write 2 bytes max");
    }
    else {
      snprintf(buf, 80, "Read %04x: %*x", addr, bytes * 2,
        ((Rcam*)param)->ReadCFG(addr, bytes));
    }
  }
  else {
    snprintf(buf, 80, "Usage: r <addr> <bytes (optional)>");
  }

  return buf;
}

bool g_roi = true;
const char* RoiSet(void* param, const char* s) {
  static char buf[80];
  int x, y, r;
  if (sscanf_s(s, "%d %d %d", &x, &y, &r) == 3) {
    ((ROI*)param)->Set(x, y, r);
    snprintf(buf, 80, "roi set to %d, %d: %d", x, y, r);
  }
  else if (strncmp(s, "off", 80) == 0) {
    g_roi = false;
  }
  else if (strncmp(s, "on", 80) == 0) {
    g_roi = true;
  }
  else if (strncmp(s, "", 80) == 0) {
    ((ROI*)param)->Get(&x, &y, &r);
    snprintf(buf, 80, "ROI: (%d, %d) r: %d", x, y, r);
  }
  else {
    snprintf(buf, 80, "Usage: roi (on|off) <x center> <y center> <radius>");
  }

  return buf;
}

const char* FFT(void* param, const char* s) {
  static char buf[80];
  char scale[80] = { 0 };
  double min_val, max_val;
  if (sscanf_s(s, "%s %lf %lf", &scale, (unsigned int)sizeof(scale), &min_val, &max_val) == 3) {
    ((FFTTDraw*)param)->SetScale(strncmp("log", scale, 80) == 0, min_val, max_val);
    snprintf(buf, 80, "FFT %s scale to %.1le to %.1le", scale, min_val, max_val);
  } else {
    snprintf(buf, 80, "Usage: fft <lin|log> <min_val> <max_val>");
  }

  return buf;
}

const char* SaveFrame(void* param, const char* s) {
  static char buf[80];
  char fname[80];
  int n = 1;
  sscanf_s(s, "%s %d", fname, 80, &n);
  ((FrameSave*)param)->setFilename(std::string(fname), n);
  if (n > 1) {
    snprintf(buf, 80, "Writing %d images %s.tiff to file", n, fname);
  } else {
    snprintf(buf, 80, "Writing %s.tiff to file", fname);
  }
  return buf;
}

const char* Temperature(void* param, const char* s) {
  static char buf[80];
  snprintf(buf, 80, "CMOS Temperature: %.1lf", ((Rcam*)param)->Temperature());
  return buf;
}

const char* ResetCamera(void* param, const char* s) {
  ((Rcam*)param)->Reset();
  return "Camera Reset";
}

const char* CameraGain(void* param, const char* s) {
  static char buf[80];
  double gain;
  if (sscanf_s(s, "%lf", &gain)) {
    ((Rcam*)param)->SetGain(gain);
    snprintf(buf, 80, "Gain set to %.2lf", gain);
  } else {
    snprintf(buf, 80, "Usage: gain <1-32>");
  }

  return buf;
}

const char* CameraBLC(void* param, const char* s) {
  Rcam* cam = (Rcam*)param;
  if (strncmp(s, "on", 2) == 0) {
    cam->SetBLC(true);
    return "Black Level Correction On";
  } else {
    cam->SetBLC(false);
    return "Black Level Correction Off";
  }
}

static const int WIN_X = 1280;
static const int WIN_Y = 720;
static const std::string CFG_FNAME = "fft.cfg";

QuantumComposers* initializeLaser_RealTimeFFT() {
  QuantumComposers* AmplitudeLaser = NULL;
  int laserCOM = 5;  // Dussik
  bool channelON = true;

  if (scanningSystem == ScanningSystem::PULSED) {
    // Initialize Unit
    AmplitudeLaser = new QuantumComposers();
    AmplitudeLaser->init(laserCOM);
    // Turn on Channel G (To FSIN) & H (To Octopus)
    AmplitudeLaser->enableChannel(7, channelON);
    AmplitudeLaser->enableChannel(8, channelON);
  }
  return AmplitudeLaser;
}

int main(int argc, char* argv[]) {
  // Process argument (default to full frame CW for testing)
  std::string scanningSystemStr;
  if (argc > 1) {
    scanningSystemStr = argv[1];
  }
  else {
    scanningSystemStr = "cwFF";
  }

  if (scanningSystemStr.compare("Curie") == 0) {
    scanningSystem = ScanningSystem::PSEUDOPULSED;
  }
  else if (scanningSystemStr.compare("Dussik") == 0) {
    scanningSystem = ScanningSystem::PULSED;
  }
  else if (scanningSystemStr.compare("Fessenden") == 0) {
    scanningSystem = ScanningSystem::PSEUDOPULSED;
  }
  else if (scanningSystemStr.compare("Franklin") == 0) {
    scanningSystem = ScanningSystem::PSEUDOPULSED_BF;
  }
  else if (scanningSystemStr.compare("Gabor") == 0) {
    scanningSystem = ScanningSystem::PSEUDOPULSED_BF;
  }
  else if (scanningSystemStr.compare("RamanNath") == 0) {
    scanningSystem = ScanningSystem::PSEUDOPULSED;
  }
  else if (scanningSystemStr.compare("Sahl") == 0) {
    scanningSystem = ScanningSystem::PSEUDOPULSED_BF;
  }
  else if (scanningSystemStr.compare("cw") == 0) {
    scanningSystem = ScanningSystem::CW;
    // Note: this isn't called by any scanner currently, but kept for 2048x2048 debug mode
  }

  QuantumComposers* AmplitudeLaser = NULL;

  double cameraExposureTime_s = 0.003;
  double cameraFSINDelay_s = 0.01; // 10ms delay to allow for overlap
  double currentOverlap_s = 0.0; // Only for pseudo-pulsed systems [note bad terminology difference w/ overlap & pulsewidth]; TODO combine overlap & pulsewidth
  double currentUSDelay_s = 0.0;
  double currentAOMDelay_s = 0.0;
  double currentPulsewidth_s = 0.0; // Pulsewidth for bloodflow systems
  double mHz2Hz = 1000000.0;
  OctopusManager_Align::AOM_Settings aomSettings;

  if (scanningSystem == ScanningSystem::PULSED) {
    cameraExposureTime_s = 0.0099; // Required for 600 line, 100Hz HM5530, LMLF laser
    cameraYResolution = 512;
    currentUSDelay_s = 0.000565656; // Explora specific
    currentOverlap_s = 0.001;
    currentAOMDelay_s = 0.000565656;
    cameraFSINDelay_s = 0.00081;
    aomSettings.AOM2Freq_Hz = 95 * mHz2Hz;
    aomSettings.AOM2Volt_V = 7.0;  // Directly to amplifier
    OctopusManager = new OctopusManager_Align();
    OctopusManager->init(true, false, false, Rcam::NumCameras(), aomSettings, currentUSDelay_s, currentOverlap_s, cameraFSINDelay_s, currentAOMDelay_s);
    //Initialize Laser //
    AmplitudeLaser = initializeLaser_RealTimeFFT();
  } else if (scanningSystemStr == "Fessenden") {
    cameraExposureTime_s = 0.0324084; // 0.05ms overlap for Fessenden fake pulsed system
    currentOverlap_s = 0.00005;
    aomSettings.AOM4Freq_Hz = 50 * mHz2Hz;  // Frequency for AOM
    aomSettings.AOM4Volt_V = 0.3;  // Vp for AOM chopper amplifier
  } else if (scanningSystemStr == "Curie") {
    cameraExposureTime_s = 0.0343584; // 2ms overlap for Curie fake pulsed system
    currentOverlap_s = 0.002;
    aomSettings.AOM4Freq_Hz = 40 * mHz2Hz;  // Frequency for AOM
    aomSettings.AOM4Volt_V = 1.0;  // Vp for AOM chopper amplifier
  } else if (scanningSystemStr == "Franklin") {
    cameraExposureTime_s = 0.034064;  // All rows on + 1.2ms overlap
    currentOverlap_s = 0.0012; // 1.2ms overlap for pseudopulsed bloodflow system, bad terminology (overlap != pulsewidth for bloodflow)
    currentPulsewidth_s = 0.0005; // default 0.5ms pulsewidth for bloodflow
    aomSettings.AOM4Freq_Hz = 100 * mHz2Hz;  // Frequency for main AOM chopper
    aomSettings.AOM4Volt_V = 0.26;  // Vp for AOM chopper amplifier
  } else if (scanningSystemStr == "Gabor") {
    cameraExposureTime_s = 0.034064;  // All rows on + 1.2ms overlap
    currentOverlap_s = 0.0012; // 1.2ms overlap for pseudopulsed bloodflow system, bad terminology (overlap != pulsewidth for bloodflow)
    currentPulsewidth_s = 0.0005; // default 0.5ms pulsewidth for bloodflow
    aomSettings.AOM3Freq_Hz = 100 * mHz2Hz;  // Frequency for 2nd AOM chopping AOM
    aomSettings.AOM3Volt_V = 0.24;  // Vp for AOM chopper amplifier
    aomSettings.AOM4Freq_Hz = 100 * mHz2Hz;  // Frequency for main AOM chopper
    aomSettings.AOM4Volt_V = 0.24;  // Vp for AOM chopper amplifier
  } else if (scanningSystemStr == "RamanNath") {
    cameraExposureTime_s = 0.0343584; // 2ms overlap for Raman Nath fake pulsed system
    currentOverlap_s = 0.002;
    aomSettings.AOM4Freq_Hz = 40 * mHz2Hz;  // Frequency for AOM
    aomSettings.AOM4Volt_V = 1.0;  // Vp for AOM chopper amplifier [TODO: measure on actual system]
  } else if (scanningSystemStr == "Sahl") {
    cameraExposureTime_s = 0.034064;  // All rows on + 1.2ms overlap
    currentOverlap_s = 0.0012; // 1.2ms overlap for pseudopulsed bloodflow system, bad terminology (overlap != pulsewidth for bloodflow)
    currentPulsewidth_s = 0.0005; // default 0.5ms pulsewidth for bloodflow
    aomSettings.AOM4Freq_Hz = 100 * mHz2Hz;  // Frequency for main AOM chopper
    aomSettings.AOM4Volt_V = 0.1;  // Vp for AOM chopper amplifier
  }
  
  if (scanningSystem == ScanningSystem::PSEUDOPULSED) {
    currentUSDelay_s = cameraFSINDelay_s - currentOverlap_s; // Pseudo-pulsed system
    currentAOMDelay_s = cameraFSINDelay_s - currentOverlap_s; // Pseudo-pulsed system
    currentPulsewidth_s = currentOverlap_s;
    OctopusManager = new OctopusManager_Align();
    // note: structural PP use overlap & pulsewidth the same
    OctopusManager->init(false, true, false, Rcam::NumCameras(), aomSettings, currentUSDelay_s, currentPulsewidth_s, cameraFSINDelay_s, currentAOMDelay_s);
  } else if (scanningSystem == ScanningSystem::PSEUDOPULSED_BF) {
    currentUSDelay_s = cameraFSINDelay_s - currentOverlap_s + 0.0001; // Pseudo-pulsed  bloodflow system
    currentAOMDelay_s = cameraFSINDelay_s - currentOverlap_s + 0.0001; // Pseudo-pulsed system
    OctopusManager = new OctopusManager_Align();
    // note: bloodflow PP have fixed overlap and variable pulsewidth
    OctopusManager->init(false, true, true, Rcam::NumCameras(), aomSettings, currentUSDelay_s, currentPulsewidth_s, cameraFSINDelay_s, currentAOMDelay_s);
  } else if (scanningSystem == ScanningSystem::CW) {
    // Power of two CW (used in structural scanners)
    cameraYResolution = 2048;
    cameraYResolution = 2048;
  }

  // check for custom resolution if CW (other modes have spcific timing requirements)
  if (scanningSystem == ScanningSystem::CW || scanningSystem == ScanningSystem::CW_FF) {
    std::ifstream cfg(CFG_FNAME);
    if (!cfg.fail()) {
      json jcfg = json::parse(cfg);
      if (jcfg.contains("resolution")) {
        cameraXResolution = jcfg["resolution"]["width"];
        cameraYResolution = jcfg["resolution"]["height"];
      }
      cfg.close();
    }
  }

  ////////// MULTICAMERA SETUP //////////
  // How many cameras are in the system?
  int numCameras = Rcam::NumCameras();
  std::map<int, int> cameraPortMap; // Container mapping camera ID to USB ports
  // [key: camera<ID>, value: port]

  // Map camera serial numbers to USB ports
  for (int i = 0; i < numCameras; i++) {
    Rcam* tempCamera = new Rcam(); // Create new Rcam object
    tempCamera->Open(i);  // Open Rcam object at port i
    int cameraID = tempCamera->SerialNumber(); // serial number now returns cameraID
    printf("Found camera %d\n", cameraID);
    Component::SleepMs(1000);
    cameraPortMap[cameraID] = i; // Map camera location (camera<ID>) to camera USB port
    tempCamera->Close();
    delete tempCamera; // Disconnect from camera
  }

  ////////// END MULTICAMERA SETUP //////////

  sf::RenderWindow window(sf::VideoMode(WIN_X, WIN_Y), "FFT Utility");

  // Dataflow objects
  int decimate = 10;
  if (scanningSystem == ScanningSystem::PULSED) {
    decimate = 100;
  }
  Rcam camera;
  camera.Open(0);
  camera.SubWindow2Point(0, 0, cameraXResolution, cameraYResolution);
  camera.SetBLC(false);  // Default to not use black level correction
  camera.SetExposure(cameraExposureTime_s);
  Decimate dec(decimate);
  FrameDraw df(camera.GetConfig());
  FFTT fftt(camera.GetConfig());
  FFTTDraw dfftt(camera.GetConfig());
  FFTTSubWindowDraw dsw;
  dsw.Resize(camera.GetConfig());
  ROI roi(camera.GetConfig());
  ROIDraw droi(camera.GetConfig());
  StdDev stddev;
  FrameStats stats;
  stats.Init(camera.GetConfig());
  FrameSave save;

  // Dataflow connections
  dec.AddProducer(&camera);
  save.AddProducer(&camera);
  fftt.AddProducer(&dec);
  roi.AddProducer(&fftt);
  stddev.AddProducer(&roi);
  stats.AddProducer(&stddev);
  df.AddProducer(&camera);
  dfftt.AddProducer(&stats);
  dsw.AddProducer(&stats);
  droi.AddProducer(&stats);

  // Load ROI / FFT settings from config file if present
  {
    std::ifstream cfg(CFG_FNAME);
    if (cfg.fail()) {
      Frame* fr_cfg = camera.GetConfig();
      roi.Set(fr_cfg->height / 4, fr_cfg->height / 4, fr_cfg->height / 8);
    }
    else {
      json jcfg = json::parse(cfg);
      roi.Set(jcfg["ROI"]["x_c"].get<int>(), jcfg["ROI"]["y_c"].get<int>(),
        jcfg["ROI"]["r"].get<int>());
      dfftt.SetScale(jcfg["FFT"]["compute_log"].get<bool>(),
        jcfg["FFT"]["min"].get<double>(),
        jcfg["FFT"]["max"].get<double>());
      cfg.close();
    }
  }

  // CLI commands
  CLI cli(20);
  cli.Register("echo", Echo, NULL);
  cli.Register("exp", Exposure, (void*)& camera);
  cli.Register("w", RegSet, (void*)& camera);
  cli.Register("r", RegGet, (void*)& camera);
  cli.Register("roi", RoiSet, (void*)& roi);
  cli.Register("fft", FFT, (void*)& dfftt);
  cli.Register("save", SaveFrame, (void*)& save);
  cli.Register("temp", Temperature, (void*)& camera);
  cli.Register("reset", ResetCamera, (void*)&camera);
  cli.Register("gain", CameraGain, (void*)&camera);
  cli.Register("blc", CameraBLC, (void*)&camera);
  cli.Register("help", CommandHelp, NULL);

  // Set up how the window will be drawn
  df.setViewport(sf::FloatRect(0, 0, 0.5, 0.75));
  dsw.setViewport(sf::FloatRect(0, 0, 0.5, 0.75));
  dfftt.setViewport(sf::FloatRect(0.5, 0, 0.5, 0.75));
  droi.setViewport(sf::FloatRect(0.5, 0, 0.5, 0.75));
  sf::Transform tcli;
  tcli.translate(0, 0.75* WIN_Y);
  sf::Transform tstats;
  tstats.translate(0.5 * WIN_X, 0.75 * WIN_Y);
  bool mouse_pressed = false;
  sf::Vector2f drag_start;
  sf::Vector2f drag_end;
  sf::Vector2i win_start;
  sf::Vector2i win_end;

  //// CR ADDITIONS FOR MULTICAM SWITCHING ////
  std::tuple<Rcam*, std::map<int, int>*, double>
    cameraSetupParams(&camera, &cameraPortMap, cameraExposureTime_s);
    // Parameters passed to CameraID cli function to switch between multiple connected cameras
  cli.Register("id", CameraID, (void*)& cameraSetupParams);

  std::tuple<double*, double*, double*, double*, double*> delayObjects(
    &currentUSDelay_s, &currentOverlap_s, &currentPulsewidth_s, &cameraFSINDelay_s, &currentAOMDelay_s);
  cli.Register("del", USDelay, (void*)& delayObjects);

  std::tuple<double*, double*, Rcam*, double*, double*, double*> overlapObjects(
    &currentOverlap_s, &currentPulsewidth_s, &camera, &currentUSDelay_s, &cameraFSINDelay_s, &currentAOMDelay_s);
  cli.Register("overlap", USOverlap, (void*)& overlapObjects);

  std::tuple<double*, double*, double*> octopusObjects(&currentAOMDelay_s, &currentOverlap_s, &currentPulsewidth_s);
  cli.Register("aom", toggleAOM, (void*)&octopusObjects);

  cli.Register("freq", setAOMFreq, (void*)&aomSettings);

  cli.Register("volt", setAOMVolt, (void*)&aomSettings);

  if (scanningSystem == ScanningSystem::CW or scanningSystem == ScanningSystem::CW_FF) {
    camera.SetStream(true);
  }

  camera.Start();

  if (OctopusManager) OctopusManager->EnableSystemChannels(true);
  //// END CR ADDITIONS FOR MULTICAM SWITCHING ////

  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::EventType::Closed) {
        window.close();
        break;
      }
      else if (event.type == sf::Event::TextEntered) {
        cli.Input((char)event.text.unicode);

        // deal with mouse events
      }
      else {
        sf::Vector2i pos = sf::Mouse::getPosition(window);
        int view = pos.x / (window.getSize().x / 2);
        sf::Vector2f rel = sf::Vector2f(
          pos.x % (window.getSize().x / 2) / (float)(window.getSize().x / 2),
          pos.y / (float)(window.getSize().y));

        // scroll event (zoom in to the relevant view)
        if (event.type == sf::Event::MouseWheelScrolled) {
          if (view == 0) {
            dsw.Zoom(rel, event.mouseWheelScroll.delta > 0 ? 1.5f : 0.66f);
            df.Zoom(rel, event.mouseWheelScroll.delta > 0 ? 1.5f : 0.66f);
          }
          else {
            droi.Zoom(rel, event.mouseWheelScroll.delta > 0 ? 1.5f : 0.66f);
            dfftt.Zoom(rel, event.mouseWheelScroll.delta > 0 ? 1.5f : 0.66f);
          }

          // drag event (select ROI / FFT window)
        }
        else if (event.type == sf::Event::MouseButtonPressed) {
          mouse_pressed = true;

          if (view == 0) {
            win_start = dsw.PixelPosition(rel);
            fftt.SubWindow2Point(0, 0, camera.GetConfig()->width,
              camera.GetConfig()->height);
          }
          else {
            drag_start = droi.FractionalPosition(rel);
            roi.Set(0, 0, 0);
          }

        }
        else if (event.type == sf::Event::MouseMoved && mouse_pressed) {

          if (view == 0) {
            win_end = dsw.PixelPosition(rel);
            fftt.SubWindow2Point(win_start.x,
              win_start.y,
              win_end.x,
              win_end.y);
          }
          else {
            drag_end = droi.FractionalPosition(rel);
            sf::Vector2f drag_diff = drag_start - drag_end;
            roi.Set(2.0 * (drag_start.x - 0.5),
              2.0 * (.5 - drag_start.y),
              2 * sqrt(drag_diff.x * drag_diff.x +
                drag_diff.y * drag_diff.y));
          }

        }
        else if (event.type == sf::Event::MouseButtonReleased) {
          mouse_pressed = false;
        }
      }
    }

    cli.Update();
    window.clear();
    window.draw(df);
    window.draw(dfftt);
    if (g_roi) window.draw(droi);
    window.draw(stats, tstats);
    window.draw(cli, tcli);
    window.draw(dsw);
    window.display();
  }

  // Close camera and stop sending electrical signals
  if (AmplitudeLaser) AmplitudeLaser->enableChannel(7, false);
  if (AmplitudeLaser) AmplitudeLaser->enableChannel(8, false);
  if (OctopusManager) OctopusManager->EnableSystemChannels(false);
  camera.Close();

  delete OctopusManager;

  // Save the current configuration
  {
    json jcfg;

    int x, y, r;
    roi.Get(&x, &y, &r);
    jcfg["ROI"]["x_c"] = x;
    jcfg["ROI"]["y_c"] = y;
    jcfg["ROI"]["r"] = r;

    bool compute_log;
    double min, max;
    dfftt.GetScale(&compute_log, &min, &max);
    jcfg["FFT"]["compute_log"] = compute_log;
    jcfg["FFT"]["min"] = min;
    jcfg["FFT"]["max"] = max;
    jcfg["resolution"]["width"] = cameraXResolution;
    jcfg["resolution"]["height"] = cameraYResolution;

    std::ofstream cfg(CFG_FNAME);
    cfg << std::setw(4) << jcfg << std::endl;
    cfg.close();
  }

  return 0;
}
