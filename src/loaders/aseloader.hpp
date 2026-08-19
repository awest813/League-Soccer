#ifndef _HPP_LOADERS_ASE
#define _HPP_LOADERS_ASE

#include "base/utils.hpp"
#include "defines.hpp"
#include "managers/resourcemanager.hpp"
#include "scene/objects/geometry.hpp"
#include "scene/resources/geometrydata.hpp"

namespace blunted {

struct s_Material {
  std::string maps[4];
  std::string shininess;
  std::string specular_amount;
  Vector3 self_illumination;
};

class ASELoader : public Loader<GeometryData> {
public:
  ASELoader();
  virtual ~ASELoader();

  // ----- encapsulating load function
  virtual void Load(const std::string& filename, boost::intrusive_ptr<Resource<GeometryData>> resource);

  // ----- interpreter for the .ase treedata
  void Build(const s_tree* data, boost::intrusive_ptr<Resource<GeometryData>> resource);

  // ----- per-object interpreters
  void BuildTriangleMesh(const s_tree* data, boost::intrusive_ptr<Resource<GeometryData>> resource,
                         const std::vector<s_Material>& materialList);

protected:
  int triangleCount;
};

}  // namespace blunted

#endif
