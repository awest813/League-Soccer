// written by bastiaan konings schuiling 2008 - 2014
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not
// be used for anything important. i do not offer support, so don't ask. to be used for inspiration
// :)

#include "audio_system.hpp"

#include "audio_scene.hpp"
#include "base/log.hpp"
#include "base/utils.hpp"
#include "managers/resourcemanagerpool.hpp"
#include "rendering/audio_messages.hpp"
#include "rendering/null_audio_renderer.hpp"

#ifdef GF_USE_OPENAL
#include "rendering/openal_renderer.hpp"
#endif

namespace blunted {

AudioSystem::AudioSystem() : systemType(e_SystemType_Audio) {
  rendererTask = nullptr;
}

AudioSystem::~AudioSystem() {}

void AudioSystem::Initialize(const Properties& config) {
  audioSoundBufferResourceManager = std::shared_ptr<ResourceManager<AudioSoundBuffer>>(
      new ResourceManager<AudioSoundBuffer>("audiosoundbuffer"));
  ResourceManagerPool::GetInstance().RegisterManager(e_ResourceType_AudioSoundBuffer,
                                                     audioSoundBufferResourceManager);

  const std::string requestedRenderer = config.Get(
      "audio_renderer",
#ifdef GF_USE_OPENAL
      "openal"
#else
      "null"
#endif
  );

#ifdef GF_USE_OPENAL
  if (requestedRenderer == "openal") {
    rendererTask = new OpenALRenderer();
  } else
#endif
  {
    if (requestedRenderer == "openal") {
      Log(e_Warning, "AudioSystem", "Initialize",
          "OpenAL support is unavailable in this build, falling back to silent audio");
    } else if (requestedRenderer != "null") {
      Log(e_Warning, "AudioSystem", "Initialize",
          "Unknown audio renderer '" + requestedRenderer + "', using silent audio");
    }
    rendererTask = new NullAudioRenderer();
  }

  rendererTask->Run();

  boost::intrusive_ptr<AudioRendererMessage_CreateContext> createContext(
      new AudioRendererMessage_CreateContext());
  rendererTask->messageQueue.PushMessage(createContext);
  createContext->Wait();

  if (!createContext->success) {
    // The requested renderer (typically OpenAL) failed to open an audio
    // device at runtime -- this is common on headless machines, CI runners
    // and containers without audio hardware. Fall back to silent audio
    // instead of treating it as fatal, so the game still runs everywhere.
    Log(e_Warning, "AudioSystem", "Initialize",
        "Requested audio renderer failed to create a context; falling back to silent audio");

    boost::intrusive_ptr<Message_Shutdown> shutdown(new Message_Shutdown());
    rendererTask->messageQueue.PushMessage(shutdown);
    shutdown->Wait();
    rendererTask->Join();
    delete rendererTask;

    rendererTask = new NullAudioRenderer();
    rendererTask->Run();

    boost::intrusive_ptr<AudioRendererMessage_CreateContext> fallbackContext(
        new AudioRendererMessage_CreateContext());
    rendererTask->messageQueue.PushMessage(fallbackContext);
    fallbackContext->Wait();

    if (!fallbackContext->success) {
      Log(e_FatalError, "AudioSystem", "Initialize", "Could not create context");
    } else {
      Log(e_Notice, "AudioSystem", "Initialize", "Created silent-audio fallback context");
    }
  } else {
    Log(e_Notice, "AudioSystem", "Initialize", "Created context");
  }
}

void AudioSystem::Exit() {
  audioSoundBufferResourceManager.reset();

  // shutdown renderer thread
  boost::intrusive_ptr<Message_Shutdown> RendererShutdown(new Message_Shutdown());
  rendererTask->messageQueue.PushMessage(RendererShutdown);
  RendererShutdown->Wait();

  rendererTask->Join();
  delete rendererTask;
  rendererTask = nullptr;
}

e_SystemType AudioSystem::GetSystemType() const {
  return systemType;
}

ISystemScene* AudioSystem::CreateSystemScene(std::shared_ptr<IScene> scene) {
  if (scene->GetSceneType() == e_SceneType_Scene2D) {
    AudioScene* audioScene = new AudioScene(this);
    scene->Attach(audioScene->GetInterpreter(e_SceneType_Scene2D));
    return audioScene;
  }
  if (scene->GetSceneType() == e_SceneType_Scene3D) {
    AudioScene* audioScene = new AudioScene(this);
    scene->Attach(audioScene->GetInterpreter(e_SceneType_Scene3D));
    return audioScene;
  }
  return nullptr;
}

ISystemTask* AudioSystem::GetTask() {
  return 0;
}

AudioRenderer* AudioSystem::GetAudioRenderer() {
  return rendererTask;
}

}  // namespace blunted
