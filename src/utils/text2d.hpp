/* DEPRECATED CLASS */

#ifndef _HPP_UTILS_TEXT2D
#define _HPP_UTILS_TEXT2D
// VK: TODO: Remove this deprecated class from compilation and from the project
#include <SDL2/SDL_ttf.h>

#include "SDL2/SDL.h"
#include "defines.hpp"
#include "scene/objects/image2d.hpp"
#include "scene/scene2d/scene2d.hpp"

namespace blunted {

class Text2D {
public:
  Text2D(std::shared_ptr<Scene2D> scene2D,
         const std::string& fontFile = "c:/windows/fonts/cour.ttf", int pts = 14);
  ~Text2D();

  boost::intrusive_ptr<Image2D> Create(int width);
  void SetText(boost::intrusive_ptr<Image2D> image, const std::string& text,
               const Vector3& color) const;

protected:
  SDL_Surface* RenderTextSurface(const std::string& text, const Vector3& color) const;

  TTF_Font* font;
  mutable int count;

  std::shared_ptr<Scene2D> scene2D;
};

}  // namespace blunted

#endif
