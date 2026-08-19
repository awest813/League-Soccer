#ifndef _HPP_REFCOUNTED
#define _HPP_REFCOUNTED

#include <atomic>

#include "defines.hpp"

namespace blunted {

class RefCounted {
public:
  RefCounted();
  virtual ~RefCounted();

  RefCounted(const RefCounted& src);
  RefCounted& operator=(const RefCounted& src);

  unsigned long GetRefCount();

protected:
private:
  std::atomic<long> refCount;

  friend void intrusive_ptr_add_ref(RefCounted* p);
  friend void intrusive_ptr_release(RefCounted* p);
};

void intrusive_ptr_add_ref(RefCounted* p);
void intrusive_ptr_release(RefCounted* p);

}  // namespace blunted

#endif
