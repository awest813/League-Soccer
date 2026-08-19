#ifndef _HPP_OBJECT_SKYBOX
#define _HPP_OBJECT_SKYBOX

#include "geometry.hpp"

namespace blunted {

class Skybox : public Geometry {
public:
  Skybox(const std::string& name);
  virtual ~Skybox();

protected:
};

class ISkyboxInterpreter : public IGeometryInterpreter {
public:
  virtual void OnLoad(boost::intrusive_ptr<Skybox> geom) = 0;
  virtual void OnUnload() = 0;
  virtual void OnMove(const Vector3& position) = 0;
  virtual void OnRotate(const Quaternion& rotation) = 0;

  virtual void OnPoke() = 0;

protected:
};

}  // namespace blunted

#endif
