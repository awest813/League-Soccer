#ifndef _HPP_LOCKABLE
#define _HPP_LOCKABLE

#include "defines.hpp"

namespace blunted {

enum e_NotificationSubject {
  e_NotificationSubject_One,
  e_NotificationSubject_All,
};

template <typename T>
class Lockable {
public:
  Lockable() {}

  T GetData() const {
    std::lock_guard<std::mutex> lock(mutex);
    return data;
  }

  void SetData(const T& newdata) {
    std::lock_guard<std::mutex> lock(mutex);
    data = newdata;
  }

  inline T operator=(const T& param) {
    SetData(param);
    return param;
  }

  inline T* operator->() { return &data; }

  inline void Lock() { mutex.lock(); }

  inline void Unlock() { mutex.unlock(); }

  T data;

  mutable std::mutex mutex;

protected:
};

}  // namespace blunted

#endif
