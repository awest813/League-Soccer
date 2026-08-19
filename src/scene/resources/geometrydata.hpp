#ifndef _HPP_OBJECT_GEOMETRYDATA
#define _HPP_OBJECT_GEOMETRYDATA

#include "base/math/vector3.hpp"
#include "defines.hpp"
#include "scene/object.hpp"
#include "surface.hpp"
#include "types/interpreter.hpp"
#include "types/material.hpp"
#include "types/resource.hpp"

namespace blunted {

struct MaterializedTriangleMesh {
  Material material;

  float* vertices;       // was: triangleMesh
  int verticesDataSize;  // was: triangleMeshSize

  /* contents:
  float vertices[verticesDataSize * 3];
  float normals[verticesDataSize * 3];
  float texturevertices[verticesDataSize * 3];
  float tangents[verticesDataSize * 3];
  float bitangents[verticesDataSize * 3];
  */

  std::vector<unsigned int> indices;
};

class GeometryData {
public:
  GeometryData();
  virtual ~GeometryData();
  GeometryData(const GeometryData& src);

  void DeleteTriangleMeshes();
  // todo: what about resource acquisition is ownership?
  void SetTriangleMesh(Material material, float* vertices, int verticesDataSize,
                       const std::vector<unsigned int>& indices);
  void AddTriangleMesh(Material material, float* vertices, int verticesDataSize,
                       const std::vector<unsigned int>& indices);
  std::vector<MaterializedTriangleMesh> GetTriangleMeshes();
  std::vector<MaterializedTriangleMesh>& GetTriangleMeshesRef();
  void SetDynamic(bool dynamic) { isDynamic = dynamic; }
  bool IsDynamic() { return isDynamic; }

  AABB GetAABB() const;

protected:
  bool isDynamic;
  std::vector<MaterializedTriangleMesh> triangleMeshes;

  mutable AABBCache aabb;
};

}  // namespace blunted

#endif
