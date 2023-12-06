#include <sstream>
#include <vector>

#include "googletest/googletest/include/gtest/gtest.h"
#include "googletest/googlemock/include/gmock/gmock.h"
#include "system/scanner/OpenwaterScanningSystem_Pulsed/ConexStage.h"
#include "system/scanner/OpenwaterScanningSystem_Pulsed/OctopusManager.h"
#include "system/scanner/OpenwaterScanningSystem_Pulsed/robot.h"
#include "system/scanner/OpenwaterScanningSystem_Pulsed/RobotScanner.h"
#include "system/scanner/OpenwaterScanningSystem_Pulsed/AsyncScanner.h"
#include "system/third_party/json-develop/single_include/nlohmann/json.hpp"

using json = nlohmann::json;
using testing::_;
using testing::Return;

// Test data from Amplitude-Continuum/syncedScanDataFiles/2019_09_17_14_29_Zqtips_2cmThickPhantom
const char* const TestJSON = R"foo({

  "scanParameters": {
    "xMaxLocation_mm": 49.1,
    "yMaxLocation_mm": 49.1,
    "zClosest_mm": 49.1,
    "xLength_mm": 48.0,
    "yLength_mm": 0.0,
    "zLength_mm": 0.0,
    "alphaAngle_deg": 0.0,
    "betaAngle_deg": 0.0,
    "gammaAngle_deg": 0.0,
    "xScanStepSize_mm": 1.0,
    "yScanStepSize_mm": 1.0,
    "zScanStepSize_mm": 1.0,
    "alphaScanStepSize_deg": 1.0,
    "betaScanStepSize_deg": 1.0,
    "gammaScanStepSize_deg": 1.0,
    "xROICenter_mm": 24.5,
    "yROICenter_mm": 35.5,
    "zROIStart_mm": 35.5,
    "alphaCenter_deg": 0.0,
    "betaCenter_deg": 0.0,
    "gammaCenter_deg": 0.0,
    "xInit_mm": 0.5,
    "yInit_mm": 35.5,
    "alphaInit_deg": 0.0,
    "betaInit_deg": 0.0,
    "gammaInit_deg": 0.0,
    "azimuthMaxLength_mm": 38.4,
    "azimuthLength_mm": 30.0,
    "axialLength_mm": 42.0,
    "azimuthScanStepSize_mm": 1.0,
    "axialScanStepSize_mm": 1.0,
    "axialROIStart_mm": 5.0,
    "azimuthROICenter_mm": 0.0,
    "azimuthInit_mm": -15.0,
    "additionalPauseTime_ms": 0.0
  },
  "ultrasoundParameters": {
    "ultrasoundAmp": "USTx",
    "ultrasoundProbe": "Sonic Concepts 5MHz",
    "ultrasoundVoltage_V": 45.0,
    "ultrasoundFreq_MHz": 5.0,
    "fNumber": 1.0,
    "speedOfSoundWater_mmus": 1.5,
    "ultrasoundFocalLength_mm": 35,
    "nCyclesUltrasound": 128,
    "usAdditionalDelay_us": 0.0
  },
  "AOMParameters": {
    "AOM1Freq_MHz": 100,
    "AOM2Freq_MHz": 95.0,
    "AOM1Volt_V": 7,
    "AOM2Volt_V": 7
  },
  "laserParameters": {
    "pseudoPulsed": 0,
    "laser": "Amplitude-Continuum-V2",
    "wavelength_nm": 830.0,
    "laserPulseArrivalDelay_us": 600.69,
    "laserClockPeriod_ms": 10.0,
    "pulsed": 1
  },
  "cameraParameters": {
    "camera": "HiMax HM5530 Gumstick Board v2.0",
    "numCameras": 1,
    "cameraIDNumbers": { "213": "" },
    "cameraROI_xCenter": { "213": 677 },
    "cameraROI_yCenter": { "213": 162 },
    "cameraROI_radius": { "213": 80 },
    "pixelSize_um": 2.0,
    "cameraBitDepth": 10,
    "rowTime_us": 15.8,
    "resolutionY": 512,
    "resolutionX": 2048,
    "exposureTime_ms": 9.9,
    "frameLength_ms": 10.0,
    "laserClkToEndOfFirstRowExposure_ms": 0.81
  },
  "photodiodeParameters": {},
  "delayParameters": {
    "TTLPulseWidth_s": 0.0001,
    "chADelay_s": "0.0008100000000000001",
    "chAWidth_s": "0.0001",
    "chBDelay_s": "0.0005878900000000002",
    "chBWidth_s": "0.0001",
    "chCDelay_s": "0.0005878900000000002",
    "chCWidth_s": "0.001",
    "chDDelay_s": "0.0007100000000000001",
    "chDWidth_s": "0.001",
    "chEDelay_s": "0.0",
    "chEWidth_s": "0.0001",
    "chFDelay_s": "0.0",
    "chFWidth_s": "0.0001",
    "QCchGDelay_s": "0.0008100000000000001",
    "QCchGWidth_s": "0.0001",
    "QCchHDelay_s": "0.0",
    "QCchHWidth_s": "0.0001"
  },
  "fileParameters": {
    "filename": "CR_photocopier_Homogeneous",
    "swVersion": "b'commit 53336eeb345e30ac92f292c3718242d14db37480 (HEAD -> refs/heads/dussikPhotocopier, refs/remotes/origin/master, refs/remotes/origin/HEAD, refs/heads/master)\\nMerge: 2638f442 d0715ea4\\nAuthor: Openwater-jfs <50122692+Openwater-jfs@users.noreply.github.com>\\nDate:   Tue Jun 2 08:03:16 2020 -0700\\n\\n    Merge pull request #298 from OpenwaterInternet/glog\\n    \\n    add glog stubs for linux\\n'",
    "metaDataVersion": 2.4,
    "scannerVersion": 3.0,
    "rootFolder": "2020_06_03_13_53_CR_photocopier_Homogeneous",
    "localScanDataDir": "../../../data/localScanDataFiles/2020_06_03_13_53_CR_photocopier_Homogeneous",
    "syncedRawImageDir": "",
    "syncedScanDataDir": "C:/data_scans/Amplitude-Continuum-V2/syncedScanDataFiles/2020_06_03_13_53_CR_photocopier_Homogeneous"
  },
  "sampleParameters": {
    "sampleMaterial": "gel wax",
    "inclusion": false,
    "absorberDescription": "",
    "sampleNotes": "40mm thick gel wax slab",
    "experimentNotes": "source-detector ~24mm separation centered with ultrasound txdr face. txdr 'vertical' relative to source detector. txdr face ~1mm from face of rexolite. gel wax coupled to rexolite with ultrasound gel. rexolite ~6.7mm thick"
  },
  "hardwareParameters": {
    "octopus": 1004,
    "robot": 0,
    "robotTRF": [ 70.0, 0.0, 125.0, 90.0, 0.0, 0.0 ],
    "robotInitJoints": [0.0, 39.579, 12.9093, 0.0, 37.5107, 0.0],
    "rotisserie": 0,
    "bncCOM": 0,
    "laserCOM": 5,
    "ultrasoundRigolSN": "",
    "AOMRigolSN": "",
    "xStageCOM": 6,
    "yStageCOM": 7,
    "zStageCOM": 9,
    "usCOM": 10,
    "ustx": 2102
  }
}
)foo";

// Until CameraManager.h is pure virtual
CameraManager::~CameraManager() {}
bool CameraManager::init(const json& systemParameters, Trigger* trigger) { return true; }
int CameraManager::captureAndWriteImagesAsync(int numFociPerSlice, int sliceIdx, int numFociPerRow, int axialRowIdx, double frameGatePeriod_s) { return 1; }
void CameraManager::writeVoxelData(const voxelData& newVoxelData) {}
void CameraManager::setImageInfoStream(std::ofstream* stream, bool local) {}
void CameraManager::setRepeatedVoxelLogStream(std::ofstream* stream) {}
void CameraManager::startAllCameras() {}
void CameraManager::resetAllCamerasMidscan(int frameCoutn) {}
bool CameraManager::endExecNodes() { return true; }

// Mock for ConexStage
class MockConexStage: public ConexStage {
 public:
  MOCK_METHOD1(init, bool(int comPort));
  MOCK_METHOD0(resetController, int());
  MOCK_METHOD0(moveHome, int());
};

class MockCameraManager: public CameraManager {
 public:
  MOCK_METHOD2(init, bool(const json& systemParameters, Trigger* trigger));
  MOCK_METHOD5(captureAndWriteImagesAsync, int(int numFociPerSlice, int sliceIdx, int numFociPerRow, int axialRowIdx, double frameGatePeriod_s));
};

class MockOctopusManager : public OctopusManager {
public:
  MOCK_METHOD3(init, bool(const json& systemParameters, int numFociPerSlice, Trigger* trigger));
  MOCK_METHOD2(SetVoxelTriggerPins, bool(Octopus::Pin triggerPin, Octopus::Pin gatePin));
  MOCK_METHOD2(InitializeOctopus, bool(const json& systemParameters, int numFociPerSlice));
};

class MockLaser: public Laser {
 public:
  MOCK_METHOD1(initializeLaser, bool(const json& systemParameters));
  MOCK_METHOD2(enableChannel, bool(int channelNum, bool channelON));
};

//
// StageScanner
//

// Test with mocks
class TestScanner : public AsyncScanner {
 public:
  TestScanner(
      const json& systemParams, Laser* laser, BerkeleyNucleonics* delays,
      ConexStage* xStage, ConexStage* yStage, ConexStage* zStage,
      USTx* ustx, CameraManager* cameras, OctopusManager* octopusManager)
      : AsyncScanner(systemParams, laser, delays, xStage, yStage, zStage,
            ustx, cameras, octopusManager) {}
  ~TestScanner() { laser_ = NULL; xStage_ = NULL; cameras_ = NULL; octopusManager_ = NULL; }  // keep parent class from deleting stack objects
};

class ScannerTest : public ::testing::Test {
 public:
  ScannerTest() {}
};

TEST(ScannerTest, initCallsStageInit) {
  std::stringstream scanMetadata(TestJSON);
  json systemParameters = json::parse(scanMetadata);
  testing::NiceMock<MockConexStage> xStage;
  TestScanner test(systemParameters, NULL, NULL, &xStage, NULL, NULL, NULL, NULL, NULL);
  EXPECT_CALL(xStage, init(6)).Times(1).WillOnce(Return(true));
  ASSERT_TRUE(test.init());
}

TEST(ScannerTest, initCallsCameraManagerInit) {
  std::stringstream scanMetadata(TestJSON);
  json systemParameters = json::parse(scanMetadata);
  testing::NiceMock<MockCameraManager> cmgr;
  TestScanner test(systemParameters, NULL, NULL, NULL, NULL, NULL, NULL, &cmgr, NULL);
  EXPECT_CALL(cmgr, init(_, _)).Times(1).WillOnce(Return(true));
  ASSERT_TRUE(test.init());
}

TEST(ScannerTest, initFailsIfLaserInitFails) {
  std::stringstream scanMetadata(TestJSON);
  json systemParameters = json::parse(scanMetadata);
  testing::NiceMock<MockLaser> laser;
  TestScanner test(systemParameters, &laser, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
  EXPECT_CALL(laser, initializeLaser(_)).Times(1).WillOnce(Return(false));
  ASSERT_FALSE(test.init());
}

// TDO(jfs): Adapt this for new pipeline, currently writeVoxelData2.
TEST(ScannerTest, scanCallsCameraManagerCaptureAndWriteImages_and_setVoxelEnergyData) {
  std::stringstream scanMetadata(TestJSON);
  json systemParameters = json::parse(scanMetadata);
  testing::NiceMock<MockCameraManager> cmgr;
  TestScanner test(systemParameters, NULL, NULL, NULL, NULL, NULL, NULL, &cmgr, NULL);
  EXPECT_CALL(cmgr, init(_, _)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(cmgr, captureAndWriteImagesAsync(_, _, _, _, _)).Times(1519).WillRepeatedly(Return(true));
  ASSERT_TRUE(test.init());
  ASSERT_TRUE(test.scan());
}

TEST(ScannerTest, initCallsOctopusManagerInit) {
  std::stringstream scanMetadata(TestJSON);
  json systemParameters = json::parse(scanMetadata);
  testing::NiceMock<MockOctopusManager> omgr;
  TestScanner test(systemParameters, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &omgr);
  EXPECT_CALL(omgr, init(_, _, _)).Times(1).WillOnce(Return(true));
  ASSERT_TRUE(test.init());
  EXPECT_CALL(omgr, init(_, _, _)).Times(1).WillOnce(Return(false));
  ASSERT_FALSE(test.init());
}

//
// RobotScanner
//

// Robot is not pure virtual. These are here so we don't haul in PracticalSocket.
Robot::Robot() {}
Robot::~Robot() {}
bool Robot::init() { return false; }
bool Robot::activate(bool) { return false; }
bool Robot::home() { return false; }
bool Robot::setTRF(double, double, double, double, double, double) { return false; }
bool Robot::moveJoints(double, double, double, double, double, double) { return false; }
bool Robot::movePose(double, double, double, double, double, double) { return false; }
bool Robot::moveLinRelTRF(double, double, double, double, double, double) { return false; }
bool Robot::setInitJoints(double, double, double, double, double, double) { return false; }
std::vector<double> Robot::getPose() { return std::vector<double>(6, 0.0); }

class MockRobot: public Robot {
 public:
  MOCK_METHOD0(init, bool());
  MOCK_METHOD1(activate, bool(bool));
  MOCK_METHOD0(home, bool());
  MOCK_METHOD6(setTRF, bool(double, double, double, double, double, double));
};

class TestRobotScanner : public RobotScanner {
 public:
  TestRobotScanner(
      const json& sysParams, Laser* laser, BerkeleyNucleonics* delays,
      Robot* robot, USTx* ustx, CameraManager* cameras, OctopusManager* octoMgr)
      : RobotScanner(
          sysParams, laser, delays, robot, ustx, cameras, octoMgr) {}
  ~TestRobotScanner() { robot_ = NULL; cameras_ = NULL; }  // keep parent class from deleting stack objects
};

TEST(ScannerTest, initFailsIfRobotInitFails) {
  std::stringstream scanMetadata(TestJSON);
  json sysParams = json::parse(scanMetadata);
  testing::NiceMock<MockRobot> robot;
  TestRobotScanner test(sysParams, NULL, NULL, &robot, NULL, NULL, NULL);
  EXPECT_CALL(robot, init()).Times(1).WillOnce(Return(false));
  ASSERT_FALSE(test.init());
}

TEST(ScannerTest, initFailsIfRobotActivateFails) {
  std::stringstream scanMetadata(TestJSON);
  json sysParams = json::parse(scanMetadata);
  testing::NiceMock<MockRobot> robot;
  TestRobotScanner test(sysParams, NULL, NULL, &robot, NULL, NULL, NULL);
  EXPECT_CALL(robot, init()).Times(1).WillOnce(Return(true));
  EXPECT_CALL(robot, activate(true)).Times(1).WillOnce(Return(false));
  ASSERT_FALSE(test.init());
}

TEST(ScannerTest, initFailsIfRobotHomeFails) {
  std::stringstream scanMetadata(TestJSON);
  json sysParams = json::parse(scanMetadata);
  testing::NiceMock<MockRobot> robot;
  TestRobotScanner test(sysParams, NULL, NULL, &robot, NULL, NULL, NULL);
  EXPECT_CALL(robot, init()).Times(1).WillOnce(Return(true));
  EXPECT_CALL(robot, activate(true)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(robot, home()).Times(1).WillOnce(Return(false));
  ASSERT_FALSE(test.init());
}

// TODO(jfs): test requires robot TRF field in metadata
TEST(ScannerTest, initSucceeds) {
  std::stringstream scanMetadata(TestJSON);
  json sysParams = json::parse(scanMetadata);
  testing::NiceMock<MockRobot> robot;
  TestRobotScanner test(sysParams, NULL, NULL, &robot, NULL, NULL, NULL);
  EXPECT_CALL(robot, init()).Times(1).WillOnce(Return(true));
  EXPECT_CALL(robot, activate(true)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(robot, home()).Times(1).WillOnce(Return(true));
  ASSERT_TRUE(test.init());
}
