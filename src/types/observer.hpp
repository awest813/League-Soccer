#ifndef _HPP_OBSERVER
#define _HPP_OBSERVER

#include "command.hpp"
#include "defines.hpp"
#include "types/refcounted.hpp"

namespace blunted {

class Observer : public RefCounted {
public:
  Observer();
  virtual ~Observer();

  void SetSubjectPtr(void* subjectPtr);

protected:
  void* subjectPtr;
};

}  // namespace blunted

#endif
