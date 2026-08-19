#ifndef _HPP_COMMAND
#define _HPP_COMMAND

#include "defines.hpp"
#include "types/lockable.hpp"
#include "types/refcounted.hpp"

namespace blunted {

class Command : public RefCounted {
public:
  Command(const std::string& name);
  virtual ~Command();

  bool IsReady();
  void Reset();
  bool Handle(void* caller = nullptr);
  void Wait();

  std::string GetName() const { return name.GetData(); }

protected:
  virtual bool Execute(void* caller = nullptr) = 0;

  std::mutex mutex;  // locks 'handled & processed'
  bool handled;
  std::condition_variable processed;

  Lockable<std::string> name;
};
}  // namespace blunted

#endif
