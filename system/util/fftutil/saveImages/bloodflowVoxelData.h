#pragma once

#include <string>

// Voxel data to write to CSV
struct bloodflowVoxelData {
  std::string imageName = "speckleImageX";
  int cameraID = 0;
  time_t timestamp = 0;
  double imageMean = 0;
  double imageStd = 0;
  double speckleContrast = 0;
  double filteredImageMean = 0;
  double filteredImageStd = 0;
  double filteredSpeckleContrast = 0;
  double temperature = 0;
  int saturatedPixels = 0;
  std::string pulseWidth_s = "";
};
