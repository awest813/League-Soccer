#include "systemmanager.hpp"

#include "base/log.hpp"
#include "scene/object.hpp"
#include "scenemanager.hpp"
#include "systems/isystem.hpp"

namespace blunted {

template <>
SystemManager* Singleton<SystemManager>::singleton = nullptr;

SystemManager::SystemManager() {}

SystemManager::~SystemManager() {}

void SystemManager::Exit() {
  for (auto& [name, system] : systems) {
    Log(e_Notice, "SystemManager", "Exit", "Deleting system named '" + name + "'");
    system->Exit();
    delete system;
  }
  systems.clear();
}

bool SystemManager::RegisterSystem(const std::string& systemName, ISystem* system) {
  auto [iter, success] = systems.insert({systemName, system});
  if (!success) {
    // property already exists, replace its value?
    return false;
  }
  Log(e_Notice, "SystemManager", "RegisterSystem", "Added system named '" + systemName + "'");
  return true;
}

void SystemManager::CreateSystemScenes(std::shared_ptr<IScene> scene) {
  for (auto& [name, system] : systems) {
    system->CreateSystemScene(scene);
  }
  scene->Init();
}

const map_Systems& SystemManager::GetSystems() const {
  return systems;
}

ISystem* SystemManager::GetSystem(const std::string& name) const {
  auto s_iter = systems.find(name);
  if (s_iter != systems.end()) {
    return s_iter->second;
  } else {
    return nullptr;
  }
}

}  // namespace blunted
