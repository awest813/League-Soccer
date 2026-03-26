// written by bastiaan konings schuiling 2008 - 2014
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not be used for anything important.
// i do not offer support, so don't ask. to be used for inspiration :)

#include "environmentmanager.hpp"

#include <SDL2/SDL.h>
#include <chrono>
#include <thread>

namespace blunted {

  template<> EnvironmentManager* Singleton<EnvironmentManager>::singleton = nullptr;

  EnvironmentManager::EnvironmentManager() {
    quit.SetData(false);
    startTime = std::chrono::steady_clock::now();
  };

  EnvironmentManager::~EnvironmentManager() {
  };

  unsigned long EnvironmentManager::GetTime_ms() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
  }

  void EnvironmentManager::Pause_ms(int duration) {
    std::this_thread::sleep_for(std::chrono::milliseconds(duration));
  }

  void EnvironmentManager::SignalQuit() {
    quit.SetData(true);
  }

  bool EnvironmentManager::GetQuit() {
    return quit.GetData();
  }

}
