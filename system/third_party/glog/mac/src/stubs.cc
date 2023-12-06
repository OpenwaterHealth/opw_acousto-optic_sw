#include <iostream>
#include <glog/logging.h>
namespace google {
std::ostream& LogMessage::stream() { return std::cerr; }
LogMessage::LogMessage(char const*, int) {}
LogMessage::LogMessage(char const*, int, int) {}
LogMessage::~LogMessage() {}
}
