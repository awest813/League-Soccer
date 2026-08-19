#include "audioreceiver.hpp"

#include "systems/isystemobject.hpp"

namespace blunted {

AudioReceiver::AudioReceiver(const std::string& name)
    : Object(name, e_ObjectType_AudioReceiver) {}

AudioReceiver::~AudioReceiver() {}

void AudioReceiver::Init() {  // ATOMIC
  subjectMutex.lock();

  int observersSize = observers.size();
  for (int i = 0; i < observersSize; i++) {
    IAudioReceiverInterpreter* AudioReceiverInterpreter =
        static_cast<IAudioReceiverInterpreter*>(observers.at(i).get());
    AudioReceiverInterpreter->OnLoad();
  }

  subjectMutex.unlock();
}

void AudioReceiver::Exit() {  // ATOMIC
  subjectMutex.lock();

  int observersSize = observers.size();
  for (int i = 0; i < observersSize; i++) {
    IAudioReceiverInterpreter* AudioReceiverInterpreter =
        static_cast<IAudioReceiverInterpreter*>(observers.at(i).get());
    AudioReceiverInterpreter->OnUnload();
  }

  Object::Exit();

  subjectMutex.unlock();
}

void AudioReceiver::RecursiveUpdateSpatialData(e_SpatialDataType spatialDataType,
                                               e_SystemType excludeSystem) {
  InvalidateSpatialData();

  subjectMutex.lock();

  int observersSize = observers.size();
  for (int i = 0; i < observersSize; i++) {
    if (observers.at(i)->GetSystemType() != excludeSystem) {
      IAudioReceiverInterpreter* audioReceiverInterpreter =
          static_cast<IAudioReceiverInterpreter*>(observers.at(i).get());
      audioReceiverInterpreter->OnSpatialChange(GetDerivedPosition(), GetDerivedRotation());
    }
  }

  subjectMutex.unlock();
}

}  // namespace blunted
