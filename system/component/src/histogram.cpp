#include "system/component/inc/histogram.h"

#define SFML_STAIC
#pragma comment (lib, "sfml-graphics-s.lib")
#pragma comment (lib, "sfml-window-s.lib")
#pragma comment (lib, "sfml-system-s.lib")
#pragma comment (lib, "opengl32.lib")
#pragma comment (lib, "gdi32.lib")
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "freetype.lib")

Histogram::Histogram(int min, int max, int bucket_size) :
  min_(min), max_(max), bucket_size_(bucket_size) {
  n_ = (max_ - min_) / bucket_size_ + 1;
  data_ = new uint64_t[n_];
  this->Clear();

  draw_ = NULL;
}

Histogram::~Histogram() {
  delete[] data_;
  if (draw_) delete draw_;
}

void Histogram::Clear() {
  memset(data_, 0, sizeof(uint64_t) * n_);
}

void Histogram::Add(int p) {
  ++data_[(p - min_) / bucket_size_];
}

void Histogram::Print() {
  printf("ELT: %d\n", n_);
  for (int i = 0; i < n_; ++i) {
    printf("%lld\n", data_[i]);
  }
}

void Histogram::SetWindow(int x_sz, int y_sz, int y_max, sf::Color color) {
  if (draw_) delete draw_;

  float bucket_width = (float)x_sz / (float)(n_ - 1);
  draw_scale_ = (float)y_sz / (float)y_max;
  max_y_ = (float)y_sz;
  draw_ = new sf::VertexArray(sf::TriangleStrip, 2 * n_);
  for (int i = 0; i < 2 * n_ ; ++i) {
    (*draw_)[i].position = sf::Vector2f(float(i >> 1) * bucket_width, 0);
    (*draw_)[i].color = color;
  }
}

const sf::Drawable & Histogram::Draw() {
  for (int i = 0; i < n_; ++i) {
    float d = data_[i] * draw_scale_;
    if (d > max_y_) d = max_y_;
    (*draw_)[i << 1].position.y = d;
  }

  return *draw_;
}

Histogram::operator sf::Drawable &() {
  for (int i = 0; i < n_; ++i) {
    float d = data_[i] * draw_scale_;
    if (d > max_y_) d = max_y_;
    (*draw_)[(i << 1) + 1].position.y = d;
  }

  return *draw_;
}
