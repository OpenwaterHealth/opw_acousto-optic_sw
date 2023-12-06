// saveImages.cpp 
// Caitlin Regan [Openwater]

#include <fstream>
#include <iostream>
#include <map>
#include <string>

#include <glog/logging.h>

#include "BloodflowCameraManager.h"
#include "bloodflowVoxelSave.h"
#include "OctopusManager_Bloodflow.h"

#include "system/component/inc/filterdev.h"
#include "system/component/inc/time.h"
#include "system/third_party/json-develop/single_include/nlohmann/json.hpp"
using json = nlohmann::json;

int main(int argc, char* argv[]) {
  std::string rootFolder(argv[1]); // Rootname of experiment [used to load json scan data info]
  // Import scan parameters from Python GUI [exe called from system/app/scanUI dir]
  std::cout << "saveImages.exe launched" << std::endl;
  std::ifstream scanMetadata("../../../data/localScanDataFiles/" + rootFolder + "/scan_metadata.json");
  json systemParameters = json::parse(scanMetadata);  // TODO (CR) check for failure / missing file
  scanMetadata.close();

  // Get info from metadata
  std::string syncedScanDataDir = systemParameters["fileParameters"]["syncedScanDataDir"].get<std::string>();
  std::string laser = systemParameters["laserParameters"]["laser"].get<std::string>();
  int numImages = systemParameters["cameraParameters"]["numImages"].get<int>();
  double frameAcquisitionRate_Hz = systemParameters["cameraParameters"]["frameAcquisitionRate_Hz"].get<double>();
  std::vector<std::string> pulseWidths_s = systemParameters["delayParameters"]["pulseWidths_s"].get<std::vector<std::string>>();
  std::string chEDelay_s = systemParameters["delayParameters"]["chEDelay_s"].get<std::string>();  // Pulse chopping AOM in pseudo-pulsed systems

  // Set up Cameras
  BloodflowCameraManager* bloodflowCameras = new BloodflowCameraManager();
  std::string bloodflowInfoSyncedDir = syncedScanDataDir + "/imageInfo.csv";
  bloodflowCameras->init(systemParameters);
  bloodflowCameras->setCSVFile(bloodflowInfoSyncedDir);
  bloodflowCameras->updatePulseWidth(pulseWidths_s[0]);
  bloodflowCameras->startAllCameras(true);
  Component::SleepMs(500);

  // Set up triggering
  OctopusManager_Bloodflow* OctopusManager = NULL;

  // Get octopus system parameters
  double mhz2hz = 1000000.0;
  double s2ms = 1000.0;
  double AOM4Freq_Hz = systemParameters["AOMParameters"]["AOM4Freq_MHz"].get<double>() * mhz2hz;
  double AOM4Volt_V = systemParameters["AOMParameters"]["AOM4Volt_V"].get<double>();
  double AOM3Freq_Hz = NULL;
  double AOM3Volt_V = NULL;
  double aom3ChopDelay_s = NULL;
  double postPulseDelay1 = 1e-6;  // Used to keep 1st AOM dark for dual laser system second pulse [negligible for single laser system]
  double postPulseDelay2 = 1e-6;  // Delay for after 2nd AOM pulse (must be non-zero)
  int hwTriggeredSystem = systemParameters["hardwareParameters"]["hwTrigger"].get<int>();  // Is system triggered by TTL button or sw trig
  int dualLaserSystem = systemParameters["hardwareParameters"]["dualLaserSystem"].get<int>();  // Is system a dual-laser system
  double aomChopDelay_s = std::stod(chEDelay_s) + (1.0 / frameAcquisitionRate_Hz);  // 1 dark image plus regular delay to chopper
  long dataCollectionTime_ms = (2.0 * (numImages+1)) / (frameAcquisitionRate_Hz)* s2ms + 1.0 * s2ms;  // Note: double numImages because alternating dark images

  if (laser == "Moglabs760_Gabor") {
    double AOM4Freq_Hz = NULL;
    double AOM4Volt_V = NULL;
    AOM3Freq_Hz = systemParameters["AOMParameters"]["AOM3Freq_MHz"].get<double>() * mhz2hz;
    AOM3Volt_V = systemParameters["AOMParameters"]["AOM3Volt_V"].get<double>();;
  }

  // Parameters for dual laser system
  if (dualLaserSystem) {
    AOM3Freq_Hz = systemParameters["AOMParameters"]["AOM3Freq_MHz"].get<double>() * mhz2hz;
    AOM3Volt_V = systemParameters["AOMParameters"]["AOM3Volt_V"].get<double>();
    aom3ChopDelay_s = std::stod(chEDelay_s) + (2.0 / frameAcquisitionRate_Hz);  // 1 dark image, 1 first laser image, regular delay to chopper
    dataCollectionTime_ms = (3.0 * (numImages+2)) / (frameAcquisitionRate_Hz)*s2ms + 1.0 * s2ms;  // Note: triple numImages because alternating dark images
    postPulseDelay1 = 1.0 / frameAcquisitionRate_Hz;  // Applied to 1st AOM chopper
  }

  // Set up octopus
  OctopusManager = new OctopusManager_Bloodflow();
  OctopusManager->init(systemParameters);
  OctopusManager->EnableFrameValid(true);
  Component::SleepMs(1000);
 
  bloodflowCameras->enableSave(true);

  for (int i = 0; i < pulseWidths_s.size(); i++) {
    bloodflowCameras->updatePulseWidth(pulseWidths_s[i]);

    if (dualLaserSystem) {
      OctopusManager->SetAOMPulsewidth(numImages, aom3ChopDelay_s, postPulseDelay2, std::stod(pulseWidths_s[i]), AOM3Freq_Hz, AOM3Volt_V, OctopusManager_Bloodflow::AOMChopper::SECOND_LASER, hwTriggeredSystem);
    }
    if (laser == "Moglabs760_Gabor") {
      OctopusManager->SetAOMPulsewidth(numImages, aomChopDelay_s, postPulseDelay1, std::stod(pulseWidths_s[i]), AOM3Freq_Hz, AOM3Volt_V, OctopusManager_Bloodflow::AOMChopper::SECOND_LASER, hwTriggeredSystem);
    } else {
      OctopusManager->SetAOMPulsewidth(numImages, aomChopDelay_s, postPulseDelay1, std::stod(pulseWidths_s[i]), AOM4Freq_Hz, AOM4Volt_V, OctopusManager_Bloodflow::AOMChopper::FIRST_LASER, hwTriggeredSystem);
    }

    // Acquire Data
    OctopusManager->ConfigureSystemChannels();
    OctopusManager->EnableSystemChannels(true);  // Note: this will start cameras
    Component::SleepMs(500);
    if (hwTriggeredSystem) {
      LOG(INFO) << "System ready for trigger";
      bloodflowCameras->waitDataCollection();
    } else {
      OctopusManager->TriggerDataCollection();
    }
    Component::SleepMs(dataCollectionTime_ms);
    bloodflowCameras->getFrameInfo();
    bloodflowCameras->setFrameCount(0);
    LOG(INFO) << pulseWidths_s[i] << "s collection complete";
  }

  // Shut down cameras and octopus
  bloodflowCameras->enableSave(false);
  OctopusManager->EnableSystemChannels(false);  // Note: this should stop cameras
  OctopusManager->EnableFrameValid(false);

  // Clean Up
  delete bloodflowCameras;
  delete OctopusManager;
  
  return 0;
}
