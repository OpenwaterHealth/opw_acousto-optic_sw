#pragma once

#include "system/component/inc/execnode.h"
#include "system/component/inc/frame.h"

// Subtract a DC offset from a frame
class Offset : public ExecNode {
 public:
  Offset() {}
  ~Offset() {}

  double setOffset(double offset);

private:
  void* Exec(void* data) override;

  double offset_ = 0;
};
