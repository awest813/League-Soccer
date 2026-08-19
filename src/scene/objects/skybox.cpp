#include "skybox.hpp"

namespace blunted {

Skybox::Skybox(const std::string& name) : Geometry(name, e_ObjectType_Skybox) {
  localMode = e_LocalMode_Absolute;
}

Skybox::~Skybox() {}

}  // namespace blunted
