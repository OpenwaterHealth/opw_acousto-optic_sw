#pragma once

#include <cstdint>
#include <fstream>

#include "system/component/inc/execnode.h"

class BloodflowCameraManager;

class BloodflowVoxelSave : public ExecNode {
 public:
  BloodflowVoxelSave(BloodflowCameraManager* bloodflowCameras) : bloodflowCameras_(bloodflowCameras) {}
  ~BloodflowVoxelSave() {};

  std::string getFilename();

  std::string setFilename(const std::string& filename);

  std::string setPulseWidth(const std::string& pulseWidth_s);

  // Enable saving [work around for extra frame at beginning of continuous acquisition sequence]
  void enableSave(bool saveON);

 private:
  void* Exec(void* data) override;
  BloodflowCameraManager* bloodflowCameras_;

  std::string filename_ = "";
  std::string pulseWidth_s_ = "";
  bool enableSave_ = false;
  std::mutex bvsMutex_;
};
