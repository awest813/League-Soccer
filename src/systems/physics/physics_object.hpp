#ifndef _HPP_SYSTEMS_PHYSICS_OBJECT
#define _HPP_SYSTEMS_PHYSICS_OBJECT

#include "defines.hpp"
#include "systems/isystemobject.hpp"

namespace blunted {

class PhysicsScene;

class PhysicsObject : public ISystemObject {
public:
  PhysicsObject(PhysicsScene* physicsScene);
  virtual ~PhysicsObject();

  virtual boost::intrusive_ptr<Interpreter> GetInterpreter(e_ObjectType objectType) = 0;

  PhysicsScene* GetPhysicsScene();

protected:
  PhysicsScene* physicsScene;
};

}  // namespace blunted

#endif
