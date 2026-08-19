#ifndef _HPP_LOADERS_IMAGE
#define _HPP_LOADERS_IMAGE

#include "defines.hpp"
#include "managers/resourcemanager.hpp"
#include "scene/objects/image2d.hpp"
#include "scene/resources/surface.hpp"

namespace blunted {

class ImageLoader : public Loader<Surface> {
public:
  ImageLoader();
  virtual ~ImageLoader();

  virtual void Load(const std::string& filename, boost::intrusive_ptr<Resource<Surface>> resource);

protected:
};

}  // namespace blunted

#endif
