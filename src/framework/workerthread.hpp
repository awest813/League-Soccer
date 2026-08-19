#ifndef _HPP_WORKERTHREAD
#define _HPP_WORKERTHREAD

#include "types/thread.hpp"

namespace blunted {

class TaskManager;

class WorkerThread : public Thread {
public:
  WorkerThread(int affinity = -1);
  ~WorkerThread();

  void operator()();

  void GetWorkerState(e_ThreadState& state, std::string& commandName);

protected:
  TaskManager* taskManager;
  int affinity;
  unsigned long messagesHandled;

  Lockable<std::string> currentCommandName;
};

}  // namespace blunted

#endif
