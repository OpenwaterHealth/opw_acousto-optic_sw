#include "system/component/inc/invertroi.h"


InvertROI::~InvertROI() {
  for (Tag& t : pool_.Raw()) {
    fftw_free(t.fft);
    t.fft = NULL;
    fftw_free(t.ifft);
    t.ifft = NULL;
  }
}

void InvertROI::Resize(int width, int height) {
  width_ = width;
  height_ = height;

  int ret = fftw_import_wisdom_from_filename("fftw.wis");
  if (!ret) {
    printf("No FFT wisdom found, computing.  This can take several minutes\n");
  }

  // first time initialiaztion
  if (pool_.size() == 0) {
    // TODO(carsten) ExecNode defaults to 10 concurrent frames, but this isn't
    //   enforced.  A better approach would be to force the user to specify the
    //   number of concurrent frames with a resize(), or better yet, rename
    //   resize() to something better
    pool_.resize(10);
  } else {
    for (Tag& t : pool_.Raw()) {
      fftw_free(t.fft);
      fftw_free(t.ifft);
    }
  }

  for (Tag& t : pool_.Raw()) {
    t.fft = fftw_alloc_complex((width_ / 2 + 1) * height_);
    t.ifft = fftw_alloc_real(width_ * height_);
    t.plan =
        fftw_plan_dft_c2r_2d(height_, width_, t.fft, t.ifft, FFTW_PATIENT);
  }

  fftw_export_wisdom_to_filename("fftw.wis");

  roi_.Resize(width_, height_);
}

void InvertROI::resize(size_t sz) {
  for (Tag& t : pool_.Raw()) {
    fftw_free(t.fft);
    fftw_free(t.ifft);
  }

  pool_.resize(sz);

  for (Tag& t : pool_.Raw()) {
    t.fft = fftw_alloc_complex((width_ / 2 + 1) * height_);
    t.ifft = fftw_alloc_real(width_ * height_);
    t.plan = fftw_plan_dft_c2r_2d(height_, width_, t.fft, t.ifft, FFTW_PATIENT);
  }

  ExecNode::resize(sz);
}

void InvertROI::Set(double x_c, double y_c, double r) { roi_.Set(x_c, y_c, r); }

void InvertROI::SetPixels(int x_c, int y_c, int r) {
  double r_scale = width_ > height_ ? width_ : height_;
  Set(x_c / (double)(width_ / 2), y_c / (double)(height_ / 2),
      r / (double)(r_scale / 2));
}

void* InvertROI::Exec(void* data) {
  Frame* fr = (Frame*)data;
  fft_tag* fft = FFTT::GetTag(fr);
  Tag* t = &pool_.Alloc();

  assert(fft);
  assert(fr->width == width_);
  assert(fr->height == height_);

  // set the input FFT to 0, except in the ROI
  // fftw_complex has no assignment operator
  memset(t->fft, 0, sizeof(fftw_complex) * (width_ / 2 + 1) * height_);
  for (int i : roi_) {
    memcpy(&t->fft[i], &fft->fft_complex[i], sizeof(fftw_complex));
  }

  fftw_execute_dft_c2r(t->plan, t->fft, t->ifft);

  t->mean = 0;
  for (int i = 0; i < width_ * height_; ++i) {
    // rescale FFT
    t->ifft[i] /= (width_ * height_);
    t->mean += t->ifft[i];
  }
  t->mean /= (width_ * height_);

  t->stddev = 0;
  for (int i = 0; i < width_ * height_; ++i) {
    double var = t->ifft[i] - t->mean;
    t->stddev += var * var;
  }
  t->stddev /= (double)(width_ * height_);
  t->stddev = sqrt(t->stddev);

  fr->AddTag(t);

  return data;
}

void InvertROI::AtExit(void* data) {
  pool_.Free(GetTag((Frame*)data));
}
