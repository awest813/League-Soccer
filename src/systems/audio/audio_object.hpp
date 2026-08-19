#ifndef _HPP_SYSTEMS_AUDIO_OBJECT
#define _HPP_SYSTEMS_AUDIO_OBJECT

#include "defines.hpp"
#include "systems/isystemobject.hpp"

namespace blunted {

class AudioScene;

class AudioObject : public ISystemObject {
public:
  AudioObject(AudioScene* audioScene);
  virtual ~AudioObject();

  virtual boost::intrusive_ptr<Interpreter> GetInterpreter(e_ObjectType objectType) = 0;

  AudioScene* GetAudioScene();

protected:
  AudioScene* audioScene;
};

}  // namespace blunted

#endif
