#ifndef _HPP_MANAGERS_ENVIRONMENT
#define _HPP_MANAGERS_ENVIRONMENT

#include "defines.hpp"
#include "types/lockable.hpp"
#include "types/singleton.hpp"

namespace blunted {

class EnvironmentManager : public Singleton<EnvironmentManager> {
public:
  EnvironmentManager();
  virtual ~EnvironmentManager();

  void SignalQuit();
  bool GetQuit();

  unsigned long GetTime_ms();
  void Pause_ms(int duration);

protected:
  Lockable<bool> quit;
  unsigned long startTime_ms;
  std::chrono::steady_clock::time_point startTime;
};

}  // namespace blunted

#endif
