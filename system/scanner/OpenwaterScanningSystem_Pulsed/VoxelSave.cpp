#include "VoxelSave.h"

#include "system/component/inc/fftt.h"
#include "system/component/inc/frame.h"
#include "system/component/inc/roi.h"
#include "system/component/inc/stddev.h"

#include <iostream>

void* VoxelSave::Exec(void* data) {
  Frame* fr = (Frame*)data;
  voxelData vd;

  roi_tag* roiInfo = ROI::GetTag(fr);
  fft_tag* fftInfo = FFTT::GetTag(fr);

  vd.cameraID = fr->serialNumber;
  vd.imageName = "hologramImage" + std::to_string(fr->seq); // TODO(CR/carsten): if this becomes device frames test what happens after reset
  vd.roiFFTEnergy = roiInfo->roi;
  vd.rouFFTEnergy = roiInfo->rou;
  vd.imageMean = fftInfo->fft_zero;
  vd.POSIXTime = fr->timestamp_ms_;
  vd.speckleContrast = StdDev::GetTag(fr)->stddev / StdDev::GetTag(fr)->mean;

  cameras_->writeVoxelData(vd);

  return data;
}
