// written by bastiaan konings schuiling 2008 - 2014
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not
// be used for anything important. i do not offer support, so don't ask. to be used for inspiration
// :)

#ifndef _HPP_AUDIO_NULL
#define _HPP_AUDIO_NULL

#include "interface_audiorenderer.hpp"

namespace blunted {

class NullAudioRenderer : public AudioRenderer {
public:
  NullAudioRenderer();
  virtual ~NullAudioRenderer();

  virtual bool CreateContext() override;
  virtual void Exit() override;

  virtual int CreateAudioSoundBuffer(const WavData* wavData) override;
  virtual void DeleteAudioSoundBuffer(int audioSoundBufferID) override;
  virtual void PlayAudioSoundBuffer(int audioSoundBufferID) override;

  virtual void SetListenerParameters(const Vector3& position, const Vector3& velocity,
                                     const Quaternion& orientation) override;

  virtual void SetSourceParameter(int audioSoundBufferID,
                                  e_AudioRenderer_SourceParameter parameter,
                                  float value) override;
  virtual void SetSourcePosition(int audioSoundBufferID, const Vector3& position) override;

  virtual void operator()() override;

private:
  int nextBufferId;
};

}  // namespace blunted

#endif
