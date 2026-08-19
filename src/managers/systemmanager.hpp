#ifndef _HPP_MANAGERS_SYSTEM
#define _HPP_MANAGERS_SYSTEM

#include "defines.hpp"
#include "types/singleton.hpp"

namespace blunted {

class ISystem;
class IScene;
class Object;

using map_Systems = std::map<std::string, ISystem*>;

/// manages the registration of systems and the creation of their scenes and objects

class SystemManager : public Singleton<SystemManager> {
public:
  SystemManager();
  virtual ~SystemManager();

  virtual void Exit();
  bool RegisterSystem(const std::string& systemName, ISystem* system);
  void CreateSystemScenes(std::shared_ptr<IScene> scene);
  const map_Systems& GetSystems() const;
  ISystem* GetSystem(const std::string& name) const;

protected:
  map_Systems systems;

private:
};

}  // namespace blunted

#endif
