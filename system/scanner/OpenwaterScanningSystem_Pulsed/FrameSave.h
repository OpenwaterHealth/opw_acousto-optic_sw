#pragma once

#include <cstdint>
#include <mutex>

#include "system/component/inc/execnode.h"

class FrameSave : public ExecNode {
 public:
  FrameSave() {}
  ~FrameSave() {};

  std::string getFilename();

  // Set a filename for incoming frames to be saved to, and start saving immediately
  // @param filename filename to save.  Frames will be saved as filename<frame_number>.tiff
  // @param n_frames how many frames to save.  Omit or use a negative number to save all frames
  std::string setFilename(std::string filename, int n_frames = -1);

 private:
  void* Exec(void* data) override;

  int frame_count_ = 0;
  int n_frames_ = 0;
  std::mutex mutex_;
  std::string filename_ = "";
};
