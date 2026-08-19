#ifndef _HPP_SYSTEM_GRAPHICS_RESOURCE_TEXTURE
#define _HPP_SYSTEM_GRAPHICS_RESOURCE_TEXTURE

#include "base/sdl_surface.hpp"
#include "defines.hpp"
#include "systems/graphics/resources/textureformats.hpp"

namespace blunted {

class Renderer3D;

class Texture {
public:
  Texture();
  virtual ~Texture();

  void SetRenderer3D(Renderer3D* renderer3D);

  void DeleteTexture();
  int CreateTexture(e_InternalPixelFormat internalPixelFormat, e_PixelFormat pixelFormat, int width,
                    int height, bool alpha, bool repeat, bool mipmaps, bool filter,
                    bool compareDepth = false);
  void ResizeTexture(SDL_Surface* image, e_InternalPixelFormat internalPixelFormat,
                     e_PixelFormat pixelFormat, bool alpha, bool mipmaps);
  void UpdateTexture(SDL_Surface* image, bool alpha, bool mipmaps);

  void SetID(int value);
  int GetID();

  void GetSize(int& width, int& height) const {
    width = this->width;
    height = this->height;
  }

protected:
  int textureID;
  Renderer3D* renderer3D;
  int width, height;
};

}  // namespace blunted

#endif
