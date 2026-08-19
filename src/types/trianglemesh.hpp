#ifndef _HPP_TRIANGLEMESH
#define _HPP_TRIANGLEMESH

#include "base/geometry/triangle.hpp"
#include "defines.hpp"

namespace blunted {

class TriangleMesh {
public:
  TriangleMesh();
  virtual ~TriangleMesh();
  TriangleMesh(const TriangleMesh& src);

  // todo: what about acquisition is ownership?
  void AddTriangle(Triangle* triangle);
  const std::vector<Triangle*>& GetTriangles();
  Triangle* GetTriangle(int id);
  int GetTriangleCount() const;
  AABB GetAABB() const;

protected:
  mutable AABB aabb;
  mutable bool dirtyAABB;

  std::vector<Triangle*> triangles;
};

}  // namespace blunted

#endif
