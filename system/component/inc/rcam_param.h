#ifndef __RCAM_PARAM_H_
#define __RCAM_PARAM_H_

// start streaming MIPI data from CMOS sensor to USB bulk endpoint
// wLength must be 0
#define RQ_MIPI_START 0x76

// stop streaming MIPI data from CMOS sensor to USB bulk endpoint
// wLength must be zero
#define RQ_MIPI_STOP 0x77

// read bytes from CMOS sensor I2C configuration
// wLength is number of bytes to read
// wValue is the 7-bit I2C address
// wIndex is 16-bit register address start
#define RQ_I2C_READ 0x71

// write bytes to CMOS sensor I2C configuration
// wLength is number of bytes to write
// wValue is the 7-bit I2C address
// wIndex is 16-bit register address start
#define RQ_I2C_WRITE 0x72

// read CMOS sensor configuration parameters from CX3
// wLength must match sizeof(RcamParam)
#define RQ_PARAM_READ 0x73

// set frame count
#define RQ_SET_FRAME_COUNT 0x74

// Soft reset.  Will also stop streaming
#define RQ_RESET 0xE0

// Request serial number
#define RQ_SERIAL 0xE1

#define CMOS_ADDR 0x24

#define PACKET_HEADER_LEN 0x10
#define PACKET_HEADER_MAGIC_POS 0
#define PACKET_HEADER_MAGIC_LEN 8
static const char PACKET_HEADER_MAGIC[] = "\xFF\xFF\x00\x00\x55\xAA\x5A\xA5";
#define PACKET_EOF_POS 8
#define PACKET_EOF_VAL (1 << 0)
#define FRAME_VALID_POS 8
#define FRAME_VALID_VAL (1 << 1)
#define FRAME_COUNT_POS 12
#define FRAME_COUNT_BYTES 4

typedef uint32_t frame_count_t;

typedef struct {
  // camera model
  char model[20];
  
  // image width
  int32_t width;
  
  // image height
  int32_t height;
  
  // bits per pixel in an image
  int32_t bits;
  
  
  // frames per millisecond
  int32_t fpms;
  
  // sensor pixel clock in Hz
  int32_t px_clk;
} RcamParam;


#endif // __RCAM_PARAM_H_
