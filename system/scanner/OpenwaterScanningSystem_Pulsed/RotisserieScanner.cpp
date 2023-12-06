#include <iostream>
#include <thread>

#include <glog/logging.h>

#include "AmpStage.h"
#include "ConexStage.h"
#include "RotisserieScanner.h"

using json = nlohmann::json;

RotisserieScanner::RotisserieScanner(
    const json& systemParameters, Laser* laser, BerkeleyNucleonics* delays,
    ConexStage* xStage, ConexStage* yStage, ConexStage* zStage, AmpStage* rStage,
    USTx* ustx, CameraManager* cameras, OctopusManager* octopusManager)
    : Scanner(systemParameters, laser, delays, ustx, cameras, octopusManager),
      xStage_(xStage), yStage_(yStage), zStage_(zStage), rStage_(rStage) {
}

RotisserieScanner::~RotisserieScanner() {
  delete xStage_;
  delete yStage_;
  delete zStage_;
  delete rStage_;
}

bool RotisserieScanner::init() {
  const auto& hwParams = systemParameters_["hardwareParameters"];
  if (!initializeStage(xStage_, hwParams["xStageCOM"].get<int>())) return false;
  if (!initializeStage(yStage_, hwParams["yStageCOM"].get<int>())) return false;
  if (!initializeStage(zStage_, hwParams["zStageCOM"].get<int>())) return false;
  if (!initializeStage(rStage_, hwParams["rStageCOM"].get<int>())) return false;

  return Scanner::init();
}

bool RotisserieScanner::initializeStage(Stage* stage, int comPort) {
  if (!stage) return true;
  if (!stage->init(comPort)) return false;
  stage->resetController();  // AmpStage notes current position as "home".
  // TODO(jfs): Stages do not work without this. They re-reset in middle of homing.
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  stage->moveHome();
  return true;
}

bool RotisserieScanner::scan() {
  const auto& scanParameters = systemParameters_["scanParameters"];
  double xROICenter_mm = scanParameters["xROICenter_mm"].get<double>();
  double yROICenter_mm = scanParameters["yROICenter_mm"].get<double>();
  double zClosest_mm = scanParameters["zClosest_mm"].get<double>();
  Range xSteps(scanParameters, "xInit_mm", "xLength_mm", "xScanStepSize_mm"),
        ySteps(scanParameters, "yInit_mm", "yLength_mm", "yScanStepSize_mm"),
        zSteps(scanParameters, "zROIStart_mm", "zLength_mm", "zScanStepSize_mm");
  double additionalPauseTime_ms = scanParameters["additionalPauseTime_ms"].get<double>();

  // Rotation angle
  Range gammaSteps(scanParameters, "gammaInit_deg", "gammaAngle_deg", "gammaScanStepSize_deg");

  // USTx Scan Geometry
  Range azimuthSteps(scanParameters, "azimuthInit_mm", "azimuthLength_mm", "azimuthScanStepSize_mm"),
        axialSteps(scanParameters, "axialROIStart_mm", "axialLength_mm", "axialScanStepSize_mm");
  double numAxialSteps = axialSteps.values.size();

  // Timing info
  double laserClockPeriod_ms = systemParameters_["laserParameters"]["laserClockPeriod_ms"].get<double>();
  double frameGatePeriod_ms = double(numAxialSteps) / (1.0 / laserClockPeriod_ms);

  std::cout << "INFO: Ready to Begin Scan" << std::endl;

  if (!startScan()) {
    return false;
  }

  bool cancel = CheckForCancel();
  int sliceIdx = 0;

  for (int zi = 0; zi < zSteps.values.size() && !cancel; zi++) {
    double z = zSteps[zi];
    if (zStage_) zStage_->moveAbsolute(z);

    for (int yi = 0; yi < ySteps.values.size() && !cancel; yi++) {
      double y = ySteps[yi];
      if (yStage_) yStage_->moveAbsolute(y);

      for (int xi = 0; xi < xSteps.values.size() && !cancel; xi++) {
        double x = xSteps[xi];
        if (xStage_) xStage_->moveAbsolute(x);

        for (int gammai = 0; gammai < gammaSteps.values.size() && !cancel; gammai++) {
          double gamma = gammaSteps[gammai];

          if (rStage_) rStage_->moveAbsolute(gamma);

          std::this_thread::sleep_for(std::chrono::milliseconds((long)additionalPauseTime_ms));

          if (ustx_) ustx_->SetFocus(0, true);  // Use incrementing

          for (int azi = 0; azi < azimuthSteps.values.size() && !cancel; azi++) {

            if (ustx_ && !cancel) {
              int focusIdx = ustx_->GetFocus();
              if (focusIdx != azi * numAxialSteps) {
                LOG(WARNING) << "Unexpected ustx focus index: " << focusIdx;
              }
            }

            int maxErrors = 4;
            int numErrors = 0;
            while (cameras_ && !cancel) {
              if (cameras_->captureAndWriteImagesAsync(numFociPerSlice_, sliceIdx, numAxialSteps, azi + 1, frameGatePeriod_ms) == 1) break;
              LOG(WARNING) << "Repeating axial column capture.";
              if (ustx_) ustx_->SetFocus(azi * numAxialSteps, true);  // Use incrementing
              cancel = CheckForCancel();
              numErrors++;
              if (numErrors > maxErrors) {
                cameras_->resetAllCamerasMidscan(sliceIdx * numFociPerSlice_ + azi * numAxialSteps);
                LOG(WARNING) << "Resetting all cameras due to excessive errors";
              }
            }

            // Check for thermal shutdown after each axial column
            if (ustx_ && ustx_->ThermalShutdown()) {
              ustx_->Reset();
              LOG(WARNING) << "USTX reset after thermal shutdown";
            }
          }

          // Check that full axial/azimuth slice completed and focus list reindexed to zero
          if (ustx_) {
            int focusIdx = ustx_->GetFocus();
            if (focusIdx != 0) {
              LOG(WARNING) << "Unexpected ustx focus index after slice: " << focusIdx;
            }
          }

          sliceIdx++;

          // Periodically check for cancel.
          cancel = CheckForCancel();
        }
      }
    }
  }

  // Move Stages back to centered and closest
  if (xStage_) xStage_->moveAbsolute(xROICenter_mm);
  if (yStage_) yStage_->moveAbsolute(yROICenter_mm);
  if (zStage_) zStage_->moveAbsolute(zClosest_mm);
  if (rStage_) rStage_->moveHome();  // Moves to absolute zero to get sample back centered, doesn't call stage homing command

  if (xStage_) xStage_->disableController();
  if (yStage_) yStage_->disableController();
  if (zStage_) zStage_->disableController();
  if (rStage_) rStage_->disableController();

  return endScan() && !cancel;
}
