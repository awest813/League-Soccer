#ifndef _HPP_BLUNTED
#define _HPP_BLUNTED

#include "defines.hpp"

namespace blunted {

class Scheduler;
class Properties;

/// load managers, systems, scheduler and scene
void Initialize(Properties& config);

/// run the scheduler
void Run();

Scheduler* GetScheduler();

/// unload all
void Exit();
}  // namespace blunted

#endif
