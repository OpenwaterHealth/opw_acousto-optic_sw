#undef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include "system/component/inc/intelhex.h"

#include <cassert>

// intel hex op codes
static const uint8_t IH_DATA = 0x00;
static const uint8_t IH_EOF = 0x01;
static const uint8_t IH_ESA = 0x04;

#define CHECK(x) \
  {int err = x;\
  if (err < 0) return err; }

int IntelHex::Open(const char* fname) {
  fptr_ = fopen(fname, "w");
  if (fptr_ == NULL) return 1;

  address_ = 0;
  partial_len_ = 0;
  return 0;
}


int IntelHex::SetAddress(uint32_t addr) {
  // write any remaining bytes buffered but not written to file
  if (partial_len_ > 0) {
    CHECK(WriteRecord(partial_len_, address_, IH_DATA, partial_));
    partial_len_ = 0;
  }

  // check for 16-bit address boundary
  if ((addr >> 16) - (address_ >> 16) > 0) {
    uint16_t eaddr = addr >> 24;
    eaddr |= (addr >> 8) & 0xFF00;
    CHECK(WriteRecord(2, 0, IH_ESA, (uint8_t*)&eaddr));
  }

  address_ = addr;
  return 0;
}


int IntelHex::Write(const uint8_t* data, size_t len) {
  while (len >= 0) {
    size_t run = 16 - (address_ & 0xF) - partial_len_;

    if (len >= run) {
      memcpy(&partial_[partial_len_], data, run);
      partial_len_ += run;
      CHECK(WriteRecord(partial_len_, address_, IH_DATA, partial_));
      address_ += partial_len_;

      // check for 16-bit address boundary
      if ((address_ >> 16) - ((address_ - run) >> 16) > 0) {
        uint16_t eaddr = address_ >> 24;
        eaddr |= (address_ >> 8) & 0xFF00;
        CHECK(WriteRecord(2, 0, IH_ESA, (uint8_t*)&eaddr));
      }
      partial_len_ = 0;
      len -= run;
      data += run;
    } else {
      memcpy(&partial_[partial_len_], data, len);
      partial_len_ += len;
      return 0;
    }
  }

  return 0;
}


int IntelHex::Write32(uint32_t data) { return Write((uint8_t*)&data, 4); }
int IntelHex::Write16(uint16_t data) { return Write((uint8_t*)&data, 2); }
int IntelHex::Write8(uint8_t data) { return Write((uint8_t*)&data, 1); }


int IntelHex::Close() {
  if (partial_len_ > 0) {
    CHECK(WriteRecord(partial_len_, address_, IH_DATA, partial_));
    partial_len_ = 0;
  }

  CHECK(WriteRecord(0, 0, IH_EOF, NULL));

  CHECK(fclose(fptr_));
  fptr_ = NULL;
  return 0;
}


int IntelHex::WriteRecord(uint8_t len, uint16_t addr, uint8_t op,
                          const uint8_t* data) {
  char linebuf[80];
  char bytebuf[4];
  sprintf(linebuf, ":%02X%04X%02X", len, addr, op);

  uint8_t checksum = 0;
  checksum -= len;
  checksum -= addr >> 8;
  checksum -= addr & 0xFF;
  checksum -= op;

  for (int i = 0; i < len; ++i) {
    sprintf(bytebuf, "%02X", data[i]);
    strcat(linebuf, bytebuf);
    checksum -= data[i];
  }

  sprintf(bytebuf, "%02X\n", checksum);
  strcat(linebuf, bytebuf);

  return fprintf(fptr_, "%s", linebuf);
}
