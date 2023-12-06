// OpenwaterScanningSystem_Pulsed.cpp
// Caitlin Regan, John Schlag [Openwater]

#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <wchar.h>

#include <glog/logging.h>

#include "system/component/inc/ustx.h"
#include "system/component/inc/intelhex.h"
#include "system/component/inc/serial.h"
#include "system/third_party/json-develop/single_include/nlohmann/json.hpp"

#include "AmpStage.h"
#include "AsyncScanner.h"
#include "CameraManager.h"
#include "ConexStage.h"
#include "delays.h"
#include "OctopusManager.h"
#include "QuantumComposers.h"
#include "robot.h"
#include "RobotScanner.h"
#include "RotisserieScanner.h"
#include "Verdi.h"

using json = nlohmann::json;

int main(int argc, char *argv[]) {
  FLAGS_logtostderr = 1;
  google::InitGoogleLogging(argv[0]);

  if (argc < 2) {
    LOG(ERROR) << "Usage: " << argv[0] << " <root_folder>";
    return -1;
  }

  // Process argument
  std::string rootFolder(argv[1]); // Rootname of experiment [used to load json scan data info]
  // Import scan parameters from Python GUI [exe called from system/app/scanUI dir]
  std::string jsonFilename = "../../../data/localScanDataFiles/" + rootFolder + "/scan_metadata.json";
  std::ifstream scanMetadata(jsonFilename);
  if (scanMetadata.fail()) {
    LOG(ERROR) << "Can't open " << jsonFilename;
    return -1;
  }
  json systemParameters = json::parse(scanMetadata);
  scanMetadata.close();
  LOG(INFO) << "Opened " << jsonFilename;

  int pulsedSystem =  systemParameters["laserParameters"]["pulsed"].get<int>();
  int octopusSystem = systemParameters["hardwareParameters"]["octopus"].get<int>();
  std::string usAmplifier = systemParameters["ultrasoundParameters"]["ultrasoundAmp"].get<std::string>();
  std::string laserModel = systemParameters["laserParameters"]["laser"].get<std::string>();
  int isRobot = systemParameters["hardwareParameters"]["robot"].get<int>();
  int isRotisserie = systemParameters["hardwareParameters"]["rotisserie"].get<int>();

  ////////// INITIALIZE DEVICES //////////

  Laser* laser = NULL; // System dependent [Moglabs vs Amplitude vs Verdi]

  // Devices that control timing/triggering of system
  OctopusManager* octopusManager = NULL; // Used to control timers to all devices and signal to AOMs and FF ultrasound
  BerkeleyNucleonics* delayGenerator = NULL;  // Used in fake pulsed system


  if (octopusSystem) {
    octopusManager = new OctopusManager();
    if (laserModel.compare("Fake") == 0 && pulsedSystem) {
      delayGenerator = new BerkeleyNucleonics();
    }
  } else {
    LOG(ERROR) << "Non-octopus systems no longer supported";
    return -1;
  }

  USTx* ustx = NULL;
  if (usAmplifier.compare("USTx") == 0) {
    Serial* serial = new Serial();
    ustx = new USTx(serial);
  }

  if (pulsedSystem) {
    // Initialize Laser
    if (laserModel.compare("Amplitude-Continuum-V2") == 0) {
      laser = new QuantumComposers();
    }
  } else if (laserModel == "Verdi") {
    laser = new Verdi();
  }

  CameraManager* cameras = new CameraManager();

  // This takes ownership of the components. TODO(jfs): Use smart ptrs.
  Scanner* scanner = NULL;
  if (isRobot) {
    Robot* robot = new Robot();
    scanner = new RobotScanner(
        systemParameters, laser, delayGenerator, robot, ustx, cameras,
        octopusManager);
  } else if (isRotisserie) {
    ConexStage* xStage = new ConexStage();
    ConexStage* yStage = new ConexStage();
    ConexStage* zStage = new ConexStage();
    AmpStage* rStage = new AmpStage();
    scanner = new RotisserieScanner(
        systemParameters, laser, delayGenerator, xStage, yStage, zStage, 
        rStage, ustx, cameras, octopusManager);
  } else {
    ConexStage* xStage = NULL;
    ConexStage* yStage = NULL;
    ConexStage* zStage = NULL;
    if (laserModel.compare("Fake") != 0) {
      xStage = new ConexStage();
      yStage = new ConexStage();
      zStage = new ConexStage();
    }
    scanner = new AsyncScanner(
        systemParameters, laser, delayGenerator, xStage, yStage, zStage,
        ustx, cameras, octopusManager);
  }

  if (!scanner->init()) {
    return -1;
  }

  // Run the scan.
  bool scanResult = scanner->scan();

  delete scanner;

  return scanResult ? 0 : -1;
}
