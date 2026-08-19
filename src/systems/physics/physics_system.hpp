#ifndef _HPP_SYSTEMS_PHYSICS_SYSTEM
#define _HPP_SYSTEMS_PHYSICS_SYSTEM

#include "defines.hpp"
#include "physics_task.hpp"
#include "scene/iscene.hpp"
#include "systems/isystem.hpp"
#include "systems/isystemscene.hpp"
#include "wrappers/ode_physics.hpp"

namespace blunted {

class Renderer3D;

class PhysicsSystem : public ISystem {
public:
  PhysicsSystem();
  virtual ~PhysicsSystem();

  virtual void Initialize(const Properties& config);
  virtual void Exit();

  virtual e_SystemType GetSystemType() const;

  virtual ISystemScene* CreateSystemScene(std::shared_ptr<IScene> scene);

  virtual ISystemTask* GetTask();
  virtual IPhysicsWrapper* GetPhysicsWrapper();

  virtual std::string GetName() const { return "physics"; }

protected:
  const e_SystemType systemType;

  IPhysicsWrapper* physicsWrapper;
  PhysicsTask* task;
};

}  // namespace blunted

#endif
