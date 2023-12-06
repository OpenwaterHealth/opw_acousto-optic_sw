#include "system/component/inc/fftwutil.h"

#include <cassert>
#include <cmath>

int FFTWIndex(double x, double y, int width, int height) {
  return FFTWIndex((int)(x * width / 2), (int)(y * height / 2), width, height);
}


int FFTWIndex(int x, int y, int width, int height) {
  if ((x >= 0 && y <= 0) || (x < 0 && y >= 0)) {
    return abs(y) * (width / 2 + 1) + abs(x);
  } else {
    return (height - abs(y)) * (width / 2 + 1) + abs(x);
  }
}


void FFTWDraw::Resize(int width, int height) {
  width_ = width;
  height_ = height;
  fft_x_sz_ = width / 2 + 1;
  fft_y_sz_ = height;

  tx_.create(fft_x_sz_, fft_y_sz_);

  sp_[0].setTexture(tx_);
  sp_[0].setOrigin(-1, float(fft_y_sz_ / 2));
  sp_[0].setTextureRect(sf::IntRect(1, fft_y_sz_ / 2 + (fft_y_sz_ & 1),
                                    fft_x_sz_ - 1, fft_y_sz_ / 2));
  sp_[0].setPosition(float(width_ / 2), float(height_ / 2));

  sp_[1].setTexture(tx_);
  sp_[1].setOrigin(float(fft_x_sz_ - 1), float(fft_y_sz_ / 2));
  sp_[1].setTextureRect(sf::IntRect(fft_x_sz_, fft_y_sz_ / 2 + 1, -(fft_x_sz_),
                                    -(fft_y_sz_ / 2 + 1)));
  sp_[1].setPosition(float(width_ / 2), float(height_ / 2));

  sp_[2].setTexture(tx_);
  sp_[2].setOrigin(float(fft_x_sz_ - 1), -1);
  sp_[2].setTextureRect(
      sf::IntRect(fft_x_sz_, fft_y_sz_, -(fft_x_sz_ - 1), -(fft_y_sz_ / 2)));
  sp_[2].setPosition(float(width_ / 2), float(height_ / 2));

  sp_[3].setTexture(tx_);
  sp_[3].setOrigin(0, 0);
  sp_[3].setTextureRect(sf::IntRect(0, 0, fft_x_sz_, fft_y_sz_ / 2 + 1));
  sp_[3].setPosition(float(width_ / 2), float(height_ / 2));

  Zoomable::SetDrawable(
      width_, height_,
      std::vector<sf::Drawable*>({&sp_[0], &sp_[1], &sp_[2], &sp_[3]}));
}


void FFTWDraw::Resize(const Frame* fr) { Resize(fr->width, fr->height); }


int FFTWDraw::FFTWIndex(double x, double y) {
  return ::FFTWIndex(x, y, width_, height_);
}


int FFTWDraw::FFTWIndex(int x, int y) {
  return ::FFTWIndex(x, y, width_, height_);
}


void FFTWDraw::Update(const std::vector<uint32_t>& pixels) {
  assert(pixels.size() == fft_x_sz_ * fft_y_sz_);
  std::lock_guard<std::mutex> lock(Zoomable::lock_);
  tx_.update((uint8_t*)pixels.data());
}


FFTWCircle::FFTWCircle(double x, double y, double r, int width, int height) {
  Resize(width, height);
  Set(x, y, r);
}


void FFTWCircle::Resize(int width, int height) {
  width_ = width;
  height_ = height;
}


void FFTWCircle::Set(double x, double y, double r) {
  idx_.clear();
  double rr = r * r;
  double w = 2.0 / width_;
  double h = 2.0 / height_;
  for (int j = int((y - r) * (height_ / 2)); j <= (y + r) * (height_ / 2); ++j) {
    double yy = j * h - y;
    yy *= yy;
    for (int i = int((x - r) * (width_ / 2)); i <= (x + r) * (width_ / 2); ++i) {
      double xx = i * w - x;
      xx *= xx;
      if (xx + yy <= rr && i >= -width_ / 2 && i <= width_ / 2 &&
          j >= -height_ / 2 && j <= height_ / 2) {
        idx_.push_back(FFTWIndex(i, j, width_, height_));
      }
    }
  }
}
