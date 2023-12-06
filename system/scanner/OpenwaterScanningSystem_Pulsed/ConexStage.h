#pragma once

#include "stages.h"

class ConexStage: public Stage {
 public:
  ConexStage();
  ~ConexStage();

  bool init(int comPort) override;

  int resetController() override;
  int moveHome() override;
  int disableController() override;

  int moveRelative(double distance_mm) override;
  int moveAbsolute(double location_mm) override;

  int stageMoving(void) override;

  double getStageLocation(void) override;
};
