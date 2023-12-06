#pragma once

#include <cstdint>

#include "CameraManager.h"

#include "system/component/inc/execnode.h"

class VoxelSave : public ExecNode {
 public:
  VoxelSave(CameraManager* cameras) : cameras_(cameras) {}
  ~VoxelSave() {};

 private:
  void* Exec(void* data) override;
  CameraManager* cameras_;
};
