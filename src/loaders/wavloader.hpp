#ifndef _HPP_LOADERS_WAVE
#define _HPP_LOADERS_WAVE

#include "defines.hpp"
#include "managers/resourcemanager.hpp"
#include "scene/objects/sound.hpp"
#include "scene/resources/soundbuffer.hpp"

namespace blunted {

class WAVLoader : public Loader<SoundBuffer> {
public:
  WAVLoader();
  virtual ~WAVLoader();

  virtual void Load(const std::string& filename, boost::intrusive_ptr<Resource<SoundBuffer>> resource);

protected:
};

}  // namespace blunted

#endif
