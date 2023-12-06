#pragma once

// A simple interface we can pass to CameraManager to trigger capture.
class Trigger {
 public:
  Trigger() {}
  ~Trigger() {}
  virtual void triggerAcquisition() = 0;
};
