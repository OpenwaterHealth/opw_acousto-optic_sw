#include "system/util/fftutil/saveImages/offset.h"
#include <algorithm>
#include <iostream>

void* Offset::Exec(void* data) {
  Frame* fr = (Frame*)data;

  int size = fr->width * fr->height;
  for (int i = 0; i < size; ++i) {
    fr->data[i] = std::max(fr->data[i] - offset_, 0.0);
  }
  return data;
}

double Offset::setOffset(double offset) {
  offset_ = offset;
  return offset_;
}
