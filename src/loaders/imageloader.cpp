#include "imageloader.hpp"

#include <fstream>

#include "base/log.hpp"

namespace blunted {

ImageLoader::ImageLoader() : Loader<Surface>() {}

ImageLoader::~ImageLoader() {}

// load file into resource
void ImageLoader::Load(const std::string& filename,
                       boost::intrusive_ptr<Resource<Surface>> resource) {
  SDL_Surface* surface = IMG_Load(filename.c_str());
  if (!surface)
    Log(e_FatalError, "ImageLoader", "Load", "Could not load " + filename + ": " + IMG_GetError());
  resource->GetResource()->SetData(surface);
}

}  // namespace blunted
