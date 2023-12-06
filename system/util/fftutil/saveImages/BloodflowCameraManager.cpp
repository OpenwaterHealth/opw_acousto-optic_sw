#include <cstdint>
#include <iomanip>
#include <iostream>
#include <time.h>

#include "system/component/inc/frame.h"
#include "system/component/inc/rcam.h"
#include "system/component/inc/stddev.h"
#include "system/component/inc/fx3.h"
#include "system/component/inc/time.h"

#include "BloodflowCameraManager.h"
#include "bloodflowVoxelData.h"
#include "bloodflowVoxelSave.h"

using json = nlohmann::json;

BloodflowCameraManager::~BloodflowCameraManager() {
  for (json::iterator id = cameraIDNumbers_.begin(); id != cameraIDNumbers_.end(); id++) {
    int cameraID = std::stoi(id.key());
    while (!cameraInfoMap_[cameraID].bloodflowVoxelSave->IsExecDone()) {
      Component::SleepMs(1000);  // Wait for processing to complete before deleting any exec nodes
    }
    this->enableSave(false);
    delete cameraInfoMap_[cameraID].camera; // Clean up camera objects
    delete cameraInfoMap_[cameraID].filterdev;
    delete cameraInfoMap_[cameraID].stdDev;
    delete cameraInfoMap_[cameraID].bloodflowVoxelSave;
  }
}

bool BloodflowCameraManager::init(const json& systemParameters) {
  // TODO (CR): check for stupid inputs (like bad ROI defaults)
  std::string syncedRawImageDir = systemParameters["fileParameters"]["syncedRawImageDir"].get<std::string>();
  std::string config_dir = systemParameters["fileParameters"]["config_dir"];
  double exposureTime_s = systemParameters["cameraParameters"]["exposureTime_ms"].get<double>() * 0.001;
  int numImages = systemParameters["cameraParameters"]["numImages"].get<int>();
  int resolutionX = systemParameters["cameraParameters"]["resolutionX"].get<int>();
  int resolutionY = systemParameters["cameraParameters"]["resolutionY"].get<int>();
  int initXpixel = systemParameters["cameraParameters"]["initXpixel"].get<int>();
  int initYpixel = systemParameters["cameraParameters"]["initYpixel"].get<int>();
  bool saveImages = systemParameters["cameraParameters"]["saveImages"].get<bool>();
  bool blackLevelCompensation = systemParameters["cameraParameters"]["blackLevelCompensation"].get<bool>();
  bool obPixels = systemParameters["cameraParameters"]["obPixels"].get<bool>();  // Enable optically black pixels
    // Note, currently doesn't change frame size, so you lose bottom 20 rows
  const std::string cameraGains = systemParameters["cameraParameters"]["cameraGains"].dump();
  const json cameraGains_JSON = json::parse(cameraGains);  // key: (int) ID number, Value: gain
  bool filterSpeckle = systemParameters["cameraParameters"]["filterSpeckle"].get<bool>();
  std::string cameraIDNumbersStr = systemParameters["cameraParameters"]["cameraIDNumbers"].dump();
  cameraIDNumbers_ = json::parse(cameraIDNumbersStr);  // Key: (int) ID number, Value: location information string

  // Set up cameras and image processing pipeline
  int numCameras = Rcam::NumCameras();
  for (int i = 0; i < numCameras; i++) {
    Rcam* tempCamera = new Rcam();
    if (tempCamera->Open(i) != 0) return false;  // error msgs will come from Open()
    int cameraID = tempCamera->SerialNumber();  // serial number now returns cameraID
    std::cout << "INFO: Connected to Camera: " << cameraID << std::endl;

    // Store camera information
    cameraInfo& cameraInfo = cameraInfoMap_[cameraID];
    cameraInfo.cameraID = cameraID;
    cameraInfo.camera = tempCamera;
    cameraInfo.portNumber = i;
    if (filterSpeckle) {
      cameraInfo.filterdev = new FilterDev();
    } else {
      cameraInfo.filterdev = NULL;
    }
    cameraInfo.stdDev = new StdDev();
    cameraInfo.bloodflowVoxelSave = new BloodflowVoxelSave(this);

    cameraInfo.camera->SetExposure(exposureTime_s);
    cameraInfo.camera->SetBLC(blackLevelCompensation);  // Turn on or off black level compensation

    //cameraInfo.camera->SubWindow2Point(initXpixel, initYpixel, resolutionX - initXpixel, resolutionY - initYpixel);  TODO(CR): fix fw(?) so this works
    cameraInfo.camera->SetGain(cameraGains_JSON[std::to_string(cameraID)].get<int>());
    cameraInfo.camera->SetOBPixels(obPixels);  // Enable or disable optically black pixels
    cameraInfo.camera->SubWindow2Point(0, 0, resolutionX, resolutionY);

    // Set up processing chain
    // Initialize FFT processing chain
    cameraInfo.stdDev->AddProducer(cameraInfo.camera);  // Process mean and regular speckle contrast before offset applied
    if (filterSpeckle) {
      cameraInfo.filterdev->Init(resolutionX, resolutionY);
      std::string flatfieldFilename = config_dir + "flatfield_" + std::to_string(cameraInfo.cameraID) + ".tiff";
      std::string badpixelFilename = config_dir + "badPixelMask_" + std::to_string(cameraInfo.cameraID) + ".tiff";
      cameraInfo.filterdev->LoadFlatfieldImage(flatfieldFilename.c_str());
      cameraInfo.filterdev->LoadBadPixelImage(badpixelFilename.c_str());
      cameraInfo.filterdev->AddProducer(cameraInfo.stdDev);
      cameraInfo.bloodflowVoxelSave->AddProducer(cameraInfo.filterdev);
    } else {
      cameraInfo.bloodflowVoxelSave->AddProducer(cameraInfo.stdDev);
    }
    if (saveImages) {
      cameraInfo.bloodflowVoxelSave->setFilename(syncedRawImageDir + "/camera" + std::to_string(cameraID) + "/image");
    }
    cameraInfo.camera->resize(30);
  }
  return true;
}

void BloodflowCameraManager::writeBloodflowVoxelData(const bloodflowVoxelData& voxelData) {
  std::lock_guard<std::mutex> lock(bloodflowVoxelDataMutex_);

  bloodflowInfoSynced_ << voxelData.imageName << "," << voxelData.cameraID << "," << voxelData.timestamp << "," <<
   voxelData.imageMean << "," << voxelData.imageStd << "," << voxelData.speckleContrast << "," 
    << voxelData.filteredImageMean << "," << voxelData.filteredImageStd << "," << voxelData.filteredSpeckleContrast << "," 
    << voxelData.temperature << "," << voxelData.saturatedPixels << "," << voxelData.pulseWidth_s << std::endl;
}

void BloodflowCameraManager::startAllCameras(bool cameraON) {
  for (json::iterator id = cameraIDNumbers_.begin(); id != cameraIDNumbers_.end(); id++) {
    int cameraID = std::stoi(id.key());
    if (cameraON) {
      cameraInfoMap_[cameraID].camera->Start();
    } else {
      cameraInfoMap_[cameraID].camera->Stop();
    }
  }
}

bool BloodflowCameraManager::setCSVFile(const std::string& csvFilePath) {
  bloodflowInfoSynced_.open(csvFilePath, std::ofstream::out | std::ofstream::app);
  if (bloodflowInfoSynced_.is_open()) {
    bloodflowInfoSynced_ << bloodflowHeader << std::endl;
    return true;
  } else {
    std::cout << "ERROR: Failed to open " << csvFilePath << std::endl;
    return false;
  }
}

void BloodflowCameraManager::getFrameInfo() {
  for (json::iterator id = cameraIDNumbers_.begin(); id != cameraIDNumbers_.end(); id++) {
    int cameraID = std::stoi(id.key());
    std::cout << "INFO: camera performance stats for camera " << cameraID << std::endl;
    std::cout << "Received Frames: " << cameraInfoMap_[cameraID].camera->GetFrameCount() << std::endl;
    std::cout << "Dropped Frames: " << cameraInfoMap_[cameraID].camera->DroppedFrames() << std::endl;
  }
}

void BloodflowCameraManager::waitDataCollection() {
  for (json::iterator id = cameraIDNumbers_.begin(); id != cameraIDNumbers_.end(); id++) {
    int cameraID = std::stoi(id.key());
    while (cameraInfoMap_[cameraID].camera->GetFrameCount() < 1) {
      Component::SleepMs(1000);
      std::cout << "Waiting for frames" << std::endl;
    }
  }
}

void BloodflowCameraManager::updatePulseWidth(const std::string& pulseWidth_s) {
  for (json::iterator id = cameraIDNumbers_.begin(); id != cameraIDNumbers_.end(); id++) {
    int cameraID = std::stoi(id.key());
    cameraInfoMap_[cameraID].bloodflowVoxelSave->setPulseWidth(pulseWidth_s);
  }
}

void BloodflowCameraManager::enableSave(bool saveON) {
  for (json::iterator id = cameraIDNumbers_.begin(); id != cameraIDNumbers_.end(); id++) {
    int cameraID = std::stoi(id.key());
    cameraInfoMap_[cameraID].bloodflowVoxelSave->enableSave(saveON);
    cameraInfoMap_[cameraID].camera->EnableSave(saveON);
  }
}

void BloodflowCameraManager::setFrameCount(int frameCount) {
  for (json::iterator id = cameraIDNumbers_.begin(); id != cameraIDNumbers_.end(); id++) {
    int cameraID = std::stoi(id.key());
    cameraInfoMap_[cameraID].camera->SetFrameCount(frameCount);
  }
}
