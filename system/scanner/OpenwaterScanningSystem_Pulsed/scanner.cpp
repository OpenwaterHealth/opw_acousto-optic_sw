#if defined(_MSC_VER) || defined(__APPLE__)
#include <filesystem>
namespace fs = std::filesystem;
#else  // linux
#include <experimental/filesystem>  // bloody hell
namespace fs = std::experimental::filesystem;
#endif

#include <iostream>
#include <thread>

#include <glog/logging.h>

#include "system/component/inc/time.h"

#include "OctopusManager.h"
#include "scanner.h"
#include "VoxelData.h"

Scanner::Scanner(
    const json& systemParameters, Laser* laser, BerkeleyNucleonics* delays,
    USTx* ustx, CameraManager* cameras, OctopusManager* octopusManager)
    : systemParameters_(systemParameters), laser_(laser), delayGenerator_(delays),
      ustx_(ustx), cameras_(cameras), octopusManager_(octopusManager) {
}

Scanner::~Scanner() {
  delete laser_;
  delete delayGenerator_;
  delete cameras_;
  delete octopusManager_;
  delete ustx_;
}

bool Scanner::init() {
  if (!initializeUSTx()) {
    return false;
  }

  if (cameras_ && !cameras_->init(systemParameters_, this)) {
    return false;
  }

  if (laser_ && !laser_->initializeLaser(systemParameters_)) {
    LOG(ERROR) << "Laser init failed.";
    return false;
  }

  if (octopusManager_ && !octopusManager_->init(systemParameters_, numAxialFoci_, this)) {
    return false;
  }

  if (delayGenerator_ && !initializeDelayGenerator()) {
    return false;
  }

  return true;
}

bool Scanner::initializeUSTx() {
  if (!ustx_) return true;

  int pulsedSystem = systemParameters_["laserParameters"]["pulsed"].get<int>();
  std::string laser = systemParameters_["laserParameters"]["laser"].get<std::string>();
  std::string ultrasoundProbe = systemParameters_["ultrasoundParameters"]["ultrasoundProbe"].get<std::string>();
  double ultrasoundFreq_Hz = systemParameters_["ultrasoundParameters"]["ultrasoundFreq_MHz"].get<double>() * 1e6;
  int nCyclesUltrasound = systemParameters_["ultrasoundParameters"]["nCyclesUltrasound"].get<int>();
  double elementPitch_m = systemParameters_["ultrasoundParameters"]["elementPitch_um"].get<double>() * 1e-6;
  int numberElements = systemParameters_["ultrasoundParameters"]["numberElements"].get<int>();
  double fNumber = systemParameters_["ultrasoundParameters"]["fNumber"].get<double>();
  double chBDelay_s = stod(systemParameters_["delayParameters"]["chBDelay_s"].get<std::string>());
  int usCOM = systemParameters_["hardwareParameters"]["usCOM"].get<int>();

  std::string syncedScanDataDir_focusList = systemParameters_["fileParameters"]["syncedScanDataDir"].get<std::string>() + "/ustxFocusList.hex";
  const char* const focusList = syncedScanDataDir_focusList.c_str();
  json ustxInitParams;
  ustxInitParams["frequency"] = ultrasoundFreq_Hz;
  ustxInitParams["f_number"] = fNumber;
  ustxInitParams["n_elements"] = numberElements;
  ustxInitParams["pitch"] = elementPitch_m;
  ustxInitParams["probe"] = ultrasoundProbe;
  if (ultrasoundProbe.at(0) == 'C') {
    double radiusOfCurvature_m = systemParameters_["ultrasoundParameters"]["radiusOfCurvature_mm"].get<double>() * 1e-3;
    ustxInitParams["radius"] = radiusOfCurvature_m;
  }
  if (ultrasoundProbe.at(0) != 'L') {
    int maxElements = systemParameters_["ultrasoundParameters"]["maxElements"].get<int>();
    ustxInitParams["max_elements"] = maxElements;
  }

  if (nCyclesUltrasound <= 128) {
    int repeats = 1;
    int waveforms = 1;
    if (nCyclesUltrasound > 96) {
      waveforms = 4;
    } else if (nCyclesUltrasound > 32 && nCyclesUltrasound <= 64) {
      if (nCyclesUltrasound % 2 == 0) {
        waveforms = 2;
      } else {
        waveforms = 3;
      }
    } else if (nCyclesUltrasound > 64 && nCyclesUltrasound <= 96) {
      if (nCyclesUltrasound % 3 == 0) {
        waveforms = 3;
      } else if (nCyclesUltrasound % 4 == 0) {
        waveforms = 4;
      } else if (nCyclesUltrasound % 2 == 0) {
        waveforms = 3;
      } else {
        waveforms = 4;
      }
    }
    repeats = (int)round(static_cast<double>(nCyclesUltrasound) / waveforms);
    ustxInitParams["waveforms"] = waveforms;
    ustxInitParams["repeat_count"] = repeats;
    systemParameters_["ultrasoundParameters"]["nCyclesUltrasound"] = waveforms * repeats;
  } else {
    double repeatTime_s = nCyclesUltrasound * (1.0 / ultrasoundFreq_Hz);
    ustxInitParams["repeat_time"] = repeatTime_s;
  }

  // Connect to COM port
  if (int err = ustx_->Open(usCOM)) {
    LOG(ERROR) << "Can't open USTx serial port. (err = " << err << ").";
    return false;
  }
  // Load parameters
  if (int err = ustx_->LoadJson(ustxInitParams)) {
    LOG(ERROR) << "Can't load USTx JSON parameters. (err = " << err << ").";
    return false;
  }

  const auto& scanParameters = systemParameters_["scanParameters"];
  Range azimuthSteps(scanParameters, "azimuthInit_mm", "azimuthLength_mm", "azimuthScanStepSize_mm"),
        axialSteps(scanParameters, "axialROIStart_mm", "axialLength_mm", "axialScanStepSize_mm");
  const double mm2m = 0.001;
  numFociPerSlice_ = int(azimuthSteps.values.size() * axialSteps.values.size());
  numAxialFoci_ = int(axialSteps.values.size());

  for (int azi = 0, focusIndex = 0; azi < azimuthSteps.values.size(); azi++) {
    for (int axi = 0; axi < axialSteps.values.size(); axi++, focusIndex++) {
      ustx_->AddFocus(azimuthSteps[azi] * mm2m, axialSteps[axi] * mm2m);
    }
  }

  if (!ustx_->ComputeFoci(focusList)) {
    return false;
  }

  double ustxDelay = ustx_->GetDelay();
  if (ustxDelay < 0) {
    LOG(ERROR) << "USTx focus list not achievable.";
    return false;
  }
  chBDelay_s = chBDelay_s - ustxDelay;
  systemParameters_["delayParameters"]["chBDelay_s"] = std::to_string(chBDelay_s);

  if (int result = ustx_->DownloadHex(focusList)) {
    LOG(ERROR) << "USTx::DownloadHex failed; err = " << result;
    return false;
  }

  return true;
}

bool Scanner::initializeDelayGenerator() {
  if (!delayGenerator_) return true;
  std::string laser = systemParameters_["laserParameters"]["laser"].get<std::string>();
  int pseudoPulsedSystem = systemParameters_["laserParameters"]["pseudoPulsed"].get<int>();
  std::string ultrasoundAmp = systemParameters_["ultrasoundParameters"]["ultrasoundAmp"].get<std::string>();
  int bncCOM = systemParameters_["hardwareParameters"]["bncCOM"].get<int>();
  const auto& delayParameters = systemParameters_["delayParameters"];
  std::string chADelay_s = delayParameters["chADelay_s"].get<std::string>();
  std::string chAWidth_s = delayParameters["chAWidth_s"].get<std::string>();
  std::string chBDelay_s = delayParameters["chBDelay_s"].get<std::string>();
  std::string chBWidth_s = delayParameters["chBWidth_s"].get<std::string>();
  std::string chCDelay_s = delayParameters["chCDelay_s"].get<std::string>();
  std::string chCWidth_s = delayParameters["chCWidth_s"].get<std::string>();
  std::string chDDelay_s = delayParameters["chDDelay_s"].get<std::string>();
  std::string chDWidth_s = delayParameters["chDWidth_s"].get<std::string>();
  std::string chEDelay_s = delayParameters["chEDelay_s"].get<std::string>();
  std::string chEWidth_s = delayParameters["chEWidth_s"].get<std::string>();
  std::string chFDelay_s = delayParameters["chFDelay_s"].get<std::string>();
  std::string chFWidth_s = delayParameters["chFWidth_s"].get<std::string>();

  if (!delayGenerator_->init(bncCOM)) {
    LOG(ERROR) << "Error initializing BNC box";
    return false;
  }
  PulseMode BNCPulseType = SINGLE;
  bool unitTRIG = true;
  bool unitON = true;
  bool channelON = true;
  bool channelOFF = false;
  bool gateON = true;
  delayGenerator_->resetUnit();
  Component::SleepMs(1000);  // Magic sleep for reset to complete

  if (laser.compare("Fake") == 0) {
    double ms2s = 0.001;
    double laserClockPeriod_ms = systemParameters_["laserParameters"]["laserClockPeriod_ms"].get<double>();
    std::string QCchGDelay_s = delayParameters["QCchGDelay_s"].get<std::string>();
    std::string QCchGWidth_s = delayParameters["QCchGWidth_s"].get<std::string>();
    std::string QCchHDelay_s = delayParameters["QCchHDelay_s"].get<std::string>();
    std::string QCchHWidth_s = delayParameters["QCchHWidth_s"].get<std::string>();
    BNCPulseType = CONTINUOUS;
    unitTRIG = false;
    delayGenerator_->setUnitPulseRate(1.0 / (laserClockPeriod_ms * ms2s));
    delayGenerator_->setPulseDelay(1, QCchGDelay_s);  // Mimic signal coming from Quantum Composers Ch G [FSIN]
    delayGenerator_->setPulseWidth(1, QCchGWidth_s);
    delayGenerator_->setPulseDelay(6, QCchHDelay_s);  // Mimic signal coming from Quantum Composers Ch H [laser clock]
    delayGenerator_->setPulseWidth(6, QCchHWidth_s);
    delayGenerator_->enableChannel(1, channelON);
    delayGenerator_->enableChannel(2, channelOFF);
    delayGenerator_->enableChannel(3, channelOFF);
    delayGenerator_->enableChannel(4, channelOFF);
    delayGenerator_->enableChannel(5, channelOFF);
    delayGenerator_->enableChannel(6, channelON);
  } else {
    LOG(ERROR) << "BNC Box only supported for fake scanner";
  }

  delayGenerator_->enableUnit(unitON);
  delayGenerator_->displayUpdate();
  return true;
}

// Implement Trigger: Trigger the signal capture sequence for a voxel.
void Scanner::triggerAcquisition() {
  int octopusSystem = systemParameters_["hardwareParameters"]["octopus"].get<int>();
  if (octopusSystem) {
    octopusManager_->TriggerVoxel();
  } else {
    LOG(ERROR) << "Non-octopus triggering no longer supported";
  }
}

bool Scanner::startScan() {
  // Enable cameras to start collecting frames.
  if (cameras_) cameras_->startAllCameras();
  if (octopusManager_) octopusManager_->EnableSystemChannels(true);
  // Open Verdi shutter
  std::string laserModel = systemParameters_["laserParameters"]["laser"].get<std::string>();
  if (laser_ && laserModel == "Verdi") {
    if (!(laser_->enableChannel(1, true))) {
      LOG(ERROR) << "Error opening Verdi laser shutter";
      return false;
    }
  }

  // Open files to save imageInfo Data.
  std::string imageInfoFilename = "/imageInfoAsync.csv"; // TODO(CR): change back to imageInfo once buffering
  std::string repeatedVoxelLogFilename = "/repeatedVoxelLog.csv";
  const auto& fileParams = systemParameters_["fileParameters"];
  std::string localScanDataDir_imageInfo =
    fileParams["localScanDataDir"].get<std::string>() + imageInfoFilename;
  std::string syncedScanDataDir_imageInfo =
    fileParams["syncedScanDataDir"].get<std::string>() + imageInfoFilename;
  std::string syncedScanDataDir_repeatedVoxelLog =
    fileParams["syncedScanDataDir"].get<std::string>() + repeatedVoxelLogFilename;
  imageInfoSynced_.open(syncedScanDataDir_imageInfo, std::ofstream::out | std::ofstream::app);
  imageInfoLocal_.open(localScanDataDir_imageInfo, std::ofstream::out | std::ofstream::app);
  repeatedVoxelLog_.open(syncedScanDataDir_repeatedVoxelLog, std::ofstream::out | std::ofstream::app);
  if (cameras_) {
    cameras_->setImageInfoStream(&imageInfoLocal_, true);
    cameras_->setImageInfoStream(&imageInfoSynced_, false);
    cameras_->setRepeatedVoxelLogStream(&repeatedVoxelLog_);
  }
  imageInfoLocal_ << CSVHeader;
  imageInfoSynced_ << CSVHeader;

  return true;
}

bool Scanner::endScan() {
  if (cameras_) cameras_->endExecNodes();

  if (delayGenerator_) delayGenerator_->enableUnit(false);

  if (octopusManager_) octopusManager_->EnableSystemChannels(false);

  std::string laserModel = systemParameters_["laserParameters"]["laser"].get<std::string>();
  if (laser_) {
    if (laserModel == "Verdi") {
      laser_->enableChannel(1, false); // Close Verdi shutter
    } else {
      laser_->enableChannel(7, false);  // Turn off FSIN trigger
      laser_->enableChannel(8, false);  // Turn off octopus trigger input
    }
  }

  if (ustx_) ustx_->Close();

  imageInfoLocal_.close();
  imageInfoSynced_.close();
  repeatedVoxelLog_.close();

  return true;
}

bool Scanner::CheckForCancel() {
  time_t now = Component::SteadyClockTimeMs();
  if (now - lastCancelCheckTime_ > 3000) {
    std::string cancelFile =
      systemParameters_["fileParameters"]["localScanDataDir"].get<std::string>() + "/cancel";
    if (fs::exists(cancelFile)) {
      LOG(INFO) << "CANCEL: cleaning up.";
      return true;
    }
    lastCancelCheckTime_ = now;
  }
  return false;
}
