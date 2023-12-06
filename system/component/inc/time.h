#pragma once

#include <chrono>
#include <thread>

namespace Component {

// Use this for all your timing needs (esp, in place of GetTickCount[64] on win).
inline time_t SteadyClockTimeMs() {
  std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::steady_clock::now().time_since_epoch());
  return ms.count();
}

// Use this in place of Sleep() on Windows, or the below on Mac/Linux.
inline void SleepMs(time_t ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

}
