#pragma once

#include "rs232_wrapper.h"

class Stage {
 public:
  Stage(): rs232_(NULL) {}
  virtual ~Stage() { delete rs232_; rs232_ = NULL; }

  // Open the comPort here, rather than in the constructor.
  virtual bool init(int comPort) = 0;

  int getCOMPortNum(void) { return rs232_->Port(); }

  virtual int resetController() = 0;
  virtual int moveHome() = 0;
  virtual int disableController() = 0;

  virtual int moveRelative(double distance_mm) = 0;
  virtual int moveAbsolute(double location_mm) = 0;

  // Returns 0 (false) if not moving; 1 (true) if moving; -1 if error
  virtual int stageMoving(void) = 0;

  virtual double getStageLocation(void) = 0;

 protected:
  RS232 *rs232_;
};
