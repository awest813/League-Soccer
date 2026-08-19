#ifndef _HPP_SCENE
#define _HPP_SCENE

#include "defines.hpp"
#include "iscene.hpp"
#include "types/lockable.hpp"

namespace blunted {

using vector_Objects = Lockable<std::vector<boost::intrusive_ptr<Object>>>;

class Scene : public IScene {
public:
  Scene(const std::string& name, e_SceneType sceneType);
  virtual ~Scene();

  virtual void Init() = 0;  // ATOMIC
  virtual void Exit();      // ATOMIC

  virtual void CreateSystemObjects(boost::intrusive_ptr<Object> object);

  virtual const std::string GetName() const;
  virtual e_SceneType GetSceneType() const;

  virtual bool SupportedObjectType(e_ObjectType objectType) const;

protected:
  std::string name;

  e_SceneType sceneType;

  std::vector<e_ObjectType> supportedObjectTypes;
};

}  // namespace blunted

#endif
