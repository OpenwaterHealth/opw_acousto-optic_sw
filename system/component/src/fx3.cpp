#include "system/component/inc/fx3.h"

#include <cassert>
#include <fstream>
#include <thread>

#ifdef _MSC_VER

// timeout to reconnect to a disconnected device in seconds
static const double REATTACH_TIMEOUT = 15.0;
static const double FIRMWARE_BOOT_TIMEOUT = 15.0;

#pragma comment(lib, "CyAPI.lib")
#pragma comment(lib, "legacy_stdio_definitions.lib")
#pragma comment(lib, "Setupapi.lib")

std::vector<std::pair<uint16_t, int>> FX3::addresses_;

static void PrintGUID(const GUID& guid) {
  printf("GUID: %08X_%04X_%04X\n", guid.Data1, guid.Data2, guid.Data3);
}

void FX3::Init() {
  if (addresses_.size() > 0) return;
  CCyUSBDevice usb(NULL);
  for (int i = 0; i < usb.DeviceCount(); ++i) {
    usb.Open(i);
    // Openwater PIDs use the low bit to determine programmed vs unprogrammed.
    // PID 0x4F10 is an upgrammed gumstick, PID 0x4F11 is a programmed gumstick.
    // They both count as the same device, so we store the PID omitting the
    // lower bit.
    addresses_.push_back(
        std::pair<uint16_t, int>(usb.ProductID & 0xFFFE, usb.USBAddress));
    usb.Close();
  }
}

int FX3::DeviceNumber(int address) {
  CCyUSBDevice usb(NULL);
  int n = 0;
  while (true) {
    if (!usb.Open(n)) continue;
    if (usb.USBAddress == address) break;
    ++n;
    if (n >= addresses_.size()) n = 0;
  }
  usb.Close();
  return n;
}

int FX3::NumDevices(uint16_t pid) {
  Init();

  int match = 0;
  for (const std::pair<uint16_t, int>& addr : addresses_) {
    if (addr.first == (pid & 0xFFFE)) ++match;
  }
  return match;
}

int FX3::Open(uint16_t pid, int n) {
  Init();

  for (const std::pair<uint16_t, int>& addr : addresses_) {
    // Openwater PIDs use the low bit to determine programmed vs unprogrammed.
    // PID 0x4F10 is an upgrammed gumstick, PID 0x4F11 is a programmed gumstick.
    // They both count as the same device, so we store the PID omitting the
    // lower bit.
    if (addr.first == (pid & 0xFFFE)) {
      if (n == 0) {
        address_ = addr.second;
        break;
      }
      --n;
    }
  }
  if (address_ < 0) return -1;

  Reattach();

  return 0;
}

int FX3::SerialNumber() {
  assert(ctrl_);
  int32_t serial_number;
  int read = CmdRead(FX3::RQ_SERIAL, 0, 8, 4, (uint8_t*)&serial_number);
  if (read != 4) return -1;
  return serial_number;
}

int FX3::Reset() {
  assert(ctrl_);
  BulkInStop();
  CmdWrite(FX3::RQ_RESET, 0, 0);
  // TODO(carsten) we can liekly get rid of this magic sleep
  Sleep(5000);

  // make sure the bootlaoder is running after reset
  CCyFX3Device bootloader;
  while (!bootloader.Open(DeviceNumber(address_))) {
    Sleep(100);
  };
  bool boot = bootloader.IsBootLoaderRunning();
  bootloader.Close();

  Reattach();
  return boot != true;
}

int FX3::CmdWrite(uint8_t req, uint16_t wValue, uint16_t wIndex, uint16_t len,
                  uint8_t* buf) {
  assert(ctrl_);
  std::lock_guard<std::mutex> lock(ctrl_mutex_);
  ctrl_->Target = TGT_DEVICE;
  ctrl_->ReqType = REQ_VENDOR;
  ctrl_->Direction = DIR_TO_DEVICE;
  ctrl_->ReqCode = req;
  ctrl_->Value = wValue;
  ctrl_->Index = wIndex;
  long buflen = len;
  ctrl_->XferData(buf, buflen);

  return buflen;
}

int FX3::CmdRead(uint8_t req, uint16_t wValue, uint16_t wIndex, uint16_t len,
                 uint8_t* buf) {
  assert(ctrl_);
  std::lock_guard<std::mutex> lock(ctrl_mutex_);
  ctrl_->Target = TGT_DEVICE;
  ctrl_->ReqType = REQ_VENDOR;
  ctrl_->Direction = DIR_FROM_DEVICE;
  ctrl_->ReqCode = req;
  ctrl_->Value = wValue;
  ctrl_->Index = wIndex;
  long buflen = len;
  ctrl_->XferData(buf, buflen);

  return buflen;
}

int FX3::Flash(int len, uint8_t* data) {
  assert(ctrl_);

  // check if the device is in bootloader mode
  // bulk_in_ is a quick check
  if (bulk_in_) return 0;
  CCyFX3Device boot;
  if (!boot.Open(DeviceNumber(address_))) return -1;
  if (!boot.IsBootLoaderRunning()) {
    boot.Close();
    return 0;
  }

  // Create a temporary file for DownloadFw, and send it to the device
  static char fname[] = "__tmp.img";
  std::ofstream fp;
  fp.open(fname, std::ios::out | std::ios::binary);
  fp.write((char*)data, len);
  fp.close();
  FX3_FWDWNLOAD_ERROR_CODE err = boot.DownloadFw(fname, RAM);
  remove(fname);
  if (err != FX3_FWDWNLOAD_ERROR_CODE::SUCCESS) {
    printf("FW Download Error: %d\n", err);
    return -1;
  }
  boot.Close();

  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  Reattach();

  return 0;
}

void FX3::Close() {
  if (!ctrl_) return;

  BulkInStop();
  usb_device_.Close();
  ctrl_ = NULL;
  bulk_in_ = NULL;
  bulk_out_ = NULL;
  address_ = -1;
}

int FX3::DataIn(int len, uint8_t* data) {
  assert(async_ == false);
  long l = len;
  if (bulk_in_->XferData(data, l)) {
    return l;
  } else {
    return -1;
  }
}

int FX3::DataOut(int len, uint8_t* data) {
  long l = len;
  if (bulk_out_->XferData(data, l)) {
    return l;
  } else {
    return -1;
  }
}

int FX3::BulkInBuffers(int buflen, int n_buffers, int timeout_ms) {
  if (buflen == 0) async_ = false;

  in_buffer_.clear();
  in_buffer_.resize(n_buffers, std::vector<uint8_t>(buflen));
  inovlap_.clear();
  inovlap_.resize(n_buffers);
  contexts_.clear();
  contexts_.resize(n_buffers);
  timeout_ms_ = timeout_ms;

  return 0;
}

int FX3::BulkInStart() {
  assert(bulk_in_);
  assert(in_buffer_.size() > 0);

  // create windows handles
  for (OVERLAPPED& overlap : inovlap_) {
    overlap.hEvent = CreateEvent(NULL, false, false, NULL);
  }

  // start USB transactions
  for (int i = 0; i < in_buffer_.size(); ++i) {
    contexts_[i] = bulk_in_->BeginDataXfer(in_buffer_[i].data(),
                                           (LONG)in_buffer_[i].size(), &inovlap_[i]);
    if (bulk_in_->NtStatus || bulk_in_->UsbdStatus) {
      printf("Error starting USB Transaction\n");
      return -1;
    }
  }
  in_n_ = -1;
  async_ = true;

  return 0;
}

int FX3::BulkInData(int& len, uint8_t*& data) {
  assert(bulk_in_);
  assert(async_);

  len = 0;
  data = NULL;

  // Start a new transaction over the previously finished buffer.
  // The first call to BulkInData() after BulkInStart() will have in_n_ = -1,
  // indicating there is no "last completed" buffer to start
  if (in_n_ >= 0) {
    contexts_[in_n_] = bulk_in_->BeginDataXfer(
        in_buffer_[in_n_].data(), (LONG)in_buffer_[in_n_].size(), &inovlap_[in_n_]);
  }
  ++in_n_;
  if (in_n_ >= in_buffer_.size()) in_n_ = 0;

  // Finish the async IO transaction
  long len_rx = (long)in_buffer_[in_n_].size();
  int retval = 0;
  if (!bulk_in_->WaitForXfer(&inovlap_[in_n_], timeout_ms_)) {
    bulk_in_->Abort();
    retval = -(int)bulk_in_->LastError;

    if (bulk_in_->LastError == ERROR_IO_PENDING) {
      WaitForSingleObject(inovlap_[in_n_].hEvent, 2000);
    }
  }
  bulk_in_->FinishDataXfer(in_buffer_[in_n_].data(), len_rx, &inovlap_[in_n_],
                           contexts_[in_n_]);

  // this seems to happen only when a USB disconnect has occurred
  if (retval == 0 && len_rx == 0 && bulk_in_->UsbdStatus == 0) {
    return USB_DISCONNECT;
  }

  // return a pointer to the internal buffer
  len = len_rx;
  data = in_buffer_[in_n_].data();
  return retval;
}

void FX3::BulkInStop() {
  if (!async_) return;

  // Abort the bulk endpoint, and finish all USB transactions
  // The buffer specified by in_n_ has already completed, so no need to finish
  // it.
  bulk_in_->Abort();
  for (int i = 0; i < in_buffer_.size(); ++i) {
    if (i == in_n_) continue;
    bulk_in_->WaitForXfer(&inovlap_[i], 5);
    long buflen = (long)in_buffer_[i].size();
    bulk_in_->FinishDataXfer(in_buffer_[i].data(), buflen, &inovlap_[i],
                             contexts_[i]);
  }

  // Close windows handles
  for (OVERLAPPED& overlap : inovlap_) {
    CloseHandle(overlap.hEvent);
  }
  async_ = false;
}

void FX3::Reattach() {
  int64_t t0 = GetTickCount64();
  while (usb_device_.DeviceCount() < addresses_.size()) {
    if (GetTickCount64() - t0 > REATTACH_TIMEOUT * 1000) {
      printf("Fatal USB Disconnect timeout\n");
      assert(0);
    }
  }

   do {
      usb_device_.Open(DeviceNumber(address_));

      ctrl_ = usb_device_.ControlEndPt;
      bulk_in_ = usb_device_.BulkInEndPt;
      bulk_out_ = usb_device_.BulkOutEndPt;
    } while (usb_device_.USBAddress != address_);

  assert(ctrl_);

  // Minimum control endpoint timeout
  ctrl_->TimeOut = 1000;
}

#else

// All of FX3 impl is commented out off-Win.
int FX3::NumDevices(uint16_t) { return 0; }
int FX3::Open(uint16_t pid, int n) { return 0; }
void FX3::Close() {}
int FX3::Flash(int len, uint8_t* data) { return -1; }
int FX3::CmdWrite(uint8_t req, uint16_t wValue, uint16_t wIndex, uint16_t len, uint8_t* buf) { return 0; }
int FX3::CmdRead(uint8_t req, uint16_t wValue, uint16_t wIndex, uint16_t len, uint8_t* buf) { return 0; }
int FX3::SerialNumber() { return -1; }
int FX3::Reset() { return 0; }
void FX3::Reattach() {}
int FX3::DataIn(int len, uint8_t* data) { return 0; }
int FX3::DataOut(int len, uint8_t* data) { return 0; }
int FX3::BulkInData(int&, unsigned char*&) { return 0; }
void FX3::BulkInStop() {}
int FX3::BulkInStart() { return -1; }
int FX3::BulkInBuffers(int, int, int) { return -1; }

#endif
