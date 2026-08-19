#ifndef _HPP_TYPES_SINGLETON
#define _HPP_TYPES_SINGLETON

#include "defines.hpp"

namespace blunted {

template <typename T>
class Singleton {
public:
  Singleton() {
    assert(!singleton);
    singleton = static_cast<T*>(this);
  }

  virtual ~Singleton() {
    // assert(singleton);  // actually, if it's already deleted via Destroy(), this might be called again?
    // wait, if Destroy() calls delete singleton, it calls this destructor.
    // So this is correct.
    singleton = nullptr;
  }

  static T& GetInstance() {
    assert(singleton);
    return (*singleton);
  }

  static T* GetInstancePtr() { return singleton; }

  virtual void Destroy() {
    delete singleton;
    singleton = nullptr;
  }

protected:
  static T* singleton;
};

}  // namespace blunted

#endif
