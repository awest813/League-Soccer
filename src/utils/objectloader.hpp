#ifndef _HPP_UTILS_OBJECTLOADER
#define _HPP_UTILS_OBJECTLOADER

#include "scene/scene3d/node.hpp"
#include "scene/scene3d/scene3d.hpp"
#include "utils/xmlloader.hpp"

namespace blunted {

class ObjectLoader {
public:
  ObjectLoader();
  ~ObjectLoader();

  // depricated: LoadObject now recurses, this would take away the need for a level->objects
  // hierarchy
  boost::intrusive_ptr<Node> LoadLevel(std::shared_ptr<Scene3D> scene3D,
                                       const std::string& filename) const;

  boost::intrusive_ptr<Node> LoadObject(std::shared_ptr<Scene3D> scene3D,
                                        const std::string& filename,
                                        const Vector3& offset = Vector3(0)) const;

protected:
  boost::intrusive_ptr<Node> LoadObjectImpl(std::shared_ptr<Scene3D> scene3D,
                                            const std::string& nodename, const XMLTree& objectTree,
                                            const Vector3& offset) const;

  void InterpretProperties(const map_XMLTree& tree, Properties& properties) const;
  e_LocalMode InterpretLocalMode(const std::string& value) const;
};

}  // namespace blunted

#endif
