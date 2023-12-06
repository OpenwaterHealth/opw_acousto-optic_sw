#pragma once

#include <fstream>
#include <iomanip>
#include <time.h>

// N.B.: Keep in sync with output operator, below!
const char* const CSVHeader =
  "imageName,cameraID,POSIXTime,i,j,k,alphai,betai,gammai,ustxX,ustxZ,"
  "x,y,z,alpha,beta,gamma,azimuth,axial,roiFFTEnergy,rouFFTEnergy,imageMean,speckleContrast\n";

// Voxel data to write to CSV
struct voxelData {
  std::string imageName;
  int cameraID;
  int ROI_xCenter, ROI_yCenter, ROI_radius;
  int i = 0, j = 0, k = 0, alphaIndex = 0, betaIndex = 0, gammaIndex = 0, usX = 0, usZ = 0;
  double x = 0, y = 0, z = 0, alpha = 0, beta = 0, gamma = 0, azimuth = 0, axial = 0;
  double roiFFTEnergy = 0, rouFFTEnergy = 0, imageMean = 0, speckleContrast = 0;
  time_t POSIXTime = 0;

  void operator>>(std::ofstream& stream) {
    stream << imageName << "," << cameraID << "," << POSIXTime << ","
        << i << "," << j << "," << k << ","
        << alphaIndex << "," << betaIndex << "," << gammaIndex << ","
        << usX << "," << usZ << ","
        << x << "," << y << "," << z << ","
        << alpha << "," << beta << "," << gamma << ","
        << azimuth << "," << axial << ","
        << roiFFTEnergy << "," << rouFFTEnergy << "," 
        << imageMean << "," << speckleContrast << std::endl;
  }
};
