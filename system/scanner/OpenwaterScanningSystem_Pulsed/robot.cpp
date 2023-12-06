#define _CRT_SECURE_NO_WARNINGS 1

#include "robot.h"

#include <iostream>

#include "system/component/inc/time.h"
#include "system/third_party/practical-socket/PracticalSocket.h"

Robot::Robot(): socket_(NULL) {
  trf_[0] = trf_[1] = trf_[2] = trf_[3] = trf_[4] = trf_[5] = 0;
}

Robot::~Robot() {
  delete socket_;
}

bool Robot::reset() {
  if (!init()) return false;
  if (!activate(true)) return false;
  if (!home()) return false;
  if (!moveJoints(joints_[0], joints_[1], joints_[2], joints_[3], joints_[4], joints_[5])) return false;
  if (!setTRF(trf_[0], trf_[1], trf_[2], trf_[3], trf_[4], trf_[5])) return false;
  if (!write("ResumeMotion", 1000)) return false;
  return true;
}

bool Robot::write(const std::string& s, int delay) {
  const int retries = 5;
  for (int i = 0; i < retries; ++i) {
    const char* op = nullptr;
    try {
      std::cout << "We say: " << s << std::endl;
      op = "send";
      socket_->send(s.c_str(), int(s.length() + 1));  // Send the null.

      if (delay) {
        Component::SleepMs(delay);
        op = "recv";
        char buffer[256];
        if (int recvd = socket_->recv(buffer, sizeof(buffer))) {
          std::cout << "Robot says: " << std::string(buffer, buffer + recvd) << std::endl;
          std::cout << "Error count: " << errors_ << std::endl;

          // If it's a status command, check the return.
          if (s.compare("GetStatusRobot") == 0) {
            int as = -1, hs = -1, sm = -1, es = -1, pm = -1, eob = -1, eom = -1;
            sscanf(buffer, "[2007][%d, %d, %d, %d, %d, %d, %d]", &as, &hs, &sm, &es, &pm, &eob, &eom);
            if (es == 1) {
              std::cout << "Robot error condition detected; attempting reset." << std::endl;
              if (!write("ResetError", 200)) return false;
            }
            if (pm == 1) {
              std::cout << "Robot pause motion condition detected; attempting to resume." << std::endl;
              if (!write("ClearMotion", 1000)) return false;
              if (!write("ResumeMotion", 1000)) return false;
            }
          } else if (s.compare("GetPose") == 0) {
            double x = 0, y = 0, z = 0, alpha = 0, beta = 0, gamma = 0;
            sscanf(buffer, "[2027][%lf, %lf, %lf, %lf, %lf, %lf]", &x, &y, &z, &alpha, &beta, &gamma);
            currentPose_[0] = x;
            currentPose_[1] = y;
            currentPose_[2] = z;
            currentPose_[3] = alpha;
            currentPose_[4] = beta;
            currentPose_[5] = gamma;
          } else if (s.compare("GetJoints") == 0) {
            double j1 = 0, j2 = 0, j3 = 0, j4 = 0, j5 = 0, j6 = 0;
            sscanf(buffer, "[2026][%lf, %lf, %lf, %lf, %lf, %lf]", &j1, &j2, &j3, &j4, &j5, &j6);
            joints_[0] = j1;
            joints_[1] = j2;
            joints_[2] = j3;
            joints_[3] = j4;
            joints_[4] = j5;
            joints_[5] = j6;
          }
        }
      }

      // If we reach this point, there've been no exceptions thrown.
      break;
    } catch (const SocketException& e) {
      ++errors_;
      std::cout << "Socket " << op << " exception: " << e.what() << "; attempting retry." << std::endl;
      Component::SleepMs(5000);
      // Try running the same sequence as at start-up.
      if (!reset()) return false;
    }
  }
  return true;
}

bool Robot::init() {
  try {
    const char* const RobotAddress = "192.168.0.100";
    const int RobotSocket = 10000;
    socket_ = new TCPSocket(RobotAddress, RobotSocket);
    std::cout << "Found robot at " << RobotAddress << ":" << RobotSocket << "." << std::endl;
  } catch(const SocketException& e) {
    std::cout << "Socket init exception: " << e.what() << std::endl;
    return false;
  }
  Component::SleepMs(100);

  // Expecting "[3000][Connected to Meca500 3_7.0.6]" here.
  try {
    char buffer[256];
    if (int recvd = socket_->recv(buffer, sizeof(buffer))) {
      std::cout << "Robot says: " << std::string(buffer, buffer + recvd) << std::endl;
    }
  } catch (const SocketException & e) {
    std::cout << "Socket recv exception: " << e.what() << std::endl;
    return false;
  }
  return true;
}

bool Robot::activate(bool sense) {
  if (!write("ResetError", 200)) {
    return false;
  }
  return write(sense ? "ActivateRobot" : "DeactivateRobot", 2000);
}

bool Robot::home() {
  return write("Home", 5000);  // manual says homing takes about 4 seconds
}

bool Robot::setTRF(double x, double y, double z, double alpha, double beta, double gamma) {
  char buf[256];
  snprintf(buf, 256, "SetTRF(%g,%g,%g,%g,%g,%g)", x, y, z, alpha, beta, gamma);
  bool result = write(std::string(buf), 15);  // End of block expected
  if (result == true) {
    trf_[0] = x, trf_[1] = y, trf_[2] = z, trf_[3] = alpha, trf_[4] = beta, trf_[5] = gamma;
  }
  return result;
}

bool Robot::moveJoints(double t1, double t2, double t3, double t4, double t5, double t6) {
  char buf[256];
  snprintf(buf, 256, "MoveJoints(%g,%g,%g,%g,%g,%g)", t1, t2, t3, t4, t5, t6);
  return write(std::string(buf), 250);
}

bool Robot::movePose(double x, double y, double z, double alpha, double beta, double gamma) {
  // Check for robot error status.
  if (!write("GetStatusRobot", 10)) return false;

  char buf[256];
  snprintf(buf, 256, "MovePose(%g,%g,%g,%g,%g,%g)", x, y, z, alpha, beta, gamma);
  return write(std::string(buf), 500);
}

bool Robot::moveLinRelTRF(double x, double y, double z, double alpha, double beta, double gamma) {
  char buf[256];
  snprintf(buf, 256, "MoveLinRelTRF(%g,%g,%g,%g,%g,%g)", x, y, z, alpha, beta, gamma);
  return write(std::string(buf), 250);
}

bool Robot::setInitJoints(double j1, double j2, double j3, double j4, double j5, double j6) {
  joints_[0] = j1, joints_[1] = j2, joints_[2] = j3, joints_[3] = j4, joints_[4] = j5, joints_[5] = j6;
  return true;
}

std::vector<double> Robot::getPose() {
  write("GetPose", 15);
  std::vector<double> currentPose;
  for (int i = 0; i < 6; i++) {
    currentPose.push_back(currentPose_[i]);
  }
  return currentPose;
}

std::vector<double> Robot::getJoints() {
  write("GetJoints", 15);
  std::vector<double> currentJoints;
  for (int i = 0; i < 6; i++) {
    currentJoints.push_back(joints_[i]);
  }
  return currentJoints;
}

bool Robot::moveJointSix(double j6) {
  return moveJoints(joints_[0], joints_[1], joints_[2], joints_[3], joints_[4], j6);
}
