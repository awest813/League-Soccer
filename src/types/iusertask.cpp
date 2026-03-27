// written by bastiaan konings schuiling 2008 - 2014
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not
// be used for anything important. i do not offer support, so don't ask. to be used for inspiration
// :)

#include "iusertask.hpp"

#include "blunted.hpp"
#include "framework/scheduler.hpp"

namespace blunted {

bool UserTaskMessage_GetPhase::Execute(void* caller) {
  task->GetPhase();
  std::unique_lock<std::mutex> lock(GetScheduler()->somethingIsDoneMutex);
  GetScheduler()->somethingIsDone.notify_one();
  return true;
}

bool UserTaskMessage_ProcessPhase::Execute(void* caller) {
  task->ProcessPhase();
  std::unique_lock<std::mutex> lock(GetScheduler()->somethingIsDoneMutex);
  GetScheduler()->somethingIsDone.notify_one();
  return true;
}

bool UserTaskMessage_PutPhase::Execute(void* caller) {
  task->PutPhase();
  std::unique_lock<std::mutex> lock(GetScheduler()->somethingIsDoneMutex);
  GetScheduler()->somethingIsDone.notify_one();
  return true;
}

}  // namespace blunted
