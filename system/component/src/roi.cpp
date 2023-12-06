#include "system/component/inc/roi.h"

#include <cassert>

ROI::ROI(int width, int height) { Init(width, height); }

ROI::ROI(const Frame* fr) { Init(fr->width, fr->height); }

void ROI::Init(int width, int height) {
  width_ = width;
  height_ = height;
  fft_x_sz_ = width / 2 + 1;
  fft_y_sz_ = height;
  roi_.Resize(width, height);
  rou_.Resize(width, height);
  data_.resize(10);
}

void ROI::Resize(int width, int height) { Init(width, height); }

void ROI::Resize(const Frame* fr) { Init(fr->width, fr->height); }

void ROI::Set(int x_c, int y_c, int r) {
  Set(x_c / (double)(width_ / 2), y_c / (double)(height_ / 2), r / (double)(height_ / 2));
}

void ROI::Set(double x_c, double y_c, double r) {
  std::lock_guard<std::mutex> lock(mutex_);
  x_c_ = x_c;
  y_c_ = y_c;
  r_ = r;
  roi_.Set(x_c_, y_c_, r_);
  rou_.Set(-y_c_, x_c_, r_);
}

void ROI::Get(int* x_c, int* y_c, int* r) {
  if (x_c) *x_c = int(x_c_ * width_ / 2);
  if (y_c) *y_c = int(y_c_ * height_ / 2);
  if (r) *r = int(r_ * height_ / 2);
}

void* ROI::Exec(void* data) {
  std::lock_guard<std::mutex> lock(mutex_);
  Frame* fr = (Frame*)data;
  FFTT::Tag* fft = fr->GetTag<FFTT::Tag>();
  assert(fft);

  Tag& roi = data_.Alloc();
  roi.x_c = x_c_;
  roi.y_c = y_c_;
  roi.r = r_;
  roi.roi = 0;
  roi.rou = 0;

  for (int& i : roi_) {
    roi.roi += fft->fft[i];
  }

  for (int& i : rou_) {
    roi.rou += fft->fft[i];
  }

  fr->AddTag(&roi);
  return data;
}

void ROI::resize(size_t n) {
  data_.resize(n);
  ExecNode::resize(n);
}

void ROI::AtExit(void* data) {
  data_.Free(ROI::GetTag((Frame*)data));
}

ROI::~ROI() {}

ROIDraw::ROIDraw(int width, int height) { Init(width, height); }

ROIDraw::ROIDraw(const Frame* fr) { Init(fr->width, fr->height); }

void ROIDraw::Init(int width, int height) {
  width_ = width;
  height_ = height;
  fft_x_sz_ = width / 2 + 1;
  fft_y_sz_ = height;
  r_ = 0;
  x_c_ = 0;
  y_c_ = 0;

  px_.resize(fft_x_sz_ * fft_y_sz_, 0);
  FFTWDraw::Resize(width, height);
}

void ROIDraw::Resize(int width, int height) { Init(width, height); }

void ROIDraw::Resize(const Frame* fr) { Init(fr->width, fr->height); }

void ROIDraw::Set(double x_c, double y_c, double r) {
  std::lock_guard<std::mutex> wrlock(wr_lock_);
  x_c_ = x_c;
  y_c_ = y_c;
  r_ = r;

  for (uint32_t& i : px_) {
    i = 0;
  }

  for (int& idx : FFTWCircle(x_c, y_c, r, width_, height_)) {
    px_.at(idx) = 0x40FF0000;
  }

  for (int& idx : FFTWCircle(-y_c, x_c, r, width_, height_)) {
    px_.at(idx) = 0x400000FF;
  }

  FFTWDraw::Update(px_);
}

void* ROIDraw::Exec(void* data) {
  roi_tag* roi = ROI::GetTag((Frame*)data);
  assert(roi);
  if (roi->x_c != x_c_ || roi->y_c != y_c_ || roi->r != r_) {
    Set(roi->x_c, roi->y_c, roi->r);
  }

  return data;
}
