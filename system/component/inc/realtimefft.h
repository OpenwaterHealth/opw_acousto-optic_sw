#pragma once

#include "fftw3.h"
#define SFML_STATIC
#include "SFML/Graphics.hpp"
#include "SFML/Window.hpp"

#include "rcam.h"
#include "circular_buffer.h"

#define HAVE_STRUCT_TIMESPEC
#include "pthread.h"

class RealTimeFFT : public sf::Drawable {
 public:
   // Allocate space and do pre-compuataion for real-time FFT of Rcam frames
   // @param h_sz horizontal pixels
   // @param v_sz vertical pixels 
   RealTimeFFT(int h_sz, int v_sz, int n_threads = 2);
   ~RealTimeFFT();
   
   // Create a framebuffer to draw the FFT
   // @param h_px number of pixels to draw horizontally
   // @param v_px number of pixels to draw vertically
   void SetWindow(int h_px, int v_px);

   // Compute a frame of FFT
   void Compute(const Frame* fr);

   // Set ROI
   void SetMask(int c_h, int c_v, int r);

   // get max power from last frame
   double MaxPower();

   // set display parameters
   // @param compute_log 0 if linear, 1 if log scale
   // @param max_val value to scale to 255 (full white).
   //   values above max_val will saturate to 255
   void SetRange(bool compute_log, double min_val, double max_val);

   double MaskPower() { return power_; }

   // Set default renderstates
   void SetRenderStates(sf::RenderStates states);

   void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

   static double abs2(fftw_complex c) {
     return c[0] * c[0] + c[1] * c[1];
   }

private:
  int h_sz_, v_sz_, fft_h_sz_, fft_v_sz_, fft_sz_;

  double* fft_roi_;
  double* fft_rou_;
  uint32_t* fft_mask_px_;
  double power_ = 0;

  struct FFTThread {
    pthread_t thread;
    volatile enum {STOPPED, RUNNING, FINISHED} state;
    double* fft_in;
    uint32_t* px;
    double* roi;
    double* rou;
    fftw_complex* fft_out;
    fftw_plan plan;
    double max;
    double scale;
    double power;
    int sz;
  };

  CircularBuffer<FFTThread>* threads_;
  static void* Exec_(void* param);

  fftw_complex* last_ = NULL;
  
  static bool compute_log_;
  static double m_, b_;
  float sf_h_, sf_v_;
  uint32_t* px_ = NULL;
  sf::Texture tx_;
  sf::Sprite sp_[4];
  sf::Texture tx_mask_;
  sf::Sprite sp_mask_[4];
};
