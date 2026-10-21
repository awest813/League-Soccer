#include <filesystem>
#include <fstream>
#include <vector>

#include <gtest/gtest.h>

#include "loaders/wavloader.hpp"

namespace {
void Append32(std::vector<unsigned char>& bytes, unsigned int value) {
  for (int shift = 0; shift < 32; shift += 8)
    bytes.push_back((value >> shift) & 255);
}

void VerifyLoad(const std::string& name, size_t sampleBytes, bool metadata) {
  std::vector<unsigned char> bytes = {'R', 'I', 'F', 'F'};
  Append32(bytes, static_cast<unsigned int>(36 + sampleBytes + (metadata ? 24 : 0)));
  const unsigned char header[] = {'W', 'A', 'V',  'E',  'f', 'm', 't',  ' ',  16, 0, 0, 0, 1,  0,
                                  1,   0,   0x44, 0xac, 0,   0,   0x88, 0x58, 1,  0, 2, 0, 16, 0};
  bytes.insert(bytes.end(), std::begin(header), std::end(header));
  if (metadata) {
    const unsigned char junk[] = {'J', 'U', 'N', 'K', 3, 0, 0, 0, 1, 2, 3, 0};
    bytes.insert(bytes.end(), std::begin(junk), std::end(junk));
  }
  for (char c : std::string("data"))
    bytes.push_back(c);
  Append32(bytes, static_cast<unsigned int>(sampleBytes));
  bytes.insert(bytes.end(), sampleBytes, 0);
  bytes[bytes.size() - 2] = 42;
  if (metadata) {
    const unsigned char tail[] = {'J', 'U', 'N', 'K', 4, 0, 0, 0, 9, 8, 7, 6};
    bytes.insert(bytes.end(), std::begin(tail), std::end(tail));
  }
  const auto path = std::filesystem::temp_directory_path() / name;
  struct Cleanup {
    std::filesystem::path path;
    ~Cleanup() {
      std::error_code error;
      std::filesystem::remove(path, error);
    }
  } cleanup{path};
  std::ofstream file(path, std::ios::binary);
  file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  file.close();
  blunted::WAVLoader loader;
  boost::intrusive_ptr<blunted::Resource<blunted::SoundBuffer>> resource(
      new blunted::Resource<blunted::SoundBuffer>(name));
  loader.Load(path.string(), resource);
  const auto* data = resource->GetResource()->GetData();
  ASSERT_NE(data, nullptr);
  EXPECT_EQ(data->size, sampleBytes);
  EXPECT_EQ(data->channels, 1);
  EXPECT_EQ(data->bits, 16);
  EXPECT_EQ(data->frequency, 44100U);
  EXPECT_EQ(data->data[data->size - 2], 42);
}

TEST(WavLoader, SkipsPaddedMetadataAndStopsAtDataBoundary) {
  VerifyLoad("league-soccer-wav-metadata-test.wav", 200, true);
}

TEST(WavLoader, DoesNotTruncateSamplesLargerThanFourMegabytes) {
  VerifyLoad("league-soccer-wav-large-test.wav", 4 * 1024 * 1024 + 200, false);
}
}  // namespace
