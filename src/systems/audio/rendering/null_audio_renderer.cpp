#include "null_audio_renderer.hpp"

#include "base/log.hpp"
#include "base/utils.hpp"
#include "types/command.hpp"

namespace blunted {

NullAudioRenderer::NullAudioRenderer() : nextBufferId(1) {}

NullAudioRenderer::~NullAudioRenderer() {}

bool NullAudioRenderer::CreateContext() {
  return true;
}

void NullAudioRenderer::Exit() {}

int NullAudioRenderer::CreateAudioSoundBuffer(const WavData* /*wavData*/) {
  return nextBufferId++;
}

void NullAudioRenderer::DeleteAudioSoundBuffer(int /*audioSoundBufferID*/) {}

void NullAudioRenderer::PlayAudioSoundBuffer(int /*audioSoundBufferID*/) {}

void NullAudioRenderer::SetListenerParameters(const Vector3& /*position*/,
                                              const Vector3& /*velocity*/,
                                              const Quaternion& /*orientation*/) {}

void NullAudioRenderer::SetSourceParameter(int /*audioSoundBufferID*/,
                                           e_AudioRenderer_SourceParameter /*parameter*/,
                                           float /*value*/) {}

void NullAudioRenderer::SetSourcePosition(int /*audioSoundBufferID*/,
                                          const Vector3& /*position*/) {}

void NullAudioRenderer::operator()() {
  Log(e_Notice, "NullAudioRenderer", "operator()()", "Starting NullAudioRenderer thread");

  bool quit = false;
  while (!quit) {
    boost::intrusive_ptr<Command> message = messageQueue.WaitForMessage();
    if (!message->Handle(this))
      quit = true;
    message.reset();
  }

  Exit();

  Log(e_Notice, "NullAudioRenderer", "operator()()", "Shutting down NullAudioRenderer thread");

  if (messageQueue.GetPending() > 0)
    Log(e_Error, "NullAudioRenderer", "operator()()",
        int_to_str(messageQueue.GetPending()) + " messages left on quit!");
}

}  // namespace blunted
