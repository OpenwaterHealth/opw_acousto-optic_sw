#include "system/component/inc/realtimefft.h"

#pragma comment (lib, "sfml-graphics-s.lib")
#pragma comment (lib, "sfml-window-s.lib")
#pragma comment (lib, "sfml-system-s.lib")
#pragma comment (lib, "opengl32.lib")
#pragma comment (lib, "gdi32.lib")
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "freetype.lib")

#include "windows.h"

// TODO(carsten) redo real time FFT into frame pipeline
// this is temporary to accomodate scanning team's needs for changing scale factor
bool RealTimeFFT::compute_log_;
double RealTimeFFT::m_;
double RealTimeFFT::b_;


RealTimeFFT::RealTimeFFT(int h_sz, int v_sz, int n_threads) {
  h_sz_ = h_sz;
  v_sz_ = v_sz;
  fft_h_sz_ = h_sz_ / 2 + 1;
  fft_v_sz_ = v_sz_;
  fft_sz_ = (h_sz_ / 2 + 1) * v_sz_;

  int ret = fftw_import_wisdom_from_filename("fftw.wis");
  if (!ret) {
    printf("No FFT wisdom found, computing.  This can take several minutes\n");
  }

  fft_roi_ = new double[fft_sz_];
  fft_rou_ = new double[fft_sz_];
  fft_mask_px_ = new uint32_t[fft_sz_];
  for (int i = 0; i < fft_sz_; ++i) {
    fft_roi_[i] = 0;
    fft_rou_[i] = 0;
    fft_mask_px_[i] = 0;
  }

  threads_ = new CircularBuffer<FFTThread>(n_threads);
  for (FFTThread& thread : threads_->Raw()) {
    thread.state = FFTThread::STOPPED;
    thread.fft_in = new double[h_sz_ * v_sz_];
    thread.roi = fft_roi_;
    thread.rou = fft_rou_;
    thread.fft_out = fftw_alloc_complex(fft_sz_);
    thread.plan = fftw_plan_dft_r2c_2d(
      v_sz_,
      h_sz_,
      thread.fft_in,
      thread.fft_out,
      FFTW_EXHAUSTIVE);
    thread.px = new uint32_t[fft_sz_];
    for (int i = 0; i < fft_sz_; ++i) thread.px[i] = 0xFF000000;
    thread.sz = fft_sz_;
  }

  fftw_export_wisdom_to_filename("fftw.wis");

  tx_.create(fft_h_sz_, fft_v_sz_);
  tx_mask_.create(fft_h_sz_, fft_v_sz_);
  m_ = 2.5;
  b_ = 0;
  compute_log_ = true;
}


RealTimeFFT::~RealTimeFFT() {
  for (FFTThread& thread : threads_->Raw()) {
    thread.fft_in;
    fftw_free(thread.fft_out);
    thread.px;
  }
  delete threads_;
  delete[] fft_roi_;
  delete[] fft_rou_;
  delete[] fft_mask_px_;
}


void RealTimeFFT::SetMask(int c_h, int c_v, int r) {
  int roi_h = c_h;
  int roi_v = fft_v_sz_ - c_v;
  int rou_h = c_h;
  int rou_v = c_v;
  
  for (int v = 0; v < fft_v_sz_; ++v) {
    for (int h = 0; h < fft_h_sz_; ++h) {
      int idx = v * fft_h_sz_ + h;
      if ((roi_h - h) * (roi_h - h) + (roi_v - v) * (roi_v - v) < r * r) {
        fft_roi_[idx] = 1.0;
        fft_rou_[idx] = 0.0;
        fft_mask_px_[idx] = 0x40FF0000;
      } else if ((rou_h - h) * (rou_h - h) + (rou_v - v) * (rou_v - v) < r * r) {
        fft_roi_[idx] = 0.0;
        fft_rou_[idx] = 1.0;
        fft_mask_px_[idx] = 0x400000FF;
      } else {
        fft_roi_[idx] = 0.0;
        fft_rou_[idx] = 0.0;
        fft_mask_px_[idx] = 0;
      }
    }
  }
  tx_mask_.update((uint8_t*)fft_mask_px_);
}


void RealTimeFFT::SetRange(bool compute_log, double min_val, double max_val) {
  compute_log_ = compute_log;
  b_ = min_val;
  m_ = 255 / (max_val - min_val);
}

void RealTimeFFT::Compute(const Frame* fr) {
  while (threads_->PopAvailable()) {
    FFTThread* thread = &threads_->Peek();
    if (thread->state == FFTThread::FINISHED) {
      thread->state = FFTThread::STOPPED;
      pthread_join(thread->thread, NULL);
    }

    if ((threads_->PopAvailable() > 1) && (threads_->Peek(1).state == FFTThread::FINISHED)) {
      threads_->Pop();
      continue;
    } else {
      tx_.update((uint8_t*)thread->px);
      power_ = thread->power;
      break;
    }
  }

  if (threads_->PushAvailable()) {
    FFTThread* thread = &threads_->Next();
    for (int i = 0; i < h_sz_ * v_sz_; ++i) {
      thread->fft_in[i] = fr->data[i];
    }
    thread->state = FFTThread::RUNNING;
    pthread_create(&(thread->thread), NULL, Exec_, thread);
    threads_->Push();
  }
}


double RealTimeFFT::MaxPower() {
  if (threads_->PopAvailable()) {
    return threads_->Peek().max;
  } else {
    return 0;
  }
}


void* RealTimeFFT::Exec_(void* param) {
  FFTThread* thread = (FFTThread*)param;

  int t1 = GetTickCount();
  fftw_execute_dft_r2c(thread->plan, thread->fft_in, thread->fft_out);
  // printf("Time: %d\n", GetTickCount() - t1);

  thread->max = 0;
  double roi = 0;
  double rou = 0;
  for (int i = 0; i < thread->sz; ++i) {
    double res = abs2(thread->fft_out[i]);
    if (compute_log_) res = log(res + 1);
    roi += res * thread->roi[i];
    rou += res * thread->rou[i];
    if (res > thread->max) thread->max = res;
    thread->fft_out[i][0] = res;
  }
  thread->power = roi / rou;

  for (int i = 0; i < thread->sz; ++i) {
    double res = (thread->fft_out[i][0] - b_) * m_;
    if (res < 0) res = 0;
    if (res > 255) res = 255;
    memset(&(thread->px[i]), (uint8_t)res, 3);
  }

  thread->state = FFTThread::FINISHED;
  return (void*)thread;
}


void RealTimeFFT::SetWindow(int h_px, int v_px) {
  float sf_h = (float)h_px / h_sz_;
  float sf_v = (float)v_px / v_sz_;
 
  sp_[0].setTexture(tx_);
  sp_[0].setPosition(h_px * 2 / 4, 0);
  sp_[0].setTextureRect(sf::IntRect(0, v_sz_ / 2, h_sz_ / 2, v_sz_ / 2));
  sp_[0].setScale(sf_h, sf_v);

  sp_[1].setTexture(tx_);
  sp_[1].setPosition(0, 0);
  sp_[1].setTextureRect(sf::IntRect(h_sz_ / 2, v_sz_ / 2, -h_sz_ / 2, -v_sz_ / 2));
  sp_[1].setScale(sf_h, sf_v);

  sp_[2].setTexture(tx_);
  sp_[2].setPosition(0, v_px / 2);
  sp_[2].setTextureRect(sf::IntRect(h_sz_ / 2, v_sz_, -h_sz_ / 2, -v_sz_ / 2));
  sp_[2].setScale(sf_h, sf_v);

  sp_[3].setTexture(tx_);
  sp_[3].setPosition(h_px / 2, v_px / 2);
  sp_[3].setTextureRect(sf::IntRect(0, 0, h_sz_ / 2, v_sz_ / 2));
  sp_[3].setScale(sf_h, sf_v);

  sp_mask_[0].setTexture(tx_mask_);
  sp_mask_[0].setPosition(h_px * 2 / 4, 0);
  sp_mask_[0].setTextureRect(sf::IntRect(0, v_sz_ / 2, h_sz_ / 2, v_sz_ / 2));
  sp_mask_[0].setScale(sf_h, sf_v);

  sp_mask_[1].setTexture(tx_mask_);
  sp_mask_[1].setPosition(0, 0);
  sp_mask_[1].setTextureRect(sf::IntRect(h_sz_ / 2, v_sz_ / 2, -h_sz_ / 2, -v_sz_ / 2));
  sp_mask_[1].setScale(sf_h, sf_v);

  sp_mask_[2].setTexture(tx_mask_);
  sp_mask_[2].setPosition(0, v_px / 2);
  sp_mask_[2].setTextureRect(sf::IntRect(h_sz_ / 2, v_sz_, -h_sz_ / 2, -v_sz_ / 2));
  sp_mask_[2].setScale(sf_h, sf_v);

  sp_mask_[3].setTexture(tx_mask_);
  sp_mask_[3].setPosition(h_px / 2, v_px / 2);
  sp_mask_[3].setTextureRect(sf::IntRect(0, 0, h_sz_ / 2, v_sz_ / 2));
  sp_mask_[3].setScale(sf_h, sf_v);
}


void RealTimeFFT::draw(sf::RenderTarget& target, sf::RenderStates states) const {
  for (int i = 0; i < 4; ++i) {
    target.draw(sp_[i], states);
    target.draw(sp_mask_[i], states);
  }
}
