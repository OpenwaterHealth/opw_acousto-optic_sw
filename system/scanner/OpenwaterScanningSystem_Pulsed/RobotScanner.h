#include "scanner.h"

class Robot;

class RobotScanner: public Scanner {
 public:
  RobotScanner(
    const json& systemParameters, Laser* laser, BerkeleyNucleonics* delays,
    Robot* robot, USTx* ustx, CameraManager* cameras, OctopusManager* octopusManager);
  ~RobotScanner();

  // Implement/Override from scanner
  bool init() override;
  bool scan() override;

 protected:
  Robot* robot_;
};
