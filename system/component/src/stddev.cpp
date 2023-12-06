#include "system/component/inc/stddev.h"

#include <cmath>


void* StdDev::Exec(void* data) {
  Frame* fr = (Frame*)data;
  Tag* tag = &pool_.Alloc();

  // use int64_t to not lose precision on large sums
  // int32_t will overflow for 5MP @ 10bit images
  int size = fr->width * fr->height;
  int64_t sum = 0;
  for (int i = 0; i < size; ++i) {
    sum += fr->data[i];
  }
  tag->mean = (double)sum / (double)size;

  tag->stddev = 0;
  for (int i = 0; i < size; ++i) {
    double var = fr->data[i] - tag->mean;
    tag->stddev += var * var;
  }
  tag->stddev /= (double)size;
  tag->stddev = sqrt(tag->stddev);

  fr->AddTag(tag);
  return data;
}

void StdDev::AtExit(void* data) {
  pool_.Free(GetTag((Frame*)data));
}

void StdDev::resize(size_t size) {
  pool_.resize(size);
  ExecNode::resize(size);
}
