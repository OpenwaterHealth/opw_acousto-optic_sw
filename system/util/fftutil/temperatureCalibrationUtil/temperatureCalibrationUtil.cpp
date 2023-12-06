// temperatureCalibrationUtil.cpp

#include <iostream>

#include "system/component/inc/rcam.h"
#include "system/component/inc/time.h"

int main()
{
  // Initialize camera object and start streaming
  Rcam* camera = new Rcam();
  camera->Open();
  camera->Start();

  // Colorbar settings [can also read from file]
  camera->WriteCFG(0x0370, 0x00); // [0] mono mode enable
  camera->WriteCFG(0x0601, 0x85); // colorbar

  // TS_CFG, TS_OFFSET_SEL
  camera->WriteCFG(0x2230, 0x03);
  camera->WriteCFG(0x2233, 0x01);
  Component::SleepMs(100);

  // TS_VALUE
  uint32_t f1 = camera->ReadCFG(0x2237);
  Component::SleepMs(1000);

  camera->WriteCFG(0x2230, 0x03);
  camera->WriteCFG(0x2233, 0x01);
  Component::SleepMs(100);
  uint32_t f2 = camera->ReadCFG(0x2237);
  Component::SleepMs(1000);

  camera->WriteCFG(0x2230, 0x03);
  camera->WriteCFG(0x2233, 0x01);
  Component::SleepMs(1);
  uint32_t f3 = camera->ReadCFG(0x2237);
  Component::SleepMs(1000);

  std::cout << "f1: " << f1 << std::endl;
  std::cout << "f2: " << f2 << std::endl;
  std::cout << "f3: " << f3 << std::endl;

  // Calibration
  uint32_t result;
  uint32_t temp = (f1 + f2 + f3) / 3;
  if (temp > 0x3D) {
    result = (temp - 0x3D) | 0x80;
    std::cout << "high" << std::endl;
  } else {
    result = 0x3D - temp;
    std::cout << "low" << std::endl;
  }
  std::cout << "temp: " << temp << std::endl;
  std::cout << "result: " << result << std::endl;

  // Write to OTP
  camera->WriteCFG(0x23B8, 0x04); // for charge pump
  camera->WriteCFG(0x2690, 0x00); // [0]:BPC_OTP_en
  camera->WriteCFG(0x23A3, 0xB0); // for charge pump
  camera->WriteCFG(0x23C0, 0x04); // for charge pump
  camera->WriteCFG(0x23A2, 0x1A); // for charge pump
  camera->WriteCFG(0x2600, 0x0A); // OTP page [page 10]
  camera->WriteCFG(0x2601, 0x0A); // temperature offset [offset 10]
  camera->WriteCFG(0x2602, 0x01); // length [length 1]
  camera->WriteCFG(0x26C0, result); // temperature calibration data
  Component::SleepMs(20);
  camera->WriteCFG(0x2603, 0x01); // write cmd
  Component::SleepMs(5);
  camera->WriteCFG(0x2603, 0x00); // idle cmd
  Component::SleepMs(200);
  camera->WriteCFG(0x23C0, 0x00); // for charge pump
  camera->WriteCFG(0x23A2, 0x00); // for charge pump

  camera->Stop();
  delete camera;
}
