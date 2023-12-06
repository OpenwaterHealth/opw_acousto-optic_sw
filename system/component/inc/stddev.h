#pragma once

#include "system/component/inc/execnode.h"
#include "system/component/inc/frame.h"
#include "system/component/inc/pool.h"

// Compute the mean and standard deviation on frames
class StdDev : public ExecNode {
 public:
  StdDev() {}
  ~StdDev() {}

  struct Tag : Frame::Tag {
    double mean;
    double stddev;
  };

  // Get a mean / standard deviation tag from a frame
  // returns NULL if no tag found
  // @note deprecated, use Frame::GetTag<StdDev::Tag>();
  static Tag* GetTag(const Frame* frame) { return frame->GetTag<Tag>(); }

  // resize the number of ExecNodes that can be active at one time
  void resize(size_t size) override;

 private:
  void* Exec(void* data) override;
  void AtExit(void* data) override;

  Pool<Tag> pool_;
};
