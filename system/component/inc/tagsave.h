#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <cstdio>
#include <mutex>
#include <string>

#include "system/component/inc/frame.h"
#include "system/component/inc/execnode.h"

// Save tags on a frame to a single CSV file
class TagSave : public ExecNode {
 public:
  TagSave() {}
  ~TagSave();

  // Set the csv file name
  // @param fname file to write (include extension, ie .csv)
  static void SetFileName(std::string fname);

 private:
  static FILE* file_;
  static bool write_header_;
  static std::mutex mutex_;

  void* Exec(void* data) override;

  template <typename T>
  void WriteHeader(const Frame* fr);

  template <typename T>
  void WriteCSV(const Frame* fr);
};
