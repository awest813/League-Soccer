#ifndef _HPP_TRIANGLEMESHUTILS
#define _HPP_TRIANGLEMESHUTILS

#include "defines.hpp"

namespace blunted {

class AABB;

AABB GetTriangleMeshAABB(float* vertices, int verticesDataSize,
                         const std::vector<unsigned int>& indices);
int GetTriangleMeshElementCount();

}  // namespace blunted

#endif
