// written by bastiaan konings schuiling 2008 - 2014
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not
// be used for anything important. i do not offer support, so don't ask. to be used for inspiration
// :)

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
