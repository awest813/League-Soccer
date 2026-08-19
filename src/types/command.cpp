#include "command.hpp"

namespace blunted {

Command::Command(const std::string& name) : handled(false) {
  this->name.SetData(name);
}

Command::~Command() {}

bool Command::IsReady() {
  std::unique_lock<std::mutex> lock(mutex);
  return handled;
}

void Command::Reset() {
  std::unique_lock<std::mutex> lock(mutex);
  handled = false;
}

bool Command::Handle(void* caller) {
  bool result = Execute(caller);

  std::unique_lock<std::mutex> lock(mutex);
  handled = true;
  // lock.unlock(); //
  // http://www.justsoftwaresolutions.co.uk/threading/implementing-a-thread-safe-queue-using-condition-variables.html
  //  edit: disabled, just to be sure no obscure bugs occur.
  processed.notify_one();

  return result;
}

void Command::Wait() {
  std::unique_lock<std::mutex> lock(mutex);
  if (!handled) {
    processed.wait(lock);
  }
}

}  // namespace blunted
