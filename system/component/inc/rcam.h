#pragma once

#include <cassert>
#include <cmath>
#include <cstdint>
#include <atomic>

#include "system/component/inc/circular_buffer.h"
#include "system/component/inc/execnode.h"
#include "system/component/inc/frame.h"
#include "system/component/inc/fx3.h"
#include "system/component/inc/rcam_param.h"

// Class to connect to cameras and read raw data
// Contains an internal circular framebuffer to store data as it comes
//   in from the device
// Two APIs exist:
// - Legacy API: Synchonous using GetFrame() or WaitFrame()
//   Rcam considers a frame "read" once GetFrame has been called to read a
//   subsequent frame (even if no subsequent frame is available), and may
//   start writing over the previous frame
// - New API: Asynchronous using ExecNodes to process data
//   Rcam will keep frames as long as subesquent ExecNodes are still
//   operating on the data.
class Rcam : public ExecNode {
 public:
  // check how many Rcams are connected to this computer
  static int NumCameras();

  Rcam() {}
  Rcam(FX3* device) { fx3_ = device; }
  ~Rcam() { Close(); }

  // open a camera
  // @param n which camera, 0 -> N-1 in a system with N cameras
  //   to connect to
  // @returns 0 on success, error code otherwise
  int Open(int n = 0);

  // close the currently open camera
  void Close();

  int SerialNumber();

  // Start the camera
  void Start();

  // Stop the camera
  void Stop();

  // Subwindow the camera, only receiving pixels in the box defined by
  // upper left: (x0, y0) lower right: (x1, y1)
  // x1 and y1 are the first pixels NOT displayed, so
  //   SubWindow2Point(0, 0, width, height) will display the entire frame,
  //   and SubWindow2Point(0, 0, 1, 1) will display nothing
  // @note Call this immediately after Open(), as it will change the
  //   configuration frame.
  void SubWindow2Point(int x0, int y0, int x1, int y1);

  // Hard reset of camera firmware
  // Will call Close() if camera is running, and unload firmware
  // Pauses for 2.5 seconds to allow USB reattach
  // Call Open() to restart communication
  // @returns 0 on success, otherwise an error code
  int Reset();

  // Set the camera to auto-stream at its programmed FPS (ignores SYNC pin)
  void SetStream(bool stream);

  // Return the next frame in the framebuffer
  // returns NULL if no new frame is available.
  Frame* GetFrame();

  // Wait for a new frame to come from the device.  Will clear all
  // previous frames from the framebuffer
  // @param timeout time in seconds to wait for frame
  //   if negative or missing, wait forever
  Frame* WaitFrame(double timeout = -1);

  // Get configuration frame
  // Frame info (height, width, bit depth) will be valid
  // Frame specifics (data, timestamp, etc..) will not
  Frame* GetConfig();

  // check number of received frames
  int GetFrameCount();

  // Set the number of received frames.
  // The next frame received will have its sequence number set to this.
  void SetFrameCount(int frames);

  // Check number of dropped frames.
  int DroppedFrames();

  // Check for errors since GetErrors() was last called
  // @returns a bitwise OR of all errors that have occurred since last call
  int GetErrors();

  // String describing the camera model (ie, "HM5530")
  const char* Model();

  // Set exposure
  // @param exp exposure time in seconds
  void SetExposure(double exp);

  // Get exposure
  // @returns exposure time in seconds
  double GetExposure();

  // Get conversion time
  // @returns conversion time in seconds
  double GetConversion();

  // Get temperature of CMOS sensor in degrees C
  double Temperature();

  // Set the analog gain
  void SetGain(double gain);

  // Turn on/off black level correction
  void SetBLC(bool blc_on);

  // Enable optically black pixel readout
  void SetOBPixels(bool obPixels_on);

  // write configuration to the device
  void WriteCFG(uint32_t reg, uint32_t val, int bytes = 1);

  // read configuration from the device
  uint32_t ReadCFG(uint32_t reg, int bytes = 1);

  // descramble pixels from the HM5511
  static void DescrambleHM5511(Frame* fr);

  // resize the buffer for a number of frames
  void resize(size_t n) override;

  // for testing
  static int pid() { return PID; }

  // for async error recovery until new rcam sw available
  void SetFramesRx(int newFramesRx);

  void EnableSave(bool saveON);

 private:
  // Underlying FX3 driver
  // Can inject another driver with Rcam(FX3* mock) constructor
  FX3 fx3_inst_;
  FX3* fx3_ = &fx3_inst_;

  static const int PID = 0x4F10;

  void AtExit(void* data) override;

  void RxThread();
  volatile enum class RxState { STOPPED, RUNNING, QUIT } rx_state_ = RxState::STOPPED;
  std::thread rx_thread_;

  RcamParam param_ = {{0}};
  Frame fr_cfg_;
  CircularBuffer<Frame> framebuf_;

  int Flash();

  // number of frames read, written, and in userspace (0 or 1)
  // in this session
  volatile int user_ = 0;
  volatile int frames_ = 0;
  volatile int frames_dropped_ = 0;
  volatile int err_ = Frame::OKAY;

  // conversion time in pixel clocks (exposure time is set in
  // units of conversion times)
  uint16_t frame_length_pck_;

  // track device temperature
  time_t temperature_sample_ms_ = 100;
  std::atomic<double> temperature_last_;
  void ReadTemperature();
  std::thread temperature_thread_;
  bool enableSaveRcam_ = false;

  // track x subwindow
  int x0_ = 0;
  int x1_ = 0;
  int y0_ = 0;
  int y1_ = 1;
  int size_ = 0;
};
