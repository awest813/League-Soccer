// written by bastiaan konings schuiling 2008 - 2014
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not
// be used for anything important. i do not offer support, so don't ask. to be used for inspiration
// :)

#include "wavloader.hpp"

#include <fstream>
#include <vector>

#include "base/log.hpp"

namespace blunted {

WAVLoader::WAVLoader() : Loader<SoundBuffer>() {}

WAVLoader::~WAVLoader() {}

// load file into resource
void WAVLoader::Load(const std::string& filename,
                     boost::intrusive_ptr<Resource<SoundBuffer>> resource) {
  int channels, bits;
  unsigned int frequency;
  long BUFFER_SIZE = 1024 * 1024 * 4;

  FILE* f = nullptr;
  fopen_s(&f, filename.c_str(), "rb");
  if (!f) {
    Log(e_FatalError, "WAVLoader", "Load", "Could not load " + filename + ": file not found");
    return;
  }

  std::vector<unsigned char> buffer(BUFFER_SIZE);
  auto readExact = [&](size_t byteCount, const char* section) {
    if (fread(buffer.data(), 1, byteCount, f) == byteCount)
      return true;
    fclose(f);
    Log(e_FatalError, "WAVLoader", "Load",
        "Could not load " + filename + ": unexpected end of " + section);
    return false;
  };

  if (!readExact(12, "WAV header") || !readExact(8, "format header"))
    return;
  if (buffer[0] != 'f' || buffer[1] != 'm' || buffer[2] != 't' || buffer[3] != ' ') {
    fclose(f);
    Log(e_FatalError, "WAVLoader", "Load",
        "Could not load " + filename + ": format information header incorrect");
    return;
  }

  if (!readExact(2, "PCM format"))
    return;

  if (buffer[0] != 1 || buffer[1] != 0) {
    fclose(f);
    Log(e_FatalError, "WAVLoader", "Load", "Could not load " + filename + ": not PCM");
    return;
  }

  if (!readExact(2, "channel count"))
    return;
  channels = buffer[1] << 8;
  channels |= buffer[0];

  if (!readExact(4, "sample frequency"))
    return;
  frequency = buffer[3] << 24;
  frequency |= buffer[2] << 16;
  frequency |= buffer[1] << 8;
  frequency |= buffer[0];

  if (!readExact(6, "block and byte rates"))
    return;

  if (!readExact(2, "sample bit depth"))
    return;
  bits = buffer[1] << 8;
  bits |= buffer[0];

  if (!readExact(8, "data header"))
    return;
  if (buffer[0] != 'd' || buffer[1] != 'a' || buffer[2] != 't' || buffer[3] != 'a') {
    fclose(f);
    Log(e_FatalError, "WAVLoader", "Load", "Could not load " + filename + ": data chunk not found");
    return;
  }

  int size = static_cast<int>(fread(buffer.data(), 1, BUFFER_SIZE, f));

  WavData* data = new WavData();
  data->data = new unsigned char[size];
  memcpy(data->data, buffer.data(), size * sizeof(unsigned char));
  data->size = size;
  data->channels = channels;
  data->bits = bits;
  data->frequency = frequency;

  // printf("wav data: %i bytes, %i channels, %i bits, %u freq\n", size * sizeof(unsigned char),
  // channels, bits, frequency);

  resource->GetResource()->SetData(data);

  fclose(f);
}

}  // namespace blunted
