#include "system/component/inc/filterdev.h"

#include <fstream>

#include "system/third_party/json-develop/single_include/nlohmann/json.hpp"
using json = nlohmann::json;

void FilterDev::Init(int width, int height) {
  std::unique_lock<std::shared_mutex> lock(mutex_);
  pool_.resize(30);
  dead_index_.clear();
  width_ = width;
  height_ = height;
  n_ = width_ * height_;
  size_ = n_;
}

bool FilterDev::LoadFlatfieldImage(const std::string& fname) {
  if (flatfieldFrame_.Read(fname.c_str()) == -1) {
    printf("Could not read flatfield data: %s\n", fname.c_str());
    return false;
  }
  return true;
}

bool FilterDev::LoadBadPixelImage(const std::string& fname) {
  if (badPixelFrame_.Read(fname.c_str()) == -1) {
    printf("Could not read bad pixel data: %s\n", fname.c_str());
    return false;
  }
  return true;
}

bool FilterDev::SetGainConstant(double k) {
  gainConst_ = k;
  return true;
}

bool FilterDev::LoadPixelData(const std::string& fname, int camera_id,
                              int filter_pixels) {
  std::unique_lock<std::shared_mutex> lock(mutex_);

  std::ifstream f(fname);
  if (f.fail()) {
    printf("Could not read pixel data: %s\n", fname.c_str());
    return false;
  }

  const json px_data = json::parse(f);
  f.close();

  std::string id = std::to_string(camera_id);

  if (!px_data.contains(id)) {
    printf("No data found for camera: %d\n", camera_id);
  }

  if (px_data[id].size() < filter_pixels) {
    printf(
        "Requested number of pixels to discard exceeds pixels in database\n");
    return false;
  }

  dead_index_.clear();
  dead_index_.reserve(filter_pixels);
  n_ = width_ * height_;
  const json px = px_data[id];

  for (int i = 0; i < filter_pixels; ++i) {
    if (px[i][0] >= width_ || px[i][1] >= height_) continue;

    dead_index_.push_back(px[i][0] + px[i][1] * width_);
    --n_;
  }

  std::sort(dead_index_.begin(), dead_index_.end());
  return true;
}

void FilterDev::resize(size_t frames) {
  std::unique_lock<std::shared_mutex> lock(mutex_);
  pool_.resize(frames);
  ExecNode::resize(frames);
}

void* FilterDev::Exec(void* data) {
  std::shared_lock<std::shared_mutex> lock(mutex_);

  Tag* tag = &pool_.Alloc();
  Frame* fr = (Frame*)data;

  assert(size_ == fr->width * fr->height);

  // Use full size mask for dead pixel (binary) and flat field masking
  // Dead pixel [applied to bright and dark frames]
  std::vector<uint16_t> data_deadPixelFiltered;  // Filter out dead pixels for all images
  data_deadPixelFiltered.reserve(size_);
  uint32_t numZeros = 0;
  for (int i = 0; i < size_; i++) {
    data_deadPixelFiltered.push_back(fr->data[i] * badPixelFrame_.data[i]);
    if (badPixelFrame_.data[i] == 0) {
      numZeros++;
    }
  }

  // If dark image, compute std, mean tags here, else if bright image do offset and flatfield correction
  int64_t sumFiltered = 0;
  for (int i = 0; i < size_; ++i) {
    sumFiltered += data_deadPixelFiltered[i];
  }

  double meanFiltered = (double)sumFiltered / ((double)size_ - (double)numZeros);

  // Dark image
  if ((fr->seq % 2 == 0)) {
    darkMean_ = meanFiltered;  // Store dark mean to correct next incoming bright frame
    tag->mean = darkMean_;

    darkVar_ = 0;
    for (int i = 0; i < size_; ++i) {
      if (data_deadPixelFiltered[i] != 0) {
        double var = data_deadPixelFiltered[i] - darkMean_;
        darkVar_ += var * var;
      }
    }
    darkVar_ /= (double)(size_ - numZeros);
    tag->stddev = sqrt(darkVar_);
    tag->saturated = 0;
    fr->AddTag(tag);
    return data;
  } else {
    // If bright image, do offset and flatfield corrections
    // Offset [subtract mean of dark image] and flatfield correction [divide by flatfield image]
    std::vector<double> data_offsetAndFlatfieldFiltered;  // Offset by dark mean and apply flatfield correction
    data_offsetAndFlatfieldFiltered.reserve(size_);
    numZeros = 0;
    double sum = 0;
    int saturated = 0;
    int saturatedVal = (1 << fr->bits) - 1;
    for (int i = 0; i < size_; i++) {
      if (data_deadPixelFiltered[i] != 0 && flatfieldFrame_.data[i] != 0) {
        double filteredVal = (data_deadPixelFiltered[i] - darkMean_) / ((flatfieldFrame_.data[i]) * (1.0/1023.0));
        data_offsetAndFlatfieldFiltered.push_back(filteredVal);  // TODO(low light deep dive): consider if there's a case where this will go negative (unlikely)
        sum += filteredVal;
        if (data_deadPixelFiltered[i] == saturatedVal) {
          saturated++;
        }
      } else {
        numZeros++;
      }
    }

    // Compute std/mean tags
    int dataPoints = data_offsetAndFlatfieldFiltered.size();
    tag->mean = sum / (double)dataPoints;
    double stddev = 0;
    for (int i = 0; i < dataPoints; i++) {
      double var = data_offsetAndFlatfieldFiltered[i] - tag->mean;
      stddev += var * var;
    }

    stddev /=(double)dataPoints;

    tag->stddev = sqrt(stddev - darkVar_ - gainConst_ * tag->mean);  // Filtered stats
    tag->saturated = saturated;
    fr->AddTag(tag);
    return data;
  }
}

void FilterDev::AtExit(void* data) {
  pool_.Free(((Frame*)data)->GetTag<Tag>());
}
