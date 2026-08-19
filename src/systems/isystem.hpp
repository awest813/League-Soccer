#ifndef _HPP_SYSTEMS_IFACE_SYSTEM
#define _HPP_SYSTEMS_IFACE_SYSTEM

#include "base/properties.hpp"
#include "defines.hpp"
#include "isystemtask.hpp"

namespace blunted {

enum e_SystemType {
  e_SystemType_None = 0,
  e_SystemType_Graphics = 1,
  e_SystemType_Physics = 2,
  e_SystemType_Audio = 3,
  e_SystemType_UserStart = 4
};

class ISystemScene;
class IScene;
class ISystemObject;
class Object;

class ISystem {
public:
  virtual ~ISystem() {};

  virtual void Initialize(const Properties& config) = 0;
  virtual void Exit() = 0;

  virtual e_SystemType GetSystemType() const = 0;

  virtual ISystemScene* CreateSystemScene(std::shared_ptr<IScene> scene) = 0;

  /// returns the systemtask belonging to this system
  virtual ISystemTask* GetTask() = 0;

  virtual std::string GetName() const = 0;

protected:
};

}  // namespace blunted

#endif
