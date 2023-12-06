#pragma once

#include <mutex>

#include "fftw3.h"

#include "execnode.h"
#include "fftwutil.h"
#include "frame.h"
#include "pool.h"
#include "zoomable.h"





// Multithreaded FFT
// Computes the absolute value of the FFT of the specified region
//   of incoming frames.
// allows for up to 10 simultaneous FFT calculations
class FFTT : public ExecNode {
 public:
  // default constructor
  // call Resize() to allocate memory for a particular frame size
  FFTT();

  // constructor
  // @param height height of the incoming frames
  // @param width width of incoming frames
  FFTT(int width, int height);

  // constructor
  // @param fr example frame (height, width) to setup buffers
  FFTT(Frame* fr);

  ~FFTT();

  // Resize FFTT to handle frames of a given x/y dimension
  // @note resizing existing FFTTs not supported
  void Resize(int width, int height);

  // Resize FFTT to handle frames of a given size
  void Resize(const Frame* fr);

  // Set subwindow to use in computing FFT
  // @param x0 x coordinate of point 0
  // @param y0 y coordinate of point 0
  // @param x1 x coordinate of point 1
  // @param y1 y coordinate of point 1
  void SubWindow2Point(int x0, int y0, int x1, int y1);

  // FFT data structure
  // fft: absolute value of the fft
  // fftw_complex: output from fftw
  // plan: plan to compute this FFT
  // ms: milliseconds used to compute the FFT
  // x: x position of subwindow used to compute FFT
  // y: y position of subwindow used to compute FFT
  // x_sz: x size of the subwindow (in pixels)
  // y_sz: y size of the submwindow (in pixels)
  struct Tag : Frame::Tag {
    double* fft = NULL;
    fftw_complex* fft_complex = NULL;
    fftw_plan plan;
    time_t ms;
    int x;
    int y;
    int x_sz;
    int y_sz;
    double fft_zero = 0.0;
  };

  // get FFT data from a frame, NULL if not available
  // @param fr frame to examine
  // @returns fft_tag* pointer for FFT data structure
  // @note deprecated, use Frame::GetTag<FFTT::Tag>();
  static Tag* GetTag(const Frame* fr) { return fr->GetTag<Tag>(); }

  // Resize buffers to allow for n concurrent FFT computations
  void resize(size_t n) override;

 private:
  // Execute the FFT on the incoming frame
  void* Exec(void* data) override;

  // Deallocate FFT memory used
  void AtExit(void* data) override;

  void Init(int x_sz, int y_sz);

  static const int BUFLEN_ = 10;

  std::mutex mutex_;

  Pool<Tag> data_;

  int x_sz_ = 0;
  int y_sz_ = 0;
  int fft_x_sz_, fft_y_sz_, fft_sz_;
  int subwin_x_ = 0;
  int subwin_y_ = 0;
  int subwin_x_sz_ = 0;
  int subwin_y_sz_ = 0;
};

// @note deprecated, use FFTT::Tag
typedef FFTT::Tag fft_tag;

class FFTTDraw : public ExecNode, public FFTWDraw {
 public:
  // default constructor
  // use Resize() to allocate memory
  FFTTDraw();

  // constructor
  // @param x_sz x size of the incoming frames
  // @param y_sz y size of incoming frames
  FFTTDraw(int x_sz, int y_sz);

  // constructor
  // @param fr example frame (height, width) to setup buffers
  FFTTDraw(const Frame* fr);

  ~FFTTDraw();

  // Allocate memory for Frames of size x_sz x y_sz
  // @param x_sz width of the frames
  // @param y_sz height of the frames
  void Resize(int x_sz, int y_sz);

  // Allocate memory for a given frame
  // @param fr example frame to consume
  void Resize(const Frame* fr);

  // get the maximum power (log or lin scale depending on settings)
  // @returns max power from most recent frame
  double MaxPower();

  // set scale to display
  // @param compute_log true if log scale, false if linear
  // @param min value that corresponds to full black (lower values will clip)
  // @param max value that corresponds to full white (higher values will clip)
  void SetScale(bool compute_log, double min, double max);

  // get current display values
  // @param compute_log true if log scale, false if linear
  // @param min value that corresponds to full black (lower values will clip)
  // @param max value that corresponds to full white (higher values will clip)
  void GetScale(bool* compute_log, double* min, double* max);

 private:
  void* Exec(void* data);

  void Init(int x_sz, int y_sz);

  int fft_x_sz_, fft_y_sz_, fft_sz_;
  int width_ = 0;
  int height_ = 0;
  bool compute_log_;
  double m_, x0_;
  double scale_min_, scale_max_;
  double max_;
  std::mutex wr_lock_;
  std::vector<uint32_t> px_;
};

// Class to draw the subwindow selected by the FFTT object
class FFTTSubWindowDraw : public ExecNode, public Zoomable {
 public:
  FFTTSubWindowDraw() {}
  ~FFTTSubWindowDraw() {}

  // Allocate memory for Frames of size x_sz x y_sz
  // @param x_sz width of the frames
  // @param y_sz height of the frames
  void Resize(int x_sz, int y_sz);

  // Allocate memory for a given frame
  // @param fr example frame to consume
  void Resize(const Frame* fr);

 private:
  void* Exec(void* data) override;

  int width_ = 0;
  int height_ = 0;
  int subwin_x_ = 0;
  int subwin_y_ = 0;
  int subwin_x_sz_ = 0;
  int subwin_y_sz_ = 0;
  sf::VertexArray vertex_;
};
