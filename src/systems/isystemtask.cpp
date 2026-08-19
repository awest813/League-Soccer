#include "isystemtask.hpp"

#include "blunted.hpp"
#include "framework/scheduler.hpp"

namespace blunted {

bool SystemTaskMessage_GetPhase::Execute(void*) {
  task->GetPhase();
  std::unique_lock<std::mutex> lock(GetScheduler()->somethingIsDoneMutex);
  GetScheduler()->somethingIsDone.notify_one();
  return true;
}

bool SystemTaskMessage_ProcessPhase::Execute(void*) {
  task->ProcessPhase();
  std::unique_lock<std::mutex> lock(GetScheduler()->somethingIsDoneMutex);
  GetScheduler()->somethingIsDone.notify_one();
  return true;
}

bool SystemTaskMessage_PutPhase::Execute(void*) {
  task->PutPhase();
  std::unique_lock<std::mutex> lock(GetScheduler()->somethingIsDoneMutex);
  GetScheduler()->somethingIsDone.notify_one();
  return true;
}

}  // namespace blunted
