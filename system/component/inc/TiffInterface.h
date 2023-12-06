#pragma once

// Wrap an interface around the TIFF library for testability.
// N.B.: Use raw types here to avoid hauling in tiff.h & friends, which define lots of cruft.

class TiffInterface {
 public:
  TiffInterface() {}
  virtual ~TiffInterface() {}

  virtual bool Open(const char* file_name, const char* mode) = 0;
  virtual void Close() = 0;

  // Specialize these for strong type checking.
  virtual int GetField(uint32_t tag, int* vp) = 0;
  virtual int GetField(uint32_t tag, uint16_t* vp) = 0;
  virtual int SetField(uint32_t, int value) = 0;

  virtual int ReadScanline(void* buf, uint32_t row) = 0;
  virtual int WriteScanline(void* buf, uint32_t row) = 0;

  virtual int WriteDirectory() = 0;
};
