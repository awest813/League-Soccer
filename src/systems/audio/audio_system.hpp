#ifndef _HPP_SYSTEMS_AUDIO_SYSTEM
#define _HPP_SYSTEMS_AUDIO_SYSTEM

#include "defines.hpp"
#include "managers/resourcemanager.hpp"
#include "resources/audio_soundbuffer.hpp"
#include "scene/iscene.hpp"
#include "systems/audio/rendering/interface_audiorenderer.hpp"
#include "systems/isystem.hpp"
#include "systems/isystemscene.hpp"

namespace blunted {

class AudioSystem : public ISystem {
public:
  AudioSystem();
  virtual ~AudioSystem();

  virtual void Initialize(const Properties& config) override;
  virtual void Exit() override;

  virtual e_SystemType GetSystemType() const override;

  virtual ISystemScene* CreateSystemScene(std::shared_ptr<IScene> scene) override;

  virtual ISystemTask* GetTask() override;
  virtual AudioRenderer* GetAudioRenderer();

  std::shared_ptr<ResourceManager<AudioSoundBuffer>> GetAudioSoundBufferResourceManager();

  virtual std::string GetName() const override { return "audio"; }

protected:
  const e_SystemType systemType;

  AudioRenderer* rendererTask;

  std::shared_ptr<ResourceManager<AudioSoundBuffer>> audioSoundBufferResourceManager;
};

}  // namespace blunted

#endif
