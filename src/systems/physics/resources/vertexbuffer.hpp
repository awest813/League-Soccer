#ifndef _HPP_SYSTEM_PHYSICS_RESOURCE_VERTEXBUFFER
#define _HPP_SYSTEM_PHYSICS_RESOURCE_VERTEXBUFFER

#include "base/geometry/triangle.hpp"
#include "defines.hpp"

namespace blunted {

class Renderer3D;

class VertexBuffer {
public:
  VertexBuffer();
  virtual ~VertexBuffer();

  int CreateVertexBuffer(Renderer3D* renderer3D, const std::vector<Triangle*>& triangles);

  void SetID(int value);
  int GetID();

  int GetVertexCount();

protected:
  int vertexBufferID;
  int vertexCount;
  Renderer3D* renderer3D;
};

}  // namespace blunted

#endif
