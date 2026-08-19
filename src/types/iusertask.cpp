#include "iusertask.hpp"

#include "blunted.hpp"
#include "framework/scheduler.hpp"

namespace blunted {

bool UserTaskMessage_GetPhase::Execute(void*) {
  task->GetPhase();
  std::unique_lock<std::mutex> lock(GetScheduler()->somethingIsDoneMutex);
  GetScheduler()->somethingIsDone.notify_one();
  return true;
}

bool UserTaskMessage_ProcessPhase::Execute(void*) {
  task->ProcessPhase();
  std::unique_lock<std::mutex> lock(GetScheduler()->somethingIsDoneMutex);
  GetScheduler()->somethingIsDone.notify_one();
  return true;
}

bool UserTaskMessage_PutPhase::Execute(void*) {
  task->PutPhase();
  std::unique_lock<std::mutex> lock(GetScheduler()->somethingIsDoneMutex);
  GetScheduler()->somethingIsDone.notify_one();
  return true;
}

}  // namespace blunted
