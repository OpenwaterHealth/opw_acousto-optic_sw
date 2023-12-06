#include <iostream>
#include <thread>

#include <glog/logging.h>

#include "RobotScanner.h"
#include "robot.h"

using json = nlohmann::json;

RobotScanner::RobotScanner(
    const json& systemParameters, Laser* laser, BerkeleyNucleonics* delays,
    Robot* robot, USTx* ustx, CameraManager* cameras, OctopusManager* octopusManager)
    : Scanner(systemParameters, laser, delays, ustx, cameras,
        octopusManager), robot_(robot) {
}

RobotScanner::~RobotScanner() {
  delete robot_;
}

bool RobotScanner::init() {
  if (robot_) {
    if (!robot_->init()) {
      return false;
    }
    if (!robot_->activate(true)) {
      return false;
    }
    if (!robot_->home()) {
      return false;
    }

    // Set joints to known location (centered, far enough back to be safe for phantom)
    const auto& joints = systemParameters_["hardwareParameters"]["robotInitJoints"];
    robot_->setInitJoints(joints[0], joints[1], joints[2], joints[3], joints[4], joints[5]);
    robot_->moveJoints(joints[0], joints[1], joints[2], joints[3], joints[4], joints[5]);

    // Set the location of the Tool Reference Frame (TRF) relative to the Flange Reference Frame (at
    // the last joint of the robot).
    const auto& trf = systemParameters_["hardwareParameters"]["robotTRF"];
    robot_->setTRF(trf[0], trf[1], trf[2], trf[3], trf[4], trf[5]);  // set TRF to be centered with source/detector in SD plane
  }

  return Scanner::init();
}

bool RobotScanner::scan() {
  const auto& scanParameters = systemParameters_["scanParameters"];

  // Get current position of robot TRF, all scan movements will be relative to this position
  std::vector<double> initialPosition = robot_->getPose();

  // Robot translations
  double xROICenter_mm = scanParameters["xROICenter_mm"].get<double>();
  double yROICenter_mm = scanParameters["yROICenter_mm"].get<double>();
  double zClosest_mm = scanParameters["zClosest_mm"].get<double>();
  Range xSteps(scanParameters, "xInit_mm", "xLength_mm", "xScanStepSize_mm"),
        ySteps(scanParameters, "yInit_mm", "yLength_mm", "yScanStepSize_mm"),
        zSteps(scanParameters, "zROIStart_mm", "zLength_mm", "zScanStepSize_mm");
  double additionalPauseTime_ms = scanParameters["additionalPauseTime_ms"].get<double>();

  // Robot rotation angles
  Range alphaSteps(scanParameters, "alphaInit_deg", "alphaAngle_deg", "alphaScanStepSize_deg"),
        betaSteps(scanParameters, "betaInit_deg", "betaAngle_deg", "betaScanStepSize_deg"),
        gammaSteps(scanParameters, "gammaInit_deg", "gammaAngle_deg", "gammaScanStepSize_deg");

  // USTx Scan Geometry
  Range azimuthSteps(scanParameters, "azimuthInit_mm", "azimuthLength_mm", "azimuthScanStepSize_mm"),
        axialSteps(scanParameters, "axialROIStart_mm", "axialLength_mm", "axialScanStepSize_mm");
  double numAxialSteps = axialSteps.values.size();

  // Timing info
  double laserClockPeriod_ms = systemParameters_["laserParameters"]["laserClockPeriod_ms"].get<double>();
  double frameGatePeriod_ms = double(numAxialSteps) / (1.0 / laserClockPeriod_ms);

  LOG(INFO) << "Ready to Begin Scan";

  if (!startScan()) {
    return false;
  }

  bool cancel = CheckForCancel();

  int sliceIdx = 0;  // Numbering index for hologram images

  ////////// STAGE SCAN //////////

  for (int alphai = 0; alphai < alphaSteps.values.size() && !cancel; alphai++) {
    double alpha = alphaSteps[alphai];

    for (int betai = 0; betai < betaSteps.values.size() && !cancel; betai++) {
      double beta = betaSteps[betai];

      for (int gammai = 0; gammai < gammaSteps.values.size() && !cancel; gammai++) {
        double gamma = gammaSteps[gammai];

        for (int zi = 0; zi < zSteps.values.size() && !cancel; zi++) {
          double z = zSteps[zi];

          for (int yi = 0; yi < ySteps.values.size() && !cancel; yi++) {
            double y = ySteps[yi];

            for (int xi = 0; xi < xSteps.values.size() && !cancel; xi++) {
              double x = xSteps[xi];

              // Reshuffle x,y,z for "scanner classic" directions; could rotate the TRF, but then
              // movement commands would have to be transformed. (MovePose args are relative to WRF)
              // Z closest is at trfX (opposite other systems, TODO change this?)
              // trfY adjusts right/left
              // trfZ adjusts up/down
              // trfBeta rotates around "up/down" axis +/-25 degrees (note: traditionally this was gamma, but due to
              // limits on the angular range, TRF coordinate frame is rotated to prevent motion errors)
              if (robot_ && !robot_->movePose(initialPosition[0] + z, initialPosition[1] + x, initialPosition[2] + y, 
                initialPosition[3] + alpha, initialPosition[4] + beta, initialPosition[5] + gamma)) {
                cancel = true;
                break;
              }

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
    }
  }

  // Move back to centered/closest
  if (robot_ && !robot_->movePose(initialPosition[0], initialPosition[1], initialPosition[2],
    initialPosition[3], initialPosition[4], initialPosition[5])) {
    cancel = true;
  }

  return endScan() && !cancel;
}
