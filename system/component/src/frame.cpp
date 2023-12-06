#include "system/component/inc/frame.h"

#include <cstring>
#include <assert.h>

#include "system/component/inc/time.h"
#include "system/third_party/inc/tiffio.h"
#include "system/third_party/inc/tiff.h"

#ifdef _MSC_VER
#pragma comment (lib, "tiff.lib")
#endif

// shift right/left based on sign
static inline uint8_t sh(uint16_t in, int shift) {
  if (shift >= 0) {
    return in >> shift;
  } else {
    return in << (-shift);
  }
}

class RealTiff: public TiffInterface {
 public:
  RealTiff() {}
  ~RealTiff() {}

  bool Open(const char* file_name, const char* mode) {
    tiff_ = TIFFOpen(file_name, mode);
    return tiff_ != NULL;
  }

  void Close() { TIFFClose(tiff_); tiff_ = NULL; }

  int GetField(ttag_t tag, int* vp) { return TIFFGetField(tiff_, tag, vp); }
  int GetField(ttag_t tag, uint16_t* vp) { return TIFFGetField(tiff_, tag, vp); }
  int SetField(ttag_t tag, int value) { return TIFFSetField(tiff_, tag, value); }

  int ReadScanline(tdata_t buf, uint32 row) { return TIFFReadScanline(tiff_, buf, row); }
  int WriteScanline(tdata_t buf, uint32 row) { return TIFFWriteScanline(tiff_, buf, row); }

  int WriteDirectory() { return TIFFWriteDirectory(tiff_); }

 private:
  TIFF* tiff_;
};

void Frame::Init() {
  data = NULL;
  bits = 0;
  width = 0;
  height = 0;
  seq = 0;
  serialNumber = -1;
  timestamp_ms_ = 0;

  err = Frame::OKAY;
  tiff_ = NULL;
}


// MATLAB (and mac TIFF viewers) uses a particularly strange bit/byte order
// for bits != 16, they are read in MSbit first, crossing byte boundaries as needed.
// ie, 10-bit pixels a[9:0], b[9:0], c[9:0], and d[9:0] would be stored
// byte 0: a9 a8 a7 a6 a8 a4 a3 a2
// byte 1: a1 a0 b9 b8 b7 b6 b5 b4
// byte 2: b3 b2 b1 b0 c9 c8 c7 c6
// byte 3: c5 c4 c3 c2 c1 c0 d9 d8
// byte 4: d7 d6 d5 d4 d3 d2 d1 d0
// ... and so on (next byte would be e[9:2])
//
// however, for 16-bit numbers, they are little-endian hwords.
// ie. 16-bit pixels a[15:0], b[15:0]
// byte 0: a7  a6  a5  a4  a3  a2  a1 a0
// byte 1: a15 a14 a13 a12 a11 a10 a9 a8
// byte 2: b7  b6  b5  b4  b3  b2  b1 b0
// byte 3: b15 b14 b13 b12 b11 b10 b9 b8
// ... and so on
//
// for 8-bit numbers, these are equivalent
//
// MATLAB is used widely, so we conform to its specification
void Frame::Load() {
  // Assumes InitTiff() has been called.
  if (bits == 16) {
    for (int j = 0; j < height; ++j) {
      tiff_->ReadScanline((*this)[j], j);  // TODO(carsten): Check return.
    }
  } else {
    int line_len = width * bits / 8;
    if (width * bits % 8 != 0) ++line_len;
    uint8_t* line_data = new uint8_t[line_len];  // TODO(carsten): Check return.

    for (int j = 0; j < height; ++j) {
      tiff_->ReadScanline(line_data, j);  // TODO(carsten): Check return.
      uint8_t* line_ptr = line_data;
      uint64_t buf = 0;
      int i = 0;
      int shift = 0;

      while (i < width) {
        buf = buf << 8 | *(line_ptr++);
        shift += 8;
        while (shift - bits >= 0 && i < width) {
          data[i + j * width] = uint16_t(buf >> (shift - bits));
          buf = buf & ((1 << (shift - bits)) - 1);
          shift -= bits;
          ++i;
        }
      }
    }
    delete[] line_data;
  }
}

Frame::Frame(const char* fname): data(NULL), width(0), height(0) {
  Init();
  if (!Read(fname)) {
    printf("ERROR: Frame(%s)", fname);
  }
}


Frame::Frame() {
  Init();
}


Frame::Frame(int w, int h) {
  Init();
  width = w;
  height = h;
  bits = 16;
  data_.resize(w * h);
  data = data_.data();
}


Frame::Frame(const Frame& fr) {
  bits = fr.bits;
  width = fr.width;
  height = fr.height;
  seq = fr.seq;
  serialNumber = fr.serialNumber;
  timestamp_ms_ = fr.timestamp_ms_;
  tags_ = fr.tags_;
  err = fr.err;
  if (fr.data) {
    data_ = fr.data_;
  } else {
    data_.resize(width * height);
  }
  data = data_.data();
  tiff_ = NULL;  // Do not copy this!
}


const Frame& Frame::operator=(const Frame& fr) {
  bits = fr.bits;
  width = fr.width;
  height = fr.height;
  seq = fr.seq;
  serialNumber = fr.serialNumber;
  timestamp_ms_ = fr.timestamp_ms_;
  tags_ = fr.tags_;
  err = fr.err;
  if (fr.data) {
    data_ = fr.data_;
  } else {
    data_.resize(width * height);
  }
  data = data_.data();
  tiff_ = NULL;  // Do not copy this!

  return *this;
}


Frame::~Frame() {
  data = NULL;
  delete tiff_;
  tiff_ = NULL;
}


uint16_t* Frame::operator[](int col_idx) {
  return data + col_idx * width;
}


void Frame::SetTimestamp() {
  timestamp_ms_ = Component::SteadyClockTimeMs();
}


void Frame::InitTiff() {
  // If you cannot afford a TiffInterface, one will be appointed for you ...
  if (!tiff_) {
    tiff_ = new RealTiff();
  }
}


int Frame::Read(const char* fname) {
  InitTiff();
  if (!tiff_->Open(fname, "r")) {
    return -1;
  }

  int w, h;
  uint16_t b;
  tiff_->GetField(TIFFTAG_IMAGEWIDTH, &w);
  tiff_->GetField(TIFFTAG_IMAGELENGTH, &h);
  tiff_->GetField(TIFFTAG_BITSPERSAMPLE, &b);
  if (!data) {
    width = w, height = h, bits = b;
    data_.resize(width * height);
    data = data_.data();
  } else if (w != width || h != height || b != bits) {
    return -1;
  }

  Load();  // TODO(carsten): Check return. (Needs a return code.)
  return 0;
}


int Frame::Write(const char* fname) {
  InitTiff();
  if (!tiff_) return -1;
  if (!tiff_->Open(fname, "w")) return -1;

  class stackTiff {  // Close on destruction.
   public:
    stackTiff(TiffInterface* tiff): t(tiff) {}
    ~stackTiff() { t->Close(); }
    TiffInterface* t;
  };
  stackTiff st(tiff_);

  tiff_->SetField(TIFFTAG_IMAGEWIDTH, width);
  tiff_->SetField(TIFFTAG_IMAGELENGTH, height);
  tiff_->SetField(TIFFTAG_BITSPERSAMPLE, bits);
  tiff_->SetField(TIFFTAG_COMPRESSION, COMPRESSION_NONE);
  tiff_->SetField(TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
  tiff_->SetField(TIFFTAG_ORIENTATION, static_cast<int>(ORIENTATION_TOPLEFT));
  tiff_->SetField(TIFFTAG_SAMPLESPERPIXEL, 1);
  tiff_->SetField(TIFFTAG_ROWSPERSTRIP, 1);
  tiff_->SetField(TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  tiff_->SetField(TIFFTAG_FILLORDER, FILLORDER_MSB2LSB);
  tiff_->SetField(TIFFTAG_MINSAMPLEVALUE, 0);
  tiff_->SetField(TIFFTAG_MAXSAMPLEVALUE, (1 << bits) - 1);

  if (bits == 16) {
    for (int j = 0; j < height; ++j) {
      if (tiff_->WriteScanline((*this)[j], j) != 1) return -1;
    }
  } else {
    int line_len = width * bits / 8;
    if (width * bits % 8 != 0) ++line_len;
    uint8_t* line_data = new uint8_t[line_len];
    for (int j = 0; j < height; ++j) {
      uint8_t* line_ptr = line_data;
      memset(line_data, 0, line_len);
      int shift = 0;
      for (int i = 0; i < width; ++i) {
        shift += bits;
        while (true) {
          *line_ptr |= sh(data[i + width * j], shift - 8);
          if (shift >= 8) {
            ++line_ptr;
            shift -= 8;
            if (shift == 0) break;
          }
          else {
            break;
          }
        }
      }
      if (tiff_->WriteScanline(line_data, j) != 1) return -1;
    }
    delete[] line_data;
  }

  if (tiff_->WriteDirectory() != 1) return -1;
  return 0;  // stackTiff closes
}
