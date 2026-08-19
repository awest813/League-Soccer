#include "audio_object.hpp"

namespace blunted {

AudioObject::AudioObject(AudioScene* audioScene) : audioScene(audioScene) {}

AudioObject::~AudioObject() {}

AudioScene* AudioObject::GetAudioScene() {
  return audioScene;
}

}  // namespace blunted
