#ifndef _HPP_SYSTEMS_AUDIO_SCENE
#define _HPP_SYSTEMS_AUDIO_SCENE

#include "defines.hpp"
#include "scene/scene2d/scene2d.hpp"
#include "scene/scene3d/scene3d.hpp"
#include "systems/isystemscene.hpp"

namespace blunted {

class AudioSystem;

class AudioScene : public ISystemScene {
public:
  AudioScene(AudioSystem* audioSystem);
  virtual ~AudioScene();

  virtual AudioSystem* GetAudioSystem();

  virtual ISystemObject* CreateSystemObject(boost::intrusive_ptr<Object> object);

  virtual boost::intrusive_ptr<ISceneInterpreter> GetInterpreter(e_SceneType sceneType);

protected:
  AudioSystem* audioSystem;
};

class AudioScene_Scene3DInterpreter : public IScene3DInterpreter {
public:
  AudioScene_Scene3DInterpreter(AudioScene* caller);

  virtual e_SystemType GetSystemType() const { return e_SystemType_Audio; }
  virtual void OnLoad();
  virtual void OnUnload();

  virtual void SetGravity(const Vector3&) {}
  virtual void SetErrorCorrection(float) {}
  virtual void SetConstraintForceMixing(float) {}

  virtual ISystemObject* CreateSystemObject(boost::intrusive_ptr<Object> object);

protected:
  AudioScene* caller;
};

class AudioScene_Scene2DInterpreter : public IScene2DInterpreter {
public:
  AudioScene_Scene2DInterpreter(AudioScene* caller);

  virtual e_SystemType GetSystemType() const { return e_SystemType_Audio; }
  virtual void OnLoad();
  virtual void OnUnload();

  virtual ISystemObject* CreateSystemObject(boost::intrusive_ptr<Object> object);

protected:
  AudioScene* caller;
};

}  // namespace blunted

#endif
