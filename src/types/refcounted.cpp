#include "refcounted.hpp"

namespace blunted {

RefCounted::RefCounted() : refCount(0) {}

RefCounted::~RefCounted() {}

RefCounted::RefCounted(const RefCounted& src) : refCount(0) {}

RefCounted& RefCounted::operator=(const RefCounted& src) {
  return *this;
}

unsigned long RefCounted::GetRefCount() {
  int i = refCount;
  return i;
}

void intrusive_ptr_add_ref(RefCounted* p) {
  assert(p);
  ++(p->refCount);
}

void intrusive_ptr_release(RefCounted* p) {
  assert(p);
  if (--(p->refCount) == 0)
    delete p;
}

}  // namespace blunted
