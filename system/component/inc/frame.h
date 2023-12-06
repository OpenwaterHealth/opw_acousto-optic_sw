#pragma once

#include <array>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

#include "system/component/inc/TiffInterface.h"

// Structure containing monochrome frames
//   data: column major array of pixel data, one pixel per element
//   bits: bit depth of the image
//   width: width of the image in pixels
//   height: height of the image in pixels
//   seq: sequence number of the image, starting at 0 for the first image acquired in a series
//   timestamp: time the image was acquired (in ms, from SteadyClockTimeMs())
//     - At some point, if we need more resolution, for higher frame rates, we can switch to usec.
//   err: errors occured while capturing this frame
//   tag list of IDs and pointers to data structures representing post-processing for this frame
//     examples: FFT, histogram
//     id is a pointer to a unique memory address that identifies the data structure
//     data is a pointer to the data structure itself
class Frame {
 public:
  uint16_t* data;
  int bits;
  int width;
  int height;
  int seq;
  int serialNumber;
  double temperature;
  time_t timestamp_ms_;

  int err;
  static const int OKAY = 0;
  static const int ERR_OVERFLOW = 1 << 0;   // more data in frame than frame size
  static const int ERR_INCOMPLETE = 1 << 1; // incomplete frame
  static const int ERR_PACKET = 1 << 2;     // malformed data packet
  static const int ERR_ORDER = 1 << 3;      // out of order frame

  Frame();

  // Create frame object from file
  // frame data will be NULL if unscucessful
  Frame(const char* fname);

  // Create frame with memory allocated for a
  // frame of size width and height
  // @param width width of the frame
  // @param height height of the frame
  Frame(int width, int height);

  // Copy constructor
  // Allocates memory and copies frame data from the original frame
  Frame(const Frame& fr);

  ~Frame();

  // Assignment operator
  const Frame& operator=(const Frame& fr);

  // Write frame to file
  int Write(const char* fname);

  // Read frame from file
  // Frame objects must match width, length, and bit depth
  int Read(const char* fname);

  // Access a row of frame data
  uint16_t* operator[](int col_idx);

  // Set timestamp to the current time.
  void SetTimestamp();

  // Tag structure to add information to a frame
  struct Tag {
    virtual ~Tag() {}
  };

  // Add a tag to a frame
  void AddTag(Tag* tag) {
    std::lock_guard<std::mutex> lock(mutex_);
    tags_.push_back(tag);
  }

  // Get a tag from a frame
  // @note T must inherit from Frame::Tag
  template <typename T>
  T* GetTag() const {
    for (Tag* tag : tags_) {
      if (T* t = dynamic_cast<T*>(tag)) return t;
    }
    return (T*)NULL;
  }

  void ClearTags() {
    std::lock_guard<std::mutex> lock(mutex_);
    tags_.clear();
  }

 private:
  // Initalize the frame with zero data
  void Init();

  // Load a frame from disk.
  void Load();

  std::mutex mutex_;

  std::vector<Tag*> tags_;

  // TODO(carsten): investigate changing Frame::data to std::vector
  std::vector<uint16_t> data_;

 protected:
  // Create a default TiffInterface for file i/o. (Called lazily.)
  void InitTiff();

  // Make this injectable, for testing, but it doesn't have to be passed in for every Frame().
  TiffInterface* tiff_;
};
