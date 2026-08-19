#include "audio_messages.hpp"

#include "../resources/audio_soundbuffer.hpp"

namespace blunted {

bool AudioRendererMessage_CreateAudioSoundBuffer::Execute(void* caller) {
  AudioRenderer* renderer = static_cast<AudioRenderer*>(caller);

  audioSoundBufferID = renderer->CreateAudioSoundBuffer(wavData);

  return true;
}

bool AudioRendererMessage_DeleteAudioSoundBuffer::Execute(void* caller) {
  AudioRenderer* renderer = static_cast<AudioRenderer*>(caller);
  renderer->SetSourceParameter(audioSoundBufferID, e_AudioRenderer_SourceParameter_Gain, 0);
  renderer->DeleteAudioSoundBuffer(audioSoundBufferID);

  return true;
}

bool AudioRendererMessage_PlayAudioSoundBuffer::Execute(void* caller) {
  AudioRenderer* renderer = static_cast<AudioRenderer*>(caller);

  renderer->PlayAudioSoundBuffer(audioSoundBufferID);

  return true;
}

bool AudioRendererMessage_ConfigAudioSoundBuffer::Execute(void* caller) {
  AudioRenderer* renderer = static_cast<AudioRenderer*>(caller);

  renderer->SetSourceParameter(audioSoundBufferID, e_AudioRenderer_SourceParameter_Gain, gain);
  renderer->SetSourceParameter(audioSoundBufferID, e_AudioRenderer_SourceParameter_Pitch, pitch);
  renderer->SetSourceParameter(audioSoundBufferID, e_AudioRenderer_SourceParameter_Loop,
                               loop ? 1 : 0);

  return true;
}

bool AudioRendererMessage_SetSourcePosition::Execute(void* caller) {
  AudioRenderer* renderer = static_cast<AudioRenderer*>(caller);
  renderer->SetSourcePosition(audioSoundBufferID, position);
  return true;
}

bool AudioRendererMessage_SetListenerParameters::Execute(void* caller) {
  AudioRenderer* renderer = static_cast<AudioRenderer*>(caller);
  renderer->SetListenerParameters(position, velocity, orientation);
  return true;
}

}  // namespace blunted
