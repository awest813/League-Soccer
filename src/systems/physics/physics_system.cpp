// written by bastiaan konings schuiling 2008 - 2014
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not
// be used for anything important. i do not offer support, so don't ask. to be used for inspiration
// :)

#include "physics_system.hpp"

#include "base/log.hpp"
#include "base/utils.hpp"
#include "objects/physics_geometry.hpp"
#include "physics_scene.hpp"
#include "wrappers/ode_physics.hpp"

namespace blunted {

PhysicsSystem::PhysicsSystem() : systemType(e_SystemType_Physics) {
  task = nullptr;
}

PhysicsSystem::~PhysicsSystem() {}

void PhysicsSystem::Initialize(const Properties& config) {
  if (config.Get("physics_wrapper", "ode") == "ode")
    physicsWrapper = new OdePhysics();
  task = new PhysicsTask(this);
  task->Run();
}

void PhysicsSystem::Exit() {
  // shutdown system task
  boost::intrusive_ptr<Message_Shutdown> shutdown(new Message_Shutdown());
  task->messageQueue.PushMessage(shutdown);
  shutdown->Wait();

  task->Join();
  delete task;
  task = nullptr;

  delete physicsWrapper;
  physicsWrapper = nullptr;
}

e_SystemType PhysicsSystem::GetSystemType() const {
  return systemType;
}

ISystemScene* PhysicsSystem::CreateSystemScene(std::shared_ptr<IScene> scene) {
  if (scene->GetSceneType() == e_SceneType_Scene3D) {
    PhysicsScene* physicsScene = new PhysicsScene(this);
    scene->Attach(physicsScene->GetInterpreter(e_SceneType_Scene3D));
    return physicsScene;
  }
  return nullptr;
}

ISystemTask* PhysicsSystem::GetTask() {
  return task;
}

IPhysicsWrapper* PhysicsSystem::GetPhysicsWrapper() {
  return physicsWrapper;
}

}  // namespace blunted
