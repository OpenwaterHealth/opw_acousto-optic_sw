#pragma once

#include <mutex>

#include "system/third_party/json-develop/single_include/nlohmann/json.hpp"

#include "trigger.h"
#include "VoxelData.h"

class FFTT;
class Rcam;
class ROI;
class StdDev;
class FrameSave;
class VoxelSave;

// Encapsulate all the expertise on [multi]camera handling for scanning.
class CameraManager {
 public:
  CameraManager() {}
  virtual ~CameraManager();

  using json = nlohmann::json;

  virtual bool init(const json& systemParameters, Trigger* trigger);

  // Asynchronously capture numFociPerRow frames
  // numFociPerSlice and sliceIdx must also be provided to check that the number of
  // images acquired since the camera was started matches the expected frame count, otherwise the
  // voxel indexing will become out of sync with the ultrasound focus
  // axialRowIdx must be provided to reset the frame count in the event that the camera and
  // ustx become out of sync
  virtual int captureAndWriteImagesAsync(int numFociPerSlice, int sliceIdx, int numFociPerRow, int axialRowIdx, double frameGatePeriod_s);

  void writeVoxelData(const voxelData& newVoxelData);

  void setImageInfoStream(std::ofstream* stream, bool local = true);

  void setRepeatedVoxelLogStream(std::ofstream* stream);

  void startAllCameras();

  void resetAllCamerasMidscan(int frameCount);

  Rcam* resetCameraMidscan(Rcam* camera);

  bool endExecNodes();

 private:
  Trigger* trigger_;  // Not owned
  double exposureTime_s_;
  std::string syncedRawImageDir_;
  json cameraIDNumbers_;
  std::ofstream* imageInfoLocal_;
  std::ofstream* imageInfoSynced_;
  std::ofstream* repeatedVoxelLog_;
  std::mutex voxelDataMutex_;

  // Container mapping each cameraID attached [key: (int) cameraID#, value: struct]
  struct cameraInfo {
    int cameraID = -1;
    voxelData voxelData;
    int portNumber = -1;
    Rcam* camera;
    FFTT* fftt;
    ROI* roi;
    StdDev* stdDev;
    FrameSave* frameSave;
    VoxelSave* voxelSave;
  };
  std::map<int, cameraInfo> cameraInfoMap_;
};
