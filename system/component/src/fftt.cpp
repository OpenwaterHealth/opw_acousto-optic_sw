#include "system/component/inc/fftt.h"
#include "system/component/inc/time.h"

#include <cassert>
#include <cmath>
#include <cstdio>

#ifdef _MSC_VER
#ifdef _DEBUG
#pragma comment(lib, "sfml-graphics-s-d.lib")
#pragma comment(lib, "sfml-window-s-d.lib")
#pragma comment(lib, "sfml-system-s-d.lib")
#else
#pragma comment(lib, "sfml-graphics-s.lib")
#pragma comment(lib, "sfml-window-s.lib")
#pragma comment(lib, "sfml-system-s.lib")
#endif
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "freetype.lib")
#pragma comment(lib, "libfftw3-3.lib")
#endif


FFTT::FFTT() {}

FFTT::FFTT(Frame* fr) { Init(fr->width, fr->height); }

FFTT::FFTT(int height, int width) { Init(height, width); }

void FFTT::Resize(const Frame* fr) {
  assert(x_sz_ == 0);
  Init(fr->width, fr->height);
}

void FFTT::Resize(int height, int width) {
  assert(x_sz_ == 0);
  Init(height, width);
}

void FFTT::Init(int x_sz, int y_sz) {
  x_sz_ = x_sz;
  y_sz_ = y_sz;
  fft_x_sz_ = x_sz_ / 2 + 1;
  fft_y_sz_ = y_sz_;
  fft_sz_ = fft_x_sz_ * fft_y_sz_;
  subwin_x_sz_ = x_sz;
  subwin_y_sz_ = y_sz;

  int ret = fftw_import_wisdom_from_filename("fftw.wis");
  if (!ret) {
    printf("No FFT wisdom found, computing.  This can take several minutes\n");
  }

  data_.resize(BUFLEN_);
  for (fft_tag& t : data_.Raw()) {
    t.fft = new double[x_sz_ * y_sz_];
    t.fft_complex = fftw_alloc_complex(fft_sz_);
    t.plan = fftw_plan_dft_r2c_2d(y_sz_, x_sz_, t.fft, t.fft_complex, FFTW_PATIENT);
  }

  fftw_export_wisdom_to_filename("fftw.wis");
}

void FFTT::resize(size_t n) {
  // need to fftw_free previous buffer
  for (fft_tag& t : data_.Raw()) {
    delete[] t.fft;
    t.fft = NULL;
    fftw_free(t.fft_complex);
    t.fft = NULL;
  }
  data_.resize(n);
  for (fft_tag& t : data_.Raw()) {
    t.fft = new double[x_sz_ * y_sz_];
    t.fft_complex = fftw_alloc_complex(fft_sz_);
    t.plan = fftw_plan_dft_r2c_2d(y_sz_, x_sz_, t.fft, t.fft_complex, FFTW_PATIENT);
  }
  ExecNode::resize(n);
}

FFTT::~FFTT() {
  for (fft_tag& t : data_.Raw()) {
    delete[] t.fft;
    t.fft = NULL;
    fftw_free(t.fft_complex);
    t.fft = NULL;
  }
}

void FFTT::SubWindow2Point(int x0, int y0, int x1, int y1) {
  std::lock_guard<std::mutex> lock(mutex_);
  subwin_x_ = x0 <= x1 ? x0 : x1;
  subwin_y_ = y0 <= y1 ? y0 : y1;
  subwin_x_sz_ = abs(x0 - x1);
  subwin_y_sz_ = abs(y0 - y1);
}

void* FFTT::Exec(void* data) {
  Frame* fr = (Frame*)data;
  Tag& t = data_.Alloc();
  time_t t1 = Component::SteadyClockTimeMs();

  memset(t.fft, 0, sizeof(double) * x_sz_ * y_sz_);

  for (int j = subwin_y_; j < subwin_y_ + subwin_y_sz_; ++j) {
    for (int i = subwin_x_; i < subwin_x_ + subwin_x_sz_; ++i) {
      t.fft[i + j * x_sz_] = fr->data[i + j * x_sz_];
    }
  }

  fftw_execute_dft_r2c(t.plan, t.fft, t.fft_complex);

  double scale = 1.0 / (fr->width * fr->height);
  for (int i = 0; i < fft_sz_; ++i) {
    t.fft[i] = (t.fft_complex[i][0] * t.fft_complex[i][0] +
                t.fft_complex[i][1] * t.fft_complex[i][1]) *
               scale;
  }

  t.x = subwin_x_;
  t.y = subwin_y_;
  t.x_sz = subwin_x_sz_;
  t.y_sz = subwin_y_sz_;

  t.fft_zero = sqrt(t.fft[0] / (t.x_sz * t.y_sz));

  t.ms = Component::SteadyClockTimeMs() - t1;

  fr->AddTag(&t);
  return (void*)fr;
}

void FFTT::AtExit(void* data) {
  data_.Free(((Frame*)data)->GetTag<Tag>());
}

FFTTDraw::FFTTDraw() {}

FFTTDraw::FFTTDraw(const Frame* fr) { Init(fr->width, fr->height); }

FFTTDraw::FFTTDraw(int x_sz, int y_sz) { Init(x_sz, y_sz); }

FFTTDraw::~FFTTDraw() {}

void FFTTDraw::Resize(int x_sz, int y_sz) { Init(x_sz, y_sz); }

void FFTTDraw::Resize(const Frame* fr) { Init(fr->width, fr->height); }

void FFTTDraw::Init(int x_sz, int y_sz) {
  fft_x_sz_ = (x_sz / 2 + 1);
  fft_y_sz_ = y_sz;
  fft_sz_ = fft_x_sz_ * fft_y_sz_;

  width_ = x_sz;
  height_ = y_sz;

  m_ = 2.5;
  x0_ = 0;
  compute_log_ = true;
  max_ = 0;

  px_.resize(fft_x_sz_ * fft_y_sz_, 0xFF000000);
  FFTWDraw::Resize(x_sz, y_sz);
}

void FFTTDraw::SetScale(bool compute_log, double min, double max) {
  compute_log_ = compute_log;
  scale_min_ = min;
  scale_max_ = max;
  if (compute_log_) {
    x0_ = log(min);
    m_ = 255.0 / (log(max + 1) - log(min + 1));
  } else {
    x0_ = min;
    m_ = 255.0 / (max - min);
  }
}

void FFTTDraw::GetScale(bool* compute_log, double* min, double* max) {
  if (compute_log) *compute_log = compute_log_;
  if (min) *min = scale_min_;
  if (max) *max = scale_max_;
}

double FFTTDraw::MaxPower() { return max_; }

void* FFTTDraw::Exec(void* data) {
  double* fft = FFTT::GetTag((Frame*)data)->fft;
  assert(fft);

  if (!wr_lock_.try_lock()) return data;
  for (int i = 0; i < fft_sz_; ++i) {
    double x = fft[i];
    if (compute_log_) x = log(x + 1);
    x = m_ * (x - x0_);
    if (x > 255) x = 255;
    if (x < 0) x = 0;
    memset(&px_[i], int(x), 3);
  }

  FFTWDraw::Update(px_);

  wr_lock_.unlock();
  return data;
}

void FFTTSubWindowDraw::Resize(const Frame* fr) {
  Resize(fr->width, fr->height);
}

void FFTTSubWindowDraw::Resize(int x_sz, int y_sz) {
  width_ = x_sz;
  height_ = y_sz;
  vertex_ = sf::VertexArray(sf::LineStrip, 5);

  // don't draw when the whole frame is selected
  vertex_[0] = sf::Vector2f(0, 0);
  vertex_[1] = sf::Vector2f(0, 0);
  vertex_[2] = sf::Vector2f(0, 0);
  vertex_[3] = sf::Vector2f(0, 0);
  vertex_[4] = sf::Vector2f(0, 0);

  for (int i = 0; i < 5; ++i) {
    vertex_[i].color = sf::Color::Blue;
  }

  Zoomable::SetDrawable(x_sz, y_sz, std::vector<sf::Drawable*>({&vertex_}));
}

void* FFTTSubWindowDraw::Exec(void* data) {
  fft_tag* fft = FFTT::GetTag((Frame*)data);
  assert(fft);

  if (subwin_x_ == fft->x && subwin_y_ == fft->y && subwin_x_sz_ == fft->x_sz &&
      subwin_y_sz_ == fft->y_sz) {
    return data;
  }

  subwin_x_ = fft->x;
  subwin_y_ = fft->y;
  subwin_x_sz_ = fft->x_sz;
  subwin_y_sz_ = fft->y_sz;

  std::lock_guard<std::mutex> lock(Zoomable::lock_);
  if (subwin_x_ == 0 && subwin_y_ == 0 && subwin_x_sz_ == width_ &&
      subwin_y_sz_ == height_) {
    // don't draw when the whole frame is selected
    vertex_[0].position = sf::Vector2f(0, 0);
    vertex_[1].position = sf::Vector2f(0, 0);
    vertex_[2].position = sf::Vector2f(0, 0);
    vertex_[3].position = sf::Vector2f(0, 0);
    vertex_[4].position = sf::Vector2f(0, 0);
  } else {
    vertex_[0].position = sf::Vector2f(float(subwin_x_), float(subwin_y_));
    vertex_[1].position = sf::Vector2f(float(subwin_x_ + subwin_x_sz_), float(subwin_y_));
    vertex_[2].position =
        sf::Vector2f(float(subwin_x_ + subwin_x_sz_), float(subwin_y_ + subwin_y_sz_));
    vertex_[3].position = sf::Vector2f(float(subwin_x_), float(subwin_y_ + subwin_y_sz_));
    vertex_[4].position = sf::Vector2f(float(subwin_x_), float(subwin_y_));
  }

  return data;
}
