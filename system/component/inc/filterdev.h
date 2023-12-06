#pragma once

#include <shared_mutex>
#include <vector>

#include "system/component/inc/execnode.h"
#include "system/component/inc/frame.h"
#include "system/component/inc/pool.h"


// Compute mean, standard deviation of an image while applying a binary mask for bad pixels,
// flatfield correction (algorithm developed by Soren). Mean and standard deviation calculations
// are corrected using the mean and variance of the previous dark image.

class FilterDev : public ExecNode {
 public:
  FilterDev() {}

  // Construct based on fixed frame size
  FilterDev(const Frame* fr) { Init(fr->width, fr->height); }
  FilterDev(int width, int height) { Init(width, height); }
  ~FilterDev() {}

  // Set native height and width for incoming frames
  // @param width width of incoming frames
  // @param height height of incoming frames
  void Init(int width, int height);

  // Load pixel data, and indicate which pixels should be discarded for
  //   computing mean / standard deviation
  // Requires a pre-computed file with a list of (x, y, val) tuples, sorted by val.
  //   When LoadPixelData() is called, it will discard the first filter_pixel x, y coordinaes
  //   when computing the mean and standard deviation
  // @param fname file containing pixel information
  // @param camera_id which camera to load pixel information
  // @param ignore_pixels number of pixels to discard
  // @returns true on success
  bool LoadPixelData(const std::string& fname, int camera_id, int filter_pixels);

  // Load flatfield image
  // @param fname file containing flatfield image [must be column major 16bit tiff]
  // @returns true on success
  bool LoadFlatfieldImage(const std::string& fname);

  // Load bad pixel image mask
// @param fname file containing bad pixel image mask image [must be column major 16bit tiff]
// @returns true on success
  bool LoadBadPixelImage(const std::string& fname);

  // Set Gain Constant
  // @param k is gain constant (currently hard coded to zero, but may be used in the future)
  bool SetGainConstant(double k);

  struct Tag : Frame::Tag {
    double mean = 0;
    double stddev = 0;
    int saturated = 0;
  };

  void resize(size_t frames) override;

 private:
  void* Exec(void* data) override;
  void AtExit(void* data) override;

  Pool<Tag> pool_;

  // list of dead pixels
  std::shared_mutex mutex_;
  std::vector<int> dead_index_;
  int width_ = 0;
  int height_ = 0;
  int size_ = 0;
  int n_ = -1; // total number of valid pixels

  // parameters for dark image subtraction and flatfielding
  double darkMean_ = 0;  // mean of most recent dark frame
  double darkVar_ = 0;  // variance of most recent dark frame
  double gainConst_ = 0;  // Optional gain constant, currently zero, but can change in future
  Frame flatfieldFrame_;
  Frame badPixelFrame_;

  // Vars for computing filtered data

};
