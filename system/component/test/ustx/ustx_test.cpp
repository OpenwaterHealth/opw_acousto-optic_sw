#include "system/component/inc/ustx.h"
#include "system/component/inc/intelhex.h"
#include "system/component/inc/serial.h"

#include <conio.h>
#include <cstring>
#include <fstream>

#include "system/third_party/json-develop/single_include/nlohmann/json.hpp"

using json = nlohmann::json;

int main() {
  json fin = json::parse(std::ifstream("foci.txt"));
  Serial ustx_com;
  USTx ustx(&ustx_com);
  ustx.LoadJson(fin);
  ustx.ComputeFoci("foci.hex");
  printf("Focus Delay: %.2e\n", ustx.GetDelay());

  ustx.Open(6);
  ustx.DownloadHex("foci.hex");

  ustx.SetFocus(1);
  printf("Focus now: %d\n", ustx.GetFocus());
  std::pair<double, double> pos = ustx.GetFocusCoordinates(1);
  printf("Focus at %.0fmm, %.0fmm\n", pos.first * 1000, pos.second * 1000);
  printf("Run Foci\n");
  system("pause");

  if (ustx.ThermalShutdown()) {
    printf("Device in thermal shutdown\n");
    ustx.Reset();
  } else {
    printf("Device okay\n");
  }

  ustx.Close();

  system("pause");
}
