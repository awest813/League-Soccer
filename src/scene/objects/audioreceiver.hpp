#ifndef _HPP_OBJECT_AUDIORECEIVER
#define _HPP_OBJECT_AUDIORECEIVER

#include "base/math/quaternion.hpp"
#include "base/math/vector3.hpp"
#include "defines.hpp"
#include "scene/object.hpp"
#include "types/interpreter.hpp"

namespace blunted {

class AudioReceiver : public Object {
public:
  AudioReceiver(const std::string& name);
  virtual ~AudioReceiver();

  virtual void Init();
  virtual void Exit();

  virtual void RecursiveUpdateSpatialData(e_SpatialDataType spatialDataType,
                                          e_SystemType excludeSystem = e_SystemType_None);

protected:
};

class IAudioReceiverInterpreter : public Interpreter {
public:
  virtual void OnLoad() = 0;
  virtual void OnUnload() = 0;
  virtual void OnSpatialChange(const Vector3& position, const Quaternion& rotation) = 0;

protected:
};

}  // namespace blunted

#endif
