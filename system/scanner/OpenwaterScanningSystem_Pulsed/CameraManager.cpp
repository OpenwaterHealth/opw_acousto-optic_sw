#include <iostream>
#include <time.h>

#include "CameraManager.h"
#include "FrameSave.h"
#include "VoxelSave.h"

#include "system/component/inc/fftt.h"
#include "system/component/inc/fx3.h"
#include "system/component/inc/rcam.h"
#include "system/component/inc/roi.h"
#include "system/component/inc/stddev.h"
#include "system/component/inc/time.h"

using json = nlohmann::json;

CameraManager::~CameraManager() {
  for (json::iterator id = cameraIDNumbers_.begin(); id != cameraIDNumbers_.end(); id++) {
    int cameraID = std::stoi(id.key());
    cameraInfo& info = cameraInfoMap_[cameraID];
    // Clean up exec node objects
    delete info.camera;
    delete info.fftt;
    delete info.roi;
    delete info.stdDev;
    delete info.frameSave;
    delete info.voxelSave;
  }
}

bool CameraManager::init(const json& systemParameters, Trigger* trigger) {
  // Parse camera parameters from JSON metadata
  const auto& camParams = systemParameters["cameraParameters"];
  exposureTime_s_ = camParams["exposureTime_ms"].get<double>() * 0.001;
    //TODO (CR): Should this actually be calculated based on getFrameLength/RowTime?
  int resolutionX = camParams["resolutionX"].get<int>();
  int resolutionY = camParams["resolutionY"].get<int>();
  std::string cameraIDNumbersStr = camParams["cameraIDNumbers"].dump();
  cameraIDNumbers_ = json::parse(cameraIDNumbersStr);
    // Key: (int) ID number, Value: location information string
  // Parse ROI values [note: this will go away once optics fixed]
  std::string cameraROI_xCenterStr = camParams["cameraROI_xCenter"].dump();
  json cameraROI_xCenterJSON = json::parse(cameraROI_xCenterStr);
  // Key: (int) ID number, Value: x ROI center (int: pixels)
  std::string cameraROI_yCenterStr = camParams["cameraROI_yCenter"].dump();
  json cameraROI_yCenterJSON = json::parse(cameraROI_yCenterStr);
  // Key: (int) ID number, Value: y ROI center (int: pixels)
  std::string cameraROI_radiusStr = camParams["cameraROI_radius"].dump();
  json cameraROI_radiusJSON = json::parse(cameraROI_radiusStr);
  // Key: (int) ID number, Value: radius (int: pixels)

  // System control information
  int pulsedSystem = systemParameters["laserParameters"]["pulsed"].get<int>();
  std::string usAmplifier = systemParameters["ultrasoundParameters"]["ultrasoundAmp"].get<std::string>();
  syncedRawImageDir_ = systemParameters["fileParameters"]["syncedRawImageDir"].get<std::string>();

  // How many cameras are in the system?
  int numCameras = Rcam::NumCameras();
  if (numCameras != cameraIDNumbers_.size()) {
    std::cout << "Error: Number of cameras from set-up file does not match number of HM-USB devices connected to computer." << std::endl;
    std::cout << "Number cameras connected: " << numCameras << std::endl;
    std::cout << "Number of cameras in set-up file: " << cameraIDNumbers_.size() << std::endl;
    return false;
  }

  // Set up cameras and image processing pipeline
  for (int i = 0; i < numCameras; i++) {
    Rcam* camera = new Rcam();
    if (camera->Open(i) != 0) return false;  // error msgs will come from Open()
    int cameraID = camera->SerialNumber();  // serial number now returns cameraID
    std::cout << "INFO: Connected to Camera: " << cameraID << std::endl;

    // Store camera information
    cameraInfo& cameraInfo = cameraInfoMap_[cameraID];
    cameraInfo.cameraID = cameraID;
    cameraInfo.camera = camera;
    cameraInfo.portNumber = i;
    cameraInfo.fftt = new FFTT(resolutionX, resolutionY);
    cameraInfo.roi = new ROI(resolutionX, resolutionY);
    cameraInfo.stdDev = new StdDev();
    cameraInfo.frameSave = new FrameSave();
    cameraInfo.voxelSave = new VoxelSave(this);

    cameraInfo.camera->SetExposure(exposureTime_s_);
    cameraInfo.camera->SubWindow2Point(0, 0, resolutionX, resolutionY);

    int ROI_xCenter = cameraROI_xCenterJSON[std::to_string(cameraID)].get<int>();
    int ROI_yCenter = cameraROI_yCenterJSON[std::to_string(cameraID)].get<int>();
    int ROI_radius = cameraROI_radiusJSON[std::to_string(cameraID)].get<int>();

    // Set up processing chain
    cameraInfo.fftt->AddProducer(cameraInfo.camera);
    cameraInfo.roi->AddProducer(cameraInfo.fftt);
    cameraInfo.roi->Set(ROI_xCenter, ROI_yCenter, ROI_radius);
    cameraInfo.stdDev->AddProducer(cameraInfo.roi);
    cameraInfo.frameSave->AddProducer(cameraInfo.stdDev);
    cameraInfo.voxelSave->AddProducer(cameraInfo.frameSave);

    if (syncedRawImageDir_ != "") {
      cameraInfo.frameSave->setFilename(syncedRawImageDir_ + "/camera" + std::to_string(cameraID) + "/hologramImage");
    }
    cameraInfo.camera->resize(30);
  }

  trigger_ = trigger;

  return true;
}

void CameraManager::writeVoxelData(const voxelData& newVoxelData) {
  std::lock_guard<std::mutex> lock(voxelDataMutex_);
  voxelData& v = cameraInfoMap_[newVoxelData.cameraID].voxelData;

  v.cameraID = newVoxelData.cameraID;
  v.imageName = newVoxelData.imageName; // Based on fr->seq
  v.roiFFTEnergy = newVoxelData.roiFFTEnergy;
  v.rouFFTEnergy = newVoxelData.rouFFTEnergy;
  v.imageMean = newVoxelData.imageMean;
  v.POSIXTime = newVoxelData.POSIXTime;
  v.speckleContrast = newVoxelData.speckleContrast;

  v >> (*imageInfoLocal_);
  v >> (*imageInfoSynced_);
}

void CameraManager::setImageInfoStream(std::ofstream* stream, bool local) {
  if (local) {
    imageInfoLocal_ = stream;
  } else {
    imageInfoSynced_ = stream;
  }
}

void CameraManager::setRepeatedVoxelLogStream(std::ofstream* stream) {
  repeatedVoxelLog_ = stream;
}

void CameraManager::startAllCameras() {
  for (json::iterator id = cameraIDNumbers_.begin(); id != cameraIDNumbers_.end(); id++) {
    int cameraID = std::stoi(id.key());
    std::cout << "INFO: start camera id: " << cameraID << "; starting camera: " << cameraInfoMap_[cameraID].cameraID << std::endl;
    cameraInfoMap_[cameraID].camera->Start();
  }
  Component::SleepMs(1000);  // Magic sleep to ensure 2 FSIN pulses before frame valid
}

Rcam* CameraManager::resetCameraMidscan(Rcam* camera) {
  camera->Reset();
  camera->SetExposure(exposureTime_s_);
  camera->Start();
  Component::SleepMs(50);
  return camera;
}

void CameraManager::resetAllCamerasMidscan(int frameCount) {
  for (json::iterator id = cameraIDNumbers_.begin(); id != cameraIDNumbers_.end(); id++) {
    int cameraID = std::stoi(id.key());
    Rcam* camera = cameraInfoMap_[cameraID].camera;
    resetCameraMidscan(camera);
    camera->SetFrameCount(frameCount);
  }
}

int CameraManager::captureAndWriteImagesAsync(int numFociPerSlice, int sliceIdx, int numFociPerRow, int axialRowIdx, double frameGatePeriod_ms) {
  // Axial Row Acquisition Trigger
  trigger_->triggerAcquisition();  // Trigger image acquisition cascade
  Component::SleepMs(frameGatePeriod_ms + 3.0*frameGatePeriod_ms/numFociPerRow);  // Ensure last frame is triggered, exposure completes, and transfers over usb

  for (json::iterator id = cameraIDNumbers_.begin(); id != cameraIDNumbers_.end(); id++) {
    int cameraID = std::stoi(id.key());
    Rcam* camera = cameraInfoMap_[cameraID].camera;
    if (camera->GetFrameCount() != sliceIdx * numFociPerSlice + axialRowIdx * numFociPerRow) {
      std::cout << "WARNING: Expected Frames: " << sliceIdx * numFociPerSlice + axialRowIdx * numFociPerRow << std::endl;
      std::cout << "WARNING: Frames: " << camera->GetFrameCount() << std::endl;
      std::cout << "WARNING: Dropped Frames: " << camera->DroppedFrames() << std::endl;
      *repeatedVoxelLog_ << sliceIdx * numFociPerSlice + (axialRowIdx - 1) * numFociPerRow << std::endl;
      camera->SetFrameCount(sliceIdx * numFociPerSlice + (axialRowIdx - 1) * numFociPerRow);
      return -1;
    }
  }
  return 1;
}

bool CameraManager::endExecNodes() {
  for (json::iterator id = cameraIDNumbers_.begin(); id != cameraIDNumbers_.end(); id++) {
    int cameraID = std::stoi(id.key());
    Rcam* camera = cameraInfoMap_[cameraID].camera;
    std::cout << "INFO: Camera " << cameraID << " Frames: " << camera->GetFrameCount() << std::endl;
    std::cout << "INFO: Camera " << cameraID << " Dropped Frames: " << camera->DroppedFrames() << std::endl;

    while (!cameraInfoMap_[cameraID].camera->IsExecDone()) {
      // Note: do not call IsExecDone from voxelSave, it will return done when it is not actually done
      std::cout << "INFO: waiting For Exec Nodes to finish" << std::endl;
      Component::SleepMs(1000);
    }
  }
  return true;
}
