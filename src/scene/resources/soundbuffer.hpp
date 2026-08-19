#ifndef _HPP_RESOURCES_SOUNDBUFFER
#define _HPP_RESOURCES_SOUNDBUFFER

#include "types/resource.hpp"

namespace blunted {

struct WavData {
  WavData() {
    data = 0;
    size = 0;
  }
  ~WavData() {
    delete[] data;
    size = 0;
  }
  WavData(const WavData& src) {
    data = new unsigned char[src.size];
    memcpy(data, src.data, src.size * sizeof(unsigned char));
    size = src.size;
    channels = src.channels;
    bits = src.bits;
    frequency = src.frequency;
  }
  unsigned char* data;
  int size;
  int channels, bits;
  unsigned int frequency;
};

class SoundBuffer {
public:
  SoundBuffer();
  virtual ~SoundBuffer();
  SoundBuffer(const SoundBuffer& src);

  const WavData* GetData() const;
  void SetData(WavData* data);

protected:
  WavData* data;
};

}  // namespace blunted

#endif
