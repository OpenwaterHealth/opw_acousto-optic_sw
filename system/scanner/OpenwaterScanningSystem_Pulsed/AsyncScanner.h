#include "scanner.h"

class ConexStage;
class Stage;

class AsyncScanner : public Scanner {
public:
  using json = nlohmann::json;
  AsyncScanner(
    const json& systemParameters, Laser* laser, BerkeleyNucleonics* delays,
    ConexStage* xStage, ConexStage* yStage, ConexStage* zStage, 
    USTx* ustx, CameraManager* cameras, OctopusManager* octopusManager);
  ~AsyncScanner();

  // Implement/override from Scanner
  bool init() override;
  bool scan() override;

protected:
  bool initializeStage(Stage* stage, int comPort);

  ConexStage* xStage_, * yStage_, * zStage_;
};
