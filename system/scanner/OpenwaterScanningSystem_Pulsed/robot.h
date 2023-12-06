#include <string>
#include <vector>

class TCPSocket;

class Robot {
 public:
  Robot();
  virtual ~Robot();

  // Initialize. Delayed from _ct for testability.
  virtual bool init();

  // Activate, deactivate and home the robot.
  virtual bool activate(bool);
  virtual bool home();

  // Set Tool Reference Frame (TRF)
  virtual bool setTRF(double x, double y, double z, double alpha, double beta, double gamma);

  // Move the joints to a particular configuration.
  bool moveJoints(double t1, double t2, double t3, double t4, double t5, double t6);

  // Move the tool to these coords in the TRF.
  bool movePose(double x, double y, double z, double alpha, double beta, double gamma);

  // Move the tool a relative amount, in current TRF coords.
  bool moveLinRelTRF(double x, double y, double z, double alpha, double beta, double gamma);

  // Set a known joint location before setting TRF coords
  bool setInitJoints(double t1, double t2, double t3, double t4, double t5, double t6);

  // Move joint 6
  bool moveJointSix(double j6);

  // Get position of TRF with respect to WRF
  std::vector<double> getPose();

  // Get joint angles
  std::vector<double> getJoints();

 protected:
  // Write a single command to the port, and handle any errors that occur.
  bool write(const std::string&, int delay_ms);

  // Reset the robot state after a network failure.
  bool reset();

  TCPSocket* socket_;
  int errors_ = 0;
  double trf_[6];
  double joints_[6];
  double currentPose_[6];
};
