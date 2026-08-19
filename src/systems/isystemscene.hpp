#ifndef _HPP_SYSTEMS_IFACE_SCENE
#define _HPP_SYSTEMS_IFACE_SCENE

#include "defines.hpp"
#include "scene/object.hpp"
#include "systems/isystemobject.hpp"
#include "types/command.hpp"
#include "types/observer.hpp"

namespace blunted {

class ISystemScene {
public:
  virtual ~ISystemScene() {}

protected:
};

}  // namespace blunted

#endif
