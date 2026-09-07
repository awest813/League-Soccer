#include "caption.hpp"

#include <algorithm>

#include <cmath>

#include "../windowmanager.hpp"
#include "SDL2/SDL2_rotozoom.h"

namespace blunted {

Gui2Caption::Gui2Caption(Gui2WindowManager* windowManager, const std::string& name, float x_percent,
                         float y_percent, float width_percent, float height_percent,
                         const std::string& caption)
    : Gui2View(windowManager, name, x_percent, y_percent, width_percent, height_percent) {
  renderedTextHeightPix = 0;
  renderedTextScale = 1.0f;
  autoWidth = width_percent <= 1.0f;
  transparency = 0.0f;

  int x, y, w, h;
  windowManager->GetCoordinates(x_percent, y_percent, width_percent, height_percent, x, y, w, h);
  image = windowManager->CreateImage2D(name, w, h, true);

  color = windowManager->GetStyle()->GetColor(e_DecorationType_Bright1);
  outlineColor = windowManager->GetStyle()->GetColor(e_DecorationType_Dark1);

  SetCaption(caption);
}

Gui2Caption::~Gui2Caption() {}

void Gui2Caption::GetImages(std::vector<boost::intrusive_ptr<Image2D>>& target) {
  target.push_back(image);
  Gui2View::GetImages(target);
}

void Gui2Caption::SetColor(const Vector3& color) {
  if (color != this->color) {
    this->color = color;
    Redraw();
  }
}

void Gui2Caption::SetOutlineColor(const Vector3& outlineColor) {
  if (outlineColor != this->outlineColor) {
    this->outlineColor = outlineColor;
    Redraw();
  }
}

void Gui2Caption::SetTransparency(float transparency) {
  if (transparency != this->transparency) {
    this->transparency = transparency;
    Redraw();
  }
}

void Gui2Caption::Redraw() {
  int x, y, w, h;
  windowManager->GetCoordinates(x_percent, y_percent, width_percent, height_percent, x, y, w, h);
  int x_margin = 0;
  int y_margin = 0;
  int outlineWidth =
      TTF_GetFontOutline(windowManager->GetStyle()->GetFont(e_TextType_DefaultOutline));

  Vector3 textColor = color;
  SDL_Color textColorSDL = {Uint8(textColor.coords[0]), Uint8(textColor.coords[1]),
                            Uint8(textColor.coords[2]), 255};
  Vector3 textOutlineColor = outlineColor;
  SDL_Color textOutlineColorSDL = {Uint8(textOutlineColor.coords[0]),
                                   Uint8(textOutlineColor.coords[1]),
                                   Uint8(textOutlineColor.coords[2]), 255};

  SDL_Surface* textSurfTmp = TTF_RenderUTF8_Blended(
      windowManager->GetStyle()->GetFont(e_TextType_Caption), caption.c_str(), textColorSDL);
  SDL_Surface* textOutlineSurfTmp =
      TTF_RenderUTF8_Blended(windowManager->GetStyle()->GetFont(e_TextType_DefaultOutline),
                             caption.c_str(), textOutlineColorSDL);

  if (!textSurfTmp || !textOutlineSurfTmp) {
    SDL_FreeSurface(textSurfTmp);
    SDL_FreeSurface(textOutlineSurfTmp);
    textWidth_percent = 0.0f;
    image->DrawRectangle(0, 0, w, h, Vector3(0, 0, 0), 0);
    image->OnChange();
    return;
  }

  int resW, resH;
  TTF_SizeUTF8(windowManager->GetStyle()->GetFont(e_TextType_DefaultOutline), caption.c_str(),
               &resW, &resH);

  float zoomy;
  renderedTextHeightPix = (float)textOutlineSurfTmp->h;
  zoomy = (float)std::max(1, h - y_margin * 2) / renderedTextHeightPix;
  // Keep labels inside their allocated column, including long translations.
  // Legacy zero/one-width captions request natural-width text (e.g. player overlays).
  if (!autoWidth)
    zoomy = std::min(zoomy, (float)std::max(1, w) / textOutlineSurfTmp->w);
  renderedTextScale = zoomy;
  SDL_Surface* textOutlineSurf = zoomSurface(textOutlineSurfTmp, zoomy, zoomy, 1);
  SDL_Surface* textSurf = zoomSurface(textSurfTmp, zoomy, zoomy, 1);
  SDL_FreeSurface(textOutlineSurfTmp);
  SDL_FreeSurface(textSurfTmp);

  textWidth_percent = windowManager->GetWidthPercent(resW * zoomy);

  image->DrawRectangle(0, 0, w, h, Vector3(0, 0, 0), 0);

  const int targetW = autoWidth ? std::max(1, (int)std::ceil(resW * zoomy)) : std::max(1, w);
  if (targetW != int(image->GetSize().coords[0]) || h != int(image->GetSize().coords[1])) {
    image->Resize(targetW, std::max(1, h));
    if (autoWidth)
      width_percent = windowManager->GetWidthPercent(targetW);
  }

  boost::intrusive_ptr<Resource<Surface>> surfaceRes = image->GetImage();
  surfaceRes->resourceMutex.lock();
  SDL_Surface* surface = surfaceRes->GetResource()->GetData();

  Uint32 color32 = SDL_MapRGBA(surface->format, 0, 0, 0, 0);
  sdl_rectangle_filled(surface, 0, 0, surface->w, surface->h, color32);
  // printf("%i %i - %i %i - %i %i\n", surface->w, surface->h, textOutlineSurf->w,
  // textOutlineSurf->h, textSurf->w, textSurf->h);

  SDL_Rect dstRect;
  dstRect.x = 0;
  dstRect.y = std::max(0, (h - textOutlineSurf->h) / 2);
  dstRect.w = 10000;
  dstRect.h = 10000;
  SDL_BlitSurface(textOutlineSurf, nullptr, surface, &dstRect);
  dstRect.x = round(outlineWidth * zoomy);
  dstRect.y = std::max(0, (h - textOutlineSurf->h) / 2) + round(outlineWidth * zoomy);
  SDL_BlitSurface(textSurf, nullptr, surface, &dstRect);
  if (transparency > 0.0f) {
    sdl_setsurfacealpha(surface, (1.0f - transparency) * 255);
  }
  surfaceRes->resourceMutex.unlock();

  SDL_FreeSurface(textOutlineSurf);
  SDL_FreeSurface(textSurf);

  image->OnChange();
}

void Gui2Caption::SetCaption(const std::string& newCaption) {
  std::string adaptedCaption = newCaption;
  if (adaptedCaption.empty())
    adaptedCaption = " ";
  if (caption != adaptedCaption) {
    caption = adaptedCaption;
    // Preserve authored capitalization and UTF-8 bytes in localized labels.
    Redraw();
  }
}

float Gui2Caption::GetTextWidthPercent(int subStrLength) {
  int x, y, w, h;
  windowManager->GetCoordinates(x_percent, y_percent, width_percent, height_percent, x, y, w, h);

  int resW, resH;
  TTF_SizeUTF8(windowManager->GetStyle()->GetFont(e_TextType_DefaultOutline),
               caption.substr(0, subStrLength).c_str(), &resW, &resH);

  float zoomy;
  zoomy = renderedTextScale;

  return windowManager->GetWidthPercent(resW * zoomy);
}

}  // namespace blunted
