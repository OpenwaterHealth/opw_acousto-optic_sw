// Classes and data structures to compute and display the
//   regoin of interest / uninterest from an FFT
#pragma once

#include <mutex>
#include <vector>

#include "execnode.h"
#include "fftt.h"
#include "frame.h"
#include "pool.h"
#include "zoomable.h"




// compute ROI / ROU from an FFT
// ROI is the region of interest, and all points in the ROI
//   are summed together to produce a single value
// ROU is the region of uninterest (ie, noise).  Points in the
//   ROU are similarly summed
// As we are interested in power, sums here are absolute value
//   squared.
class ROI : public ExecNode {
 public:
  ROI() {}

  ROI(int width, int height);
  ROI(const Frame* fr);

  ~ROI();

  // resize ROI to deal with different incoming frame sizes
  void Resize(int width, int height);
  void Resize(const Frame* fr);

  // tag containing ROI data
  // x_c, y_c, and radius describe the ROI being calculated
  //   as a circle centered at (x_c, y_c) with radius r
  // x_c, y_c, and r are in fractional nyquist units, ie span [-1 .. 1]
  // ROI is the sum of the FFT
  // ROU is the region of uninterest, a circle located
  // at (-y_c, x_c) with radius r
  struct Tag : Frame::Tag {
    double x_c;
    double y_c;
    double r;
    double roi;
    double rou;
  };

  // Get an ROI tag from a frame
  // returns NULL if no ROI tag associated with the frame
  // @return roi tag of frame
  // @note deprecated, use Frame::GetTag<ROI::Tag>();
  static Tag* GetTag(const Frame* fr) { return fr->GetTag<Tag>(); }

  // Set ROI based on center (x,y) and radius
  // @note deprecated, use Set(double)
  // @param x_c x-position of circle center in pixels
  // @param y_c y-position of circle center in pixels
  // @param r radius
  void Set(int x_c, int y_c, int r);

  // Set ROI based on center(x, y) and radius
  // @param x_c x-position of circle center
  // @param y_c y-position of circle center
  // @param r radius
  void Set(double x_c, double y_c, double r);

  // Get the circle centered at (x_c, y_c) with radius r
  //   that defines the ROI.
  // Pointers can be NULL if data is not needed.
  // @param x_c where to store the current x center
  // @param y_c where to store the current y center
  // @param r where to store the current radius
  void Get(int* x_c, int* y_c, int* r);

  // Get the circle centered at (x_c, y_c) with radius r
  //   that defines the ROI.
  // Pointers can be NULL if data is not needed.
  // @param x_c where to store the current x center
  // @param y_c where to store the current y center
  // @param r where to store the current radius
  void Get(double* x_c, double* y_c, double* r);

  // Resize number of concurrent ROI calculations
  void resize(size_t n) override;

 private:
  // Initialize object for frames of size width x height
  void Init(int width, int height);

  // Compute ROI / ROU from an fft_tag
  void* Exec(void* data) override;

  // Cleanup ROI / ROU tag
  void AtExit(void* data) override;

  Pool<Tag> data_;

  int width_ = 0;
  int height_ = 0;
  int fft_x_sz_, fft_y_sz_;
  double x_c_, y_c_, r_;

  // keep track if a point is in the ROI/ROU or not
  FFTWCircle roi_;
  FFTWCircle rou_;

  std::mutex mutex_;
};

// @note deprecated, use ROI::Tag
typedef ROI::Tag roi_tag;

class ROIDraw : public ExecNode, public FFTWDraw {
 public:
  ROIDraw() {}

  ROIDraw(int width, int height);
  ROIDraw(const Frame* fr);

  ~ROIDraw() {}

  // Resize an ROI drawing for a frame size
  void Resize(int width, int height);
  void Resize(const Frame* fr);

  // Set the ROI to draw
  // @param x_c x center of ROI
  // @param y_c y center of ROI
  // @param r radius
  void Set(double x_c, double y_c, double radius);

 private:
  void Init(int width, int height);

  void* Exec(void* data) override;

  int fft_x_sz_, fft_y_sz_;
  int width_ = 0;
  int height_ = 0;
  double x_c_, y_c_, r_;
  std::mutex wr_lock_;
  std::vector<uint32_t> px_;
  sf::Texture tx_;
  sf::Sprite sp_[4];
};
