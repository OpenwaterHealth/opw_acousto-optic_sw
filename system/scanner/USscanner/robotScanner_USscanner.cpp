#include <iostream>
#include <filesystem>

#include "system/component/inc/time.h"

#include "robotScanner_USscanner.h"


using json = nlohmann::json;


RobotScanner_USscanner::RobotScanner_USscanner() {
  robot_ = new Robot();
}

RobotScanner_USscanner::~RobotScanner_USscanner() {
  delete robot_;
}

bool RobotScanner_USscanner::init(const json& systemParameters) {
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
    const auto& joints = systemParameters["hardwareParameters"]["robotInitJoints"];
    robot_->setInitJoints(joints[0], joints[1], joints[2], joints[3], joints[4], joints[5]);
    robot_->moveJoints(joints[0], joints[1], joints[2], joints[3], joints[4], joints[5]);

    // Set the location of the Tool Reference Frame (TRF) relative to the Flange Reference Frame (at
    // the last joint of the robot).
    const auto& trf = systemParameters["hardwareParameters"]["robotTRF"];
    robot_->setTRF(trf[0], trf[1], trf[2], trf[3], trf[4], trf[5]);  // set TRF to be centered with source/detector in SD plane
  }

  return true;
}

bool RobotScanner_USscanner::scan(const json& systemParameters, ustxManager_USscanner* ustxManager, OctopusManager_USscanner* octopusManager) {
  const auto& scanParameters = systemParameters["scanParameters"];

  // Get current position of robot TRF, all scan movements will be relative to this position
  std::vector<double> initialPosition = robot_->getPose();

  // Robot translations
  double yROICenter_mm = scanParameters["yROICenter_mm"].get<double>();
  Range ySteps(scanParameters, "yInit_mm", "yLength_mm", "yScanStepSize_mm");
  double additionalPauseTime_ms = scanParameters["additionalPauseTime_ms"].get<double>();

  // USTx Scan Geometry
  Range azimuthSteps(scanParameters, "azimuthInit_mm", "azimuthLength_mm", "azimuthScanStepSize_mm"),
    axialSteps(scanParameters, "axialROIStart_mm", "axialLength_mm", "axialScanStepSize_mm");
  double numAxialSteps = axialSteps.values.size();
  double numAzimuthSteps = azimuthSteps.values.size();

  // Robot joint 6 rotation angle 
  Range joint6Steps(scanParameters, "joint6Start_deg", "joint6Angle_deg", "joint6StepSize_deg");

  // Timing info
  double ustxTriggerPeriod_s = systemParameters["ultrasoundParameters"]["ustxTriggerPeriod_s"].get<double>();
  double frameGatePeriod_ms = double(numAxialSteps * numAzimuthSteps) / (1.0 / (ustxTriggerPeriod_s * 1000));

  bool cancel = CheckForCancel(systemParameters);

  ////////// STAGE SCAN /////////

  for (int yi = 0; yi < ySteps.values.size() && !cancel; yi++) {
    double y = ySteps[yi];

      if (robot_ && !robot_->movePose(initialPosition[0], initialPosition[1], initialPosition[2] + y,
        initialPosition[3], initialPosition[4], initialPosition[5])) {
        cancel = true;
        break;
      }

      for (int j6i = 0; j6i < joint6Steps.values.size() && !cancel; j6i++) {
        if (robot_) {
          robot_->getJoints();  // Update joint locations before moving joint 6
        }
        if (robot_ && !robot_->moveJointSix(joint6Steps[j6i])) {
          cancel = true;
          break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds((long)additionalPauseTime_ms));

        if (ustxManager) ustxManager->resetFocus();  // Use incrementing

        if (octopusManager && !cancel) {
          octopusManager->EnableUSTxTimer(true);
          octopusManager->Trigger();
        }
        else {
          std::cout << "Octopus required to trigger acquisition" << std::endl;
          return false;
        }
        Component::SleepMs(frameGatePeriod_ms); // numaxial * num azi/ ....


        // Check for thermal shutdown after each axial column
        if (ustxManager) {
          ustxManager->isThermalShutdown();
        }

        // Periodically check for cancel.
        cancel = CheckForCancel(systemParameters);
      }
      
    // Move back to centered/closest
    if (robot_ && !robot_->movePose(initialPosition[0], initialPosition[1], initialPosition[2],
      initialPosition[3], initialPosition[4], initialPosition[5])) {
      cancel = true;
    }
  }
  return !cancel;
}

bool RobotScanner_USscanner::CheckForCancel(const json& systemParameters) {
  time_t now = Component::SteadyClockTimeMs();
  if (now - lastCancelCheckTime_ > 3000) {
    std::string cancelFile =
      systemParameters["fileParameters"]["localScanDataDir"].get<std::string>() + "/cancel";
    if (std::filesystem::exists(cancelFile)) {
      std::cout << "CANCEL: cleaning up." << std::endl;
      return true;
    }
    lastCancelCheckTime_ = now;
  }
  return false;
}
