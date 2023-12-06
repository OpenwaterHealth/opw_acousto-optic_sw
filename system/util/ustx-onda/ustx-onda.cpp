#undef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include "system/component/inc/intelhex.h"
#include "system/component/inc/serial.h"
#include "system/component/inc/ustx.h"

#include <conio.h>
#include <cstring>
#include <fstream>

#include "system/third_party/json-develop/single_include/nlohmann/json.hpp"

using json = nlohmann::json;

int main(int argc, char** argv) {
  if (argc < 3) {
    printf("Usage: ustx-onda <json file> <COM port number>\n");
    return -1;
  }

  json fin = json::parse(std::ifstream(argv[1]));
  Serial ustx_com;
  USTx ustx(&ustx_com);
  ustx.LoadJson(fin);
  if (!ustx.ComputeFoci("foci.hex")) {
    printf("Invalid Foci List\n");
    return -1;
  }

  int com_port = -1;
  sscanf(argv[2], "%d", &com_port);
  if (ustx.Open(com_port) != 0) {
    printf("Could not open USTx connection on port %d\n", com_port);
    return -1;
  }
  ustx.Reset();
  ustx.DownloadHex("foci.hex");
  if (ustx.ThermalShutdown()) {
    printf("USTx in Thermal Shutdown\n");
    return -1;
  }
  printf("Focus Delay: %.1lfus\n", ustx.GetDelay() * 1000000.0);

  while (true) {
    printf("Set Focus: ");
    int focus;
    if (scanf("%d", &focus) == 1) {
      if (focus >= ustx.GetFocusCount() || focus < 0) {
        printf("Invalid Focus, max focus: %d\n", ustx.GetFocusCount() - 1);
        continue;
      }
      ustx.SetFocus(focus);
      double x = ustx.GetFocusCoordinates(focus).first;
      double z = ustx.GetFocusCoordinates(focus).second;
      printf("Focus now at %.1lfmm, %.1lfmm\n", x * 1000.0, z * 1000.0);
      printf("Delay: %.1lfus\n",
             (ustx.GetDelay() - sqrt(x * x + z * z) / ustx.GetSpeedOfSound()) *
                 1e6);
    } else {
      break;
    }
  }

  ustx.Close();
}
