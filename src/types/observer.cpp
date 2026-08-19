#include "observer.hpp"

namespace blunted {

Observer::Observer() {
  subjectPtr = 0;
}

Observer::~Observer() {}

void Observer::SetSubjectPtr(void* subjectPtr) {
  this->subjectPtr = subjectPtr;
}

}  // namespace blunted
