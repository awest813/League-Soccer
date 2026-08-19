#ifndef _HPP_SYSTEMS_IFACE_TASK
#define _HPP_SYSTEMS_IFACE_TASK

#include "base/log.hpp"
#include "base/properties.hpp"
#include "types/command.hpp"
#include "types/thread.hpp"

namespace blunted {

class ISystemTask : public Thread {
public:
  ISystemTask() {}

  virtual ~ISystemTask() {}

  virtual void operator()() = 0;

  /// the actual per-frame system work
  virtual void GetPhase() = 0;
  virtual void ProcessPhase() = 0;
  virtual void PutPhase() = 0;

protected:
};

// messages

class ISystemTaskMessage : public Command {
public:
  ISystemTaskMessage(const std::string& name, ISystemTask* task) : Command(name), task(task) {};
  ISystemTask* GetTask() { return task; }

protected:
  ISystemTask* task;
};

class SystemTaskMessage_GetPhase : public ISystemTaskMessage {
public:
  SystemTaskMessage_GetPhase(const std::string& name, ISystemTask* task)
      : ISystemTaskMessage(name, task) {};

protected:
  virtual bool Execute(void* caller = nullptr);
};

class SystemTaskMessage_ProcessPhase : public ISystemTaskMessage {
public:
  SystemTaskMessage_ProcessPhase(const std::string& name, ISystemTask* task)
      : ISystemTaskMessage(name, task) {};

protected:
  virtual bool Execute(void* caller = nullptr);
};

class SystemTaskMessage_PutPhase : public ISystemTaskMessage {
public:
  SystemTaskMessage_PutPhase(const std::string& name, ISystemTask* task)
      : ISystemTaskMessage(name, task) {};

protected:
  virtual bool Execute(void* caller = nullptr);
};

}  // namespace blunted

#endif
