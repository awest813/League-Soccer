#include "trianglemeshutils.hpp"

#include "aabb.hpp"

namespace blunted {

const int __triangleMeshElementCount = 5;

AABB GetTriangleMeshAABB(float* vertices, int verticesDataSize,
                         const std::vector<unsigned int>& indices) {
  AABB aabb;

  aabb.Reset();

  if (indices.size() == 0) {
    for (unsigned int t = 0;
         t < verticesDataSize / (unsigned int)GetTriangleMeshElementCount() / 3 / 3; t++) {
      for (unsigned int v = 0; v < 3; v++) {
        for (unsigned int i = 0; i < 3; i++) {
          if (vertices[t * 9 + v * 3 + i] < aabb.minxyz.coords[i])
            aabb.minxyz.coords[i] = vertices[t * 9 + v * 3 + i];
          if (vertices[t * 9 + v * 3 + i] > aabb.maxxyz.coords[i])
            aabb.maxxyz.coords[i] = vertices[t * 9 + v * 3 + i];
        }
      }
    }

  } else {
    for (unsigned int t = 0; t < indices.size() / 3; t++) {
      for (unsigned int v = 0; v < 3; v++) {
        for (unsigned int i = 0; i < 3; i++) {
          const unsigned int vertexOffset = indices[t * 3 + v] + i;
          assert(verticesDataSize >= 0 &&
                 static_cast<unsigned int>(verticesDataSize) > vertexOffset);
          if (vertices[vertexOffset] < aabb.minxyz.coords[i])
            aabb.minxyz.coords[i] = vertices[vertexOffset];
          if (vertices[vertexOffset] > aabb.maxxyz.coords[i])
            aabb.maxxyz.coords[i] = vertices[vertexOffset];
        }
      }
    }
  }

  aabb.MakeDirty();
  return aabb;
}

int GetTriangleMeshElementCount() {
  return __triangleMeshElementCount;
}

}  // namespace blunted
