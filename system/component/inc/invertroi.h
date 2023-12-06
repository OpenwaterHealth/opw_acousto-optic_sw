#pragma once

#include <shared_mutex>

#include "system/component/inc/execnode.h"
#include "system/component/inc/frame.h"
#include "system/component/inc/fftt.h"
#include "system/component/inc/fftwutil.h"

// Compute the IFFT of the ROI
class InvertROI : public ExecNode {
 public:
  InvertROI() {}
  ~InvertROI();

  // Construct the object for a specified frame size
  InvertROI(const Frame* fr) { Resize(fr->width, fr->height); }
  InvertROI(int width, int height) { Resize(width, height); }

  // Resize the expected size of incoming frame
  void Resize(int width, int height);

  // Resize the number of concurrent iffts in progress
  void resize(size_t sz) override;

  // Set the ROI for the IFFT
  // @param x_c x center of the ROI as a fraction of the Nyquist frequency,
  //   value between 0-1
  // @param y_c y center of the ROI as a fraction of the Nyquist frequency
  // @param r radius of the ROI as a fraction of the Nyquist frequency
  // @note explict to avoid accidentally using pixel values
  void Set(double x_c, double y_c, double r);

  // Do not allow Set with integer values
  void Set(int x, int y, int r) = delete;

  // Set the ROI for the IFFT
  // @param x_c x center of the ROI in pixels
  // @param y_c y center of the ROI in pixels
  // @param r radius of the ROI
  void SetPixels(int x_c, int y_c, int r);

  struct Tag : Frame::Tag {
    fftw_plan plan;    // plan to compute IFFT from FFT
    fftw_complex* fft; // fft with non-ROI zeroed out
    double* ifft;      // raw IIFT of the ROI
    double mean;       // mean of the IFFT
    double stddev;     // standard deviation of the IFFT
  };

  static Tag* GetTag(const Frame* fr) { return fr->GetTag<Tag>(); }

 private:
  // size of incoming frames
  int width_ = 0;
  int height_ = 0;

  void* Exec(void* data) override;
  void AtExit(void* data) override;
  Pool<Tag> pool_;

  // Keep track of the ROI
  FFTWCircle roi_;
};
