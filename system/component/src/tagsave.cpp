#include "system/component/inc/tagsave.h"

#include "system/component/inc/filterdev.h"
#include "system/component/inc/fftt.h"
#include "system/component/inc/invertroi.h"
#include "system/component/inc/roi.h"
#include "system/component/inc/stddev.h"

FILE* TagSave::file_ = stdout;
bool TagSave::write_header_ = true;
std::mutex TagSave::mutex_;


template <typename T>
static const char* header = "";
template <typename T>
static void WriteCSV(const T* tag, FILE* file) {}

// Frame
template <>
static const char* header<Frame> =
    "Camera Number, Frame Number, Timestamp (ms), Temperature (C)";
template <>
static void WriteCSV<Frame>(const Frame* fr, FILE* file) {
  fprintf(file, "%d, %d, %ld, %.1lf", fr->serialNumber, fr->seq, fr->timestamp_ms_, fr->temperature);
}

// FFTT
template <>
static const char* header<FFTT::Tag> = ", FFT Time(ms), FFTT Zero";
template <>
static void WriteCSV<FFTT::Tag>(const FFTT::Tag* tag, FILE* file) {
  fprintf(file, ", %d, %.3le", tag->ms, tag->fft_zero);
}

// ROI
template <>
static const char* header<ROI::Tag> = ", ROI, ROU";
template <>
static void WriteCSV<ROI::Tag>(const ROI::Tag* tag, FILE* file) {
  fprintf(file, ", %.3le, %.3le", tag->roi, tag->rou);
}

// StdDev
template <>
static const char* header<StdDev::Tag> = ", Mean, Standard Deviation";
template <>
static void WriteCSV<StdDev::Tag>(const StdDev::Tag* tag, FILE* file) {
  fprintf(file, ", %.3le, %.3le", tag->mean, tag->stddev);
}

// invert ROI
template <>
static const char* header<InvertROI::Tag> = ", ROI Mean, ROI Standard Deviation";
template <>
static void WriteCSV<InvertROI::Tag>(const InvertROI::Tag* tag, FILE* file) {
  fprintf(file, ", %.3le, %.3le", tag->mean, tag->stddev);
}

// FilterDev
template <>
static const char* header<FilterDev::Tag> =
    ", Filtered Mean, Filtered Standard Deviation";
template <>
static void WriteCSV<FilterDev::Tag>(const FilterDev::Tag* tag, FILE* file) {
  fprintf(file, ", %.3le, %.3le", tag->mean, tag->stddev);
}

template <typename T>
void TagSave::WriteHeader(const Frame* fr) {
  if (fr->GetTag<T>()) {
    fprintf(file_, header<T>);
  }
}

template <typename T>
void TagSave::WriteCSV(const Frame* fr) {
  if (T* tag = fr->GetTag<T>()) {
    ::WriteCSV<T>(tag, file_);
  }
}


void TagSave::SetFileName(std::string fname) {
  file_ = fopen(fname.c_str(), "wx");
  if (file_ == NULL) {
    printf("Could not open csv for writing: %s\n", fname.c_str());
    exit(1);
  }
}

void* TagSave::Exec(void* data) {
  Frame* fr = (Frame*)data;
  std::lock_guard<std::mutex> lock(mutex_);

  if (write_header_) {
    fprintf(file_, header<Frame>);
    WriteHeader<FFTT::Tag>(fr);
    WriteHeader<ROI::Tag>(fr);
    WriteHeader<StdDev::Tag>(fr);
    WriteHeader<InvertROI::Tag>(fr);
    WriteHeader<FilterDev::Tag>(fr);
    fputc('\n', file_);
    write_header_ = false;
  }

  // always write frame stats
  ::WriteCSV<Frame>(fr, file_);
  WriteCSV<FFTT::Tag>(fr);
  WriteCSV<ROI::Tag>(fr);
  WriteCSV<StdDev::Tag>(fr);
  WriteCSV<InvertROI::Tag>(fr);
  WriteCSV<FilterDev::Tag>(fr);
  fputc('\n', file_);

  if (ferror(file_)) {
    printf("Error writing frame tags\n");
    exit(1);
  }

  return data;
}

TagSave::~TagSave() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (file_) {
    fclose(file_);
    file_ = NULL;
  }
}
