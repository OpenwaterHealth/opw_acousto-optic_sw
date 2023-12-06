#include "scanner.h"

class AmpStage;

// TODO(jfs): Subclass from StageScanner?
class RotisserieScanner: public Scanner {
 public:
  RotisserieScanner(
    const json& systemParameters, Laser* laser, BerkeleyNucleonics* delays,
    ConexStage* xStage_, ConexStage* yStage_, ConexStage* zStage_, AmpStage* rStage,
    USTx* ustx, CameraManager* cameras, OctopusManager* octopusManager);
  ~RotisserieScanner();

  // Implement/Override from scanner
  bool init() override;
  bool scan() override;

 protected:
  bool initializeStage(Stage* stage, int comPort);

  ConexStage* xStage_;
  ConexStage* yStage_;
  ConexStage* zStage_;
  AmpStage* rStage_;
};
