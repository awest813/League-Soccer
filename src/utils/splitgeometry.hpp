#ifndef _HPP_UTILS_SPLITGEOMETRY
#define _HPP_UTILS_SPLITGEOMETRY

#include "defines.hpp"
#include "scene/objects/geometry.hpp"
#include "scene/scene3d/node.hpp"
#include "scene/scene3d/scene3d.hpp"

namespace blunted {

boost::intrusive_ptr<Node> SplitGeometry(std::shared_ptr<Scene3D> scene3D,
                                         boost::intrusive_ptr<Geometry> source,
                                         float gridSize = 1.0);

}

#endif
