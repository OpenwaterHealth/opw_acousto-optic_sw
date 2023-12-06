#include <iostream>
#include <fstream>

#include "ustxManager.h"
#include "OctopusManager_USscanner.h"
#include "robotScanner_USscanner.h"


#include "system/third_party/json-develop/single_include/nlohmann/json.hpp"

using json = nlohmann::json;

  // read in scan metadata to get all the parameters (from scanUI or json file)
  // initialize all the objects (ustx, robot, octopus)
  // call scanner to loop through scan 
  // clean everything up

  int main(int argc, char* argv[]) {

    if (argc < 2) {
      std::cout << "Usage: " << argv[0] << " <root_folder>" << std::endl;
      return -1;
    }

    // Process argument
    std::string rootFolder(argv[1]); // Rootname of experiment [used to load json scan data info]
    // Import scan parameters from Python GUI [exe called from system/app/scanUI dir]
    std::string jsonFilename = "../../../data/localScanDataFiles/" + rootFolder + "/scan_metadata.json";
    // for debugging
    // std::string jsonFilename = "C:/data_scans/Ultasound_Curie/syncedScanDataFiles/2020_12_01_15_40_testScan_CBCR_/scan_metadata.json";
    
    std::ifstream scanMetadata(jsonFilename);
    if (scanMetadata.fail()) {
      std::cout << "Can't open " << jsonFilename << std::endl;
      return -1;
    }
    json systemParameters = json::parse(scanMetadata);
    scanMetadata.close();
    std::cout << "Opened " << jsonFilename << std::endl;

    // create objects for everything
    OctopusManager_USscanner* octopusManager = new OctopusManager_USscanner(); 

    RobotScanner_USscanner* robotScanner = new RobotScanner_USscanner();

    std::string usAmplifier = systemParameters["ultrasoundParameters"]["ultrasoundAmp"].get<std::string>();
    ustxManager_USscanner* ustxManager =  NULL;
    if (usAmplifier.compare("USTx") == 0) {
      ustxManager = new ustxManager_USscanner();
    }

    // initialize everything
    octopusManager->init(systemParameters);
    robotScanner->init(systemParameters);
    if(ustxManager) ustxManager->init(systemParameters);

    // run the scan
    robotScanner->scan(systemParameters, ustxManager, octopusManager);

    delete robotScanner;
    delete octopusManager;
    if (ustxManager) delete ustxManager;

    std::cout << "scan done" << std::endl;

    return 0;
}
