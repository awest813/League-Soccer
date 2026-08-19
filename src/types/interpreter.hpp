#ifndef _HPP_INTERPRETER
#define _HPP_INTERPRETER

#include "base/log.hpp"
#include "systems/isystem.hpp"
#include "types/observer.hpp"

namespace blunted {

class Interpreter : public Observer {
public:
  virtual e_SystemType GetSystemType() const = 0;
  virtual void OnSynchronize() {
    Log(e_FatalError, "Interpreter", "OnSynchronize",
        "OnSynchronize not written yet for this object! N00B!");
  }

protected:
};

}  // namespace blunted

#endif
