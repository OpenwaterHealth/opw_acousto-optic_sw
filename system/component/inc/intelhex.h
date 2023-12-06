#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>


// Simple Intel Hex file writer
// Data must be written in order (ie, low address to high)
class IntelHex {
 public:
  IntelHex() {}
  ~IntelHex() {}

  // Open a file to write 
  // @param fname file to open
  int Open(const char* fname);

  // Finish writing 
  int Close();

  // Set next writing address
  int SetAddress(uint32_t addr);

  // Write data to the file
  // @param data bytes to write
  // @param len number of bytes
  int Write(const uint8_t* data, size_t len);

  // Write a 32-bit value to the file
  // Assumes little-endian
  int Write32(uint32_t data);

  // Write a 16-bit value to the file
  // Assumes little-endian
  int Write16(uint16_t data);

  // Write a byte to the file
  int Write8(uint8_t data);

 private:
  // write a single line to the intel hex file, calculating checksum
  int WriteRecord(uint8_t len, uint16_t addr, uint8_t op, const uint8_t* data);

  // Intel hex files traditionaly bunch groups of bytes in 16-byte blocks
  //   While this is not a requirement, we will adhere to the standard.
  uint32_t address_;
  uint8_t partial_[16];
  int partial_len_;

  FILE* fptr_ = NULL;
};
