#include "soundbuffer.hpp"

namespace blunted {

SoundBuffer::SoundBuffer() : data(0) {}

SoundBuffer::~SoundBuffer() {
  if (data) {
    delete data;
  }
  data = 0;
}

SoundBuffer::SoundBuffer(const SoundBuffer& src) {
  this->data = new WavData(*src.GetData());
}

const WavData* SoundBuffer::GetData() const {
  return data;
}

void SoundBuffer::SetData(WavData* data) {
  if (this->data)
    delete this->data;
  this->data = data;
}

}  // namespace blunted
