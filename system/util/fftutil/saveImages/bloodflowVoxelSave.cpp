#include <string>
#include <iostream>

#include "BloodflowCameraManager.h"
#include "bloodflowVoxelData.h"
#include "bloodflowVoxelSave.h"

#include "system/component/inc/filterdev.h"
#include "system/component/inc/frame.h"
#include "system/component/inc/stdDev.h"

void* BloodflowVoxelSave::Exec(void* data) {
  Frame* fr = (Frame*)data;
  std::lock_guard<std::mutex> lock(bvsMutex_);
  if (filename_ != "") {
    std::string fname = filename_ + std::to_string(fr->seq) + ".tiff";
    fr->Write(fname.c_str());
  }

  bloodflowVoxelData voxelData;
  voxelData.imageName = "speckleImage" + std::to_string(fr->seq);
  voxelData.cameraID = fr->serialNumber;
  voxelData.timestamp = fr->timestamp_ms_;
  voxelData.imageMean = StdDev::GetTag(fr)->mean;
  voxelData.imageStd = StdDev::GetTag(fr)->stddev;
  voxelData.speckleContrast = voxelData.imageStd / voxelData.imageMean;
  if (fr->GetTag<FilterDev::Tag>()) {
    voxelData.filteredImageMean =  fr->GetTag<FilterDev::Tag>()->mean;
    voxelData.filteredImageStd = fr->GetTag<FilterDev::Tag>()->stddev;
    voxelData.filteredSpeckleContrast = voxelData.filteredImageStd / voxelData.filteredImageMean;
    voxelData.saturatedPixels = fr->GetTag <FilterDev::Tag>()->saturated;
  }
  voxelData.temperature = fr->temperature;
  voxelData.pulseWidth_s = pulseWidth_s_;

  if (enableSave_) {
    bloodflowCameras_->writeBloodflowVoxelData(voxelData);
  }

  return data;
}

std::string BloodflowVoxelSave::getFilename() {
  return filename_;
}

std::string BloodflowVoxelSave::setFilename(const std::string& filename) {
  filename_ = filename;
  return filename_;
}

std::string BloodflowVoxelSave::setPulseWidth(const std::string& pulseWidth_s) {
  pulseWidth_s_ = pulseWidth_s;
  return pulseWidth_s_;
}

void BloodflowVoxelSave::enableSave(bool saveON) {
  enableSave_ = saveON;
}
