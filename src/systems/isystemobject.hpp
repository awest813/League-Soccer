#ifndef _HPP_SYSTEMS_IFACE_OBJECT
#define _HPP_SYSTEMS_IFACE_OBJECT

#include "defines.hpp"
#include "scene/object.hpp"
#include "types/interpreter.hpp"

namespace blunted {

class ISystemObject {
public:
  virtual ~ISystemObject() {};

  virtual boost::intrusive_ptr<Interpreter> GetInterpreter(e_ObjectType objectType) = 0;

protected:
};

}  // namespace blunted

#endif
