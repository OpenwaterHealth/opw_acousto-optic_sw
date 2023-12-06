#include <iostream>
#include <filesystem>

#include "system/component/inc/time.h"

#include "ustxManager.h"

using json = nlohmann::json;

ustxManager_USscanner::ustxManager_USscanner() {
  Serial* serial = new Serial();
  ustx_ = new USTx(serial);
}

ustxManager_USscanner::~ustxManager_USscanner() {
  ustx_->Close();
  delete ustx_;
}

bool ustxManager_USscanner::init(const json& systemParameters) {
  if (!ustx_) return true;

  std::string ultrasoundProbe = systemParameters["ultrasoundParameters"]["ultrasoundProbe"].get<std::string>();
  double ultrasoundFreq_Hz = systemParameters["ultrasoundParameters"]["ultrasoundFreq_MHz"].get<double>() * 1e6;
  int nCyclesUltrasound = systemParameters["ultrasoundParameters"]["nCyclesUltrasound"].get<int>();
  double elementPitch_m = systemParameters["ultrasoundParameters"]["elementPitch_um"].get<double>() * 1e-6;
  int numberElements = systemParameters["ultrasoundParameters"]["numberElements"].get<int>();
  double fNumber = systemParameters["ultrasoundParameters"]["fNumber"].get<double>();
  int usCOM = systemParameters["hardwareParameters"]["usCOM"].get<int>();

  std::string syncedScanDataDir_focusList = systemParameters["fileParameters"]["syncedScanDataDir"].get<std::string>() + "/ustxFocusList.hex";
  const char* const focusList = syncedScanDataDir_focusList.c_str();
  json ustxInitParams; // making json file from metadata to pass to the ustx
  ustxInitParams["frequency"] = ultrasoundFreq_Hz;
  ustxInitParams["f_number"] = fNumber;
  ustxInitParams["n_elements"] = numberElements;
  ustxInitParams["pitch"] = elementPitch_m;
  ustxInitParams["probe"] = ultrasoundProbe;
  if (ultrasoundProbe.at(0) == 'C') {
    double radiusOfCurvature_m = systemParameters["ultrasoundParameters"]["radiusOfCurvature_mm"].get<double>() * 1e-3;
    ustxInitParams["radius"] = radiusOfCurvature_m;
  }
  if (ultrasoundProbe.at(0) != 'L') {
    int maxElements = systemParameters["ultrasoundParameters"]["maxElements"].get<int>();
    ustxInitParams["max_elements"] = maxElements;
  }

  if (nCyclesUltrasound <= 128) {
    int repeats = 1;
    int waveforms = 1;
    if (nCyclesUltrasound > 96) {
      waveforms = 4;
    }
    else if (nCyclesUltrasound > 32 && nCyclesUltrasound <= 64) {
      if (nCyclesUltrasound % 2 == 0) {
        waveforms = 2;
      }
      else {
        waveforms = 3;
      }
    }
    else if (nCyclesUltrasound > 64 && nCyclesUltrasound <= 96) {
      if (nCyclesUltrasound % 3 == 0) {
        waveforms = 3;
      }
      else if (nCyclesUltrasound % 4 == 0) {
        waveforms = 4;
      }
      else if (nCyclesUltrasound % 2 == 0) {
        waveforms = 3;
      }
      else {
        waveforms = 4;
      }
    }
    repeats = (int)round(static_cast<double>(nCyclesUltrasound) / waveforms);
    ustxInitParams["waveforms"] = waveforms;
    ustxInitParams["repeat_count"] = repeats;
    std::cout << "The sctual number of cycles (nCycles): " << waveforms * repeats << std::endl;
  }
  else {
    double repeatTime_s = nCyclesUltrasound * (1.0 / ultrasoundFreq_Hz);
    ustxInitParams["repeat_time"] = repeatTime_s;
  }

  // Connect to COM port
  if (int err = ustx_->Open(usCOM)) {
    std::cout << "Can't open USTx serial port. (err = " << err << ")." << std::endl;
    return false;
  }
  // Load parameters
  if (int err = ustx_->LoadJson(ustxInitParams)) {
    std::cout << "Can't load USTx JSON parameters. (err = " << err << ")." << std::endl;
    return false;
  }

  const auto& scanParameters = systemParameters["scanParameters"];
  Range azimuthSteps(scanParameters, "azimuthInit_mm", "azimuthLength_mm", "azimuthScanStepSize_mm"),
    axialSteps(scanParameters, "axialROIStart_mm", "axialLength_mm", "axialScanStepSize_mm");
  const double mm2m = 0.001;
 
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
    std::cout << "USTx focus list not achievable." << std::endl;
    return false;
  }
  
  if (int result = ustx_->DownloadHex(focusList)) {
    std::cout << "USTx::DownloadHex failed; err = " << result << std::endl;
    return false;
  }

  return true;
}

void ustxManager_USscanner::resetFocus() {
  if (ustx_) ustx_->SetFocus(0, true);
  return;
}


void ustxManager_USscanner::isThermalShutdown() {
  if (ustx_ && ustx_->ThermalShutdown()) {
    ustx_->Reset();
    std::cout << "USTX reset after thermal shutdown" << std::endl;
  }
  return;
}


