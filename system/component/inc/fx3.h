#pragma once

#include <cstdint>
#include <mutex>
#include <vector>

// windows needed for Cypress API
// CyAPI has issues if windows.h is not included first
#if defined(_MSC_VER)
#include <windows.h>
#include "system/third_party/cypress-fx3/inc/CyAPI.h"
#elif defined(__APPLE__)
#include "system/third_party/cypress-fx3/cyusb_mac_1.0/include/cyusb.h"
#else  // linux
#include "system/third_party/cypress-fx3/cyusb_linux_1.0.5/include/cyusb.h"
#endif


// Interface to generic FX3 devices.
class FX3 {
 public:
  FX3() {}
  ~FX3() { Close(); }

  // Error codes
  // USB disconnect error
  static const int USB_DISCONNECT = -1000;

  // Number of FX3 with a specific PID attached to the computer.
  // @note PID for Openwater devices have the lower bit cleared
  //   when unprogrammed, and set when programmed.
  //   This means PID 0x1A0 and 0x1A1 are the same physical device,
  //   however, PID 0x1A1 is programmed and ready for instructions.
  //   This was chosen to allow programmed / unprogrammed devices
  //   to be visible from the device manager.
  static int NumDevices(uint16_t pid);

  // Connect to a device with a particular PID and serial number
  // @param pid PID of the device
  // @param n nth device with the specified PID to which to connect
  //   Leave blank to connect to any device found.
  // @returns 0 on success
  virtual int Open(uint16_t pid, int n = 0);

  // Reset the target device to bootloader
  int Reset();

  // Serial number of this device
  // @returns serial number if found, -1 otherwise
  int SerialNumber();

  // Close connection to the FX3 device.
  void Close();

  // Flash an FX3 with a firmware image
  // @param len length of the firmware image
  // @param data data to flash
  // @returns 0 on success
  virtual int Flash(int len, uint8_t* data);

  // Threadsafe write control packet to endpoint 0
  // @param req request code
  // @param wValue request value
  // @param wIndex request index
  // @param len length of request, if blank 0
  // @param buf bytes with length len to transmit
  // @returns bytes written
  int CmdWrite(uint8_t req, uint16_t wValue, uint16_t wIndex, uint16_t len = 0,
               uint8_t* buf = NULL);

  // Threadsafe read control packet to endpoint 0
  // @param req request code
  // @param wValue request value
  // @param wIndex request index
  // @param len length of request
  // @param buf bytes with length len to fill with recieved data
  // @returns bytes read
  virtual int CmdRead(uint8_t req, uint16_t wValue, uint16_t wIndex, uint16_t len,
                      uint8_t* buf);

  // Receive data from a bulk in endpoint in synchronous mode
  // @param len maximum number of bytes to receive
  // @param data buffer to fill with data
  // @returns number of bytes read, <0 if an error has occurred
  int DataIn(int len, uint8_t* data);

  // Transmit data to a bulk in endpoint in synchronous mode
  // @param len number of bytes to transmit
  // @param data data to transmit
  // @returns number of bytes transmitted, <0 if an error has occurred.
  int DataOut(int len, uint8_t* data);

  // Bulk in asynchronous control
  // The bulk in endpoint operates in blocking mode by default.
  // This is sufficient for low data rate endpoints, or synchronous endpoins.
  // For high data rate endpoints, it is recommended to use the
  //   BulkInBuffers() and BulkInStart() functions to setup
  //   the buffers and start streaming data in respectively.
  //   BulkInData() will then draw data from those buffers as they
  //   fill.  BulkInStop() is automatically called by the destructor, but can
  //   be explicitly called as well.

  // Set buffers for bulk in endpoint
  // @param buflen buffer length for a single bulk transaction
  //   if 0, will revert to synchronous mode
  // @param n_buffers number of buffers per endpoint.
  // @param timeout_ms milliseconds to wait for each buffer to be filled
  // @returns 0 on success
  int BulkInBuffers(int buflen, int n_buffers, int timeout_ms);

  // Start bulk input transfer
  // @returns 0 on success
  int BulkInStart();

  // Get data from the bulk in endpoint
  // @param[out] len length of data produced
  // @param[out] data pointer to data
  // @returns 0 on success
  // @note may block if waiting on data
  int BulkInData(int& len, uint8_t*& data);

  // Stop bulk input transfer
  // @returns 0 on success
  void BulkInStop();

  // Reattach the device and refresh endpoint pointers
  void Reattach();

 private:
  static const int RQ_SERIAL = 0xE1;
  static const int RQ_RESET = 0xE0;

#ifdef _MSC_VER
  // device endpoints
  CCyUSBDevice usb_device_;
  CCyControlEndPoint* ctrl_ = NULL;
  CCyBulkEndPoint* bulk_in_ = NULL;
  CCyBulkEndPoint* bulk_out_ = NULL;
  int address_ = -1;

  // The Cypress driver maps Open(device_number) to different devices after a USB
  //   disconnect.  To work around this, we use the underlying windows ID, and map
  //   that to device number.
  // Because of this, we need to create a list of device PIDs and addresses the first
  //   time this driver is called, to ensure Open(0) always opens the 0th device
  //   connected to the system (ie, the cypress driver hasn't reordered devices between
  //   Open(0); ... Open(1);
  static void Init();

  // map of PID to address.  may have multiple idendical entries for PID, which
  //   correspond to multiple devices of the same type connected to the system.
  static std::vector<std::pair<uint16_t, int>> addresses_;

  // Find the device number that has address.  Device numbers change based on USB
  //   disconnects, addresses do not.
  static int DeviceNumber(int address);

  // bulk in buffers
  bool async_ = false;
  int timeout_ms_ = 0;
  std::vector<std::vector<uint8_t>> in_buffer_;
  std::vector<OVERLAPPED> inovlap_;
  std::vector<uint8_t*> contexts_;
  int in_n_ = -1;

  std::mutex ctrl_mutex_;
#endif
};
