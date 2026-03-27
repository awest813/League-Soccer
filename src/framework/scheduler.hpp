// written by bastiaan konings schuiling 2008 - 2014
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not
// be used for anything important. i do not offer support, so don't ask. to be used for inspiration
// :)

#ifndef _HPP_SCHEDULER
#define _HPP_SCHEDULER

#include "defines.hpp"
#include "systems/isystemtask.hpp"
#include "tasksequence.hpp"
#include "types/iusertask.hpp"

namespace blunted {

class TaskManager;

struct TaskSequenceProgram {
  std::shared_ptr<TaskSequence> taskSequence;
  int programCounter;
  int previousProgramCounter;
  unsigned long sequenceStartTime;  // todo: add _ms to varnames like this one
  unsigned long lastSequenceTime;
  unsigned long startTime;
  int timesRan;
  // set to true to let sequence finish, but not restart
  bool dueQuit;
  bool paused;
  // set to true if sequence is finished
  bool readyToQuit;
};

// sort of 'light version' of the above, meant as informative to return to nosey enquirers
struct TaskSequenceInfo {
  TaskSequenceInfo() {
    sequenceStartTime_ms = 0;
    lastSequenceTime_ms = 0;
    startTime_ms = 0;
    sequenceTime_ms = 0;
    timesRan = 0;
  }
  unsigned long sequenceStartTime_ms;
  unsigned long lastSequenceTime_ms;
  unsigned long startTime_ms;
  int sequenceTime_ms;
  int timesRan;
};

struct TaskSequenceQueueEntry {
  TaskSequenceQueueEntry() { timeUntilDueEntry_ms = 0; }
  std::shared_ptr<TaskSequenceProgram> program;
  long timeUntilDueEntry_ms;

  bool operator<(const TaskSequenceQueueEntry& other) const {
    return timeUntilDueEntry_ms < other.timeUntilDueEntry_ms;
  }
};

class Scheduler {
public:
  Scheduler(TaskManager* taskManager);
  virtual ~Scheduler();

  void Exit();

  int GetSequenceCount();
  void RegisterTaskSequence(std::shared_ptr<TaskSequence> sequence);
  void UnregisterTaskSequence(std::shared_ptr<TaskSequence> sequence);
  void UnregisterTaskSequence(const std::string& name);
  void PauseTaskSequence(const std::string& name);
  void UnpauseTaskSequence(const std::string& name);
  void ResetTaskSequenceTime(const std::string& name);
  unsigned long GetTaskSequenceTime_ms(const std::string& name);
  TaskSequenceInfo GetTaskSequenceInfo(const std::string& name);

  /// send due system tasks a SystemTaskMessage_StartFrame message
  /// invoke due user tasks with an Execute() call
  bool Run();

  std::condition_variable somethingIsDone;
  std::mutex somethingIsDoneMutex;

protected:
  TaskManager* taskManager;

  unsigned long previousTime_ms;

  Lockable<std::vector<std::shared_ptr<TaskSequenceProgram>>> sequences;

  unsigned long cleanUpTimeOffset;
};

}  // namespace blunted

#endif
