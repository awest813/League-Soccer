#include "physics_object.hpp"

#include "physics_scene.hpp"

namespace blunted {

PhysicsObject::PhysicsObject(PhysicsScene* physicsScene) : physicsScene(physicsScene) {}

PhysicsObject::~PhysicsObject() {}

PhysicsScene* PhysicsObject::GetPhysicsScene() {
  return physicsScene;
}

}  // namespace blunted
