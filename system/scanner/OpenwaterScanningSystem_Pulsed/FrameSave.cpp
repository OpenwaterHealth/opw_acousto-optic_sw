#include <string>

#include "FrameSave.h"

#include "system/component/inc/frame.h"

void* FrameSave::Exec(void* data) {
  Frame* fr = (Frame*)data;
  mutex_.lock();

  if (n_frames_ < 0) {
    std::string fname = filename_ + std::to_string(fr->seq) + ".tiff";
    mutex_.unlock();
    fr->Write(fname.c_str());
    return data;
  }

  if (n_frames_ == 0) {
    mutex_.unlock();
    return data;
  }

  std::string fname;
  if (frame_count_ == 0 && n_frames_ == 1) {
    fname = filename_ + ".tiff";
  } else {
    fname = filename_ + std::to_string(frame_count_) + ".tiff";
  }
 
  ++frame_count_;
  --n_frames_;
  mutex_.unlock();
  fr->Write(fname.c_str());
  return data;
}

std::string FrameSave::getFilename() {
  return filename_;
}

std::string FrameSave::setFilename(std::string filename, int n_frames) {
  std::lock_guard<std::mutex> lock(mutex_);
  filename_ = filename;
  frame_count_ = 0;
  if (filename_.compare("") == 0) {
    n_frames_ = 0;
  } else {
    n_frames_ = n_frames;
  }
  return filename_;
}
