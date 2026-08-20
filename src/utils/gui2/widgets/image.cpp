// Copyright 2019 Google LLC & Bastiaan Konings
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//     http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include "image.hpp"

#include <cmath>

#include "../windowmanager.hpp"
#include "base/log.hpp"
#include "managers/resourcemanagerpool.hpp"
#include "scene/objectfactory.hpp"
#include "SDL2/SDL2_rotozoom.h"

namespace blunted {

namespace {

bool HasRenderableSurface(const boost::intrusive_ptr<Image2D>& imageSource, SDL_Surface*& surface) {
  if (!imageSource) {
    return false;
  }

  boost::intrusive_ptr<Resource<Surface>> surfaceRes = imageSource->GetImage();
  if (!surfaceRes) {
    return false;
  }

  surface = surfaceRes->GetResource()->GetData();
  return surface != nullptr && surface->w > 0 && surface->h > 0;
}

}  // namespace

Gui2Image::Gui2Image(Gui2WindowManager* windowManager, const std::string& name, float x_percent,
                     float y_percent, float width_percent, float height_percent)
    : Gui2View(windowManager, name, x_percent, y_percent, width_percent, height_percent) {
  int x, y, w, h;
  windowManager->GetCoordinates(x_percent, y_percent, width_percent, height_percent, x, y, w, h);
  image = windowManager->CreateImage2D(name, w, h, true);
}

Gui2Image::~Gui2Image() {}

void Gui2Image::LoadImage(const std::string& filename) {
  boost::intrusive_ptr<Resource<Surface>> surfaceRes =
      ResourceManagerPool::GetInstance()
          .GetManager<Surface>(e_ResourceType_Surface)
          ->Fetch(filename, true, true);

  if (!surfaceRes || !surfaceRes->GetResource() || !surfaceRes->GetResource()->GetData()) {
    Log(e_Warning, "Gui2Image", "LoadImage", "Failed to load \"" + filename + "\" via resource manager");
    return;
  }

  imageSource = boost::static_pointer_cast<Image2D>(
      ObjectFactory::GetInstance().CreateObject(name + "source", e_ObjectType_Image2D));
  imageSource->SetImage(surfaceRes);

  Redraw();
}

void Gui2Image::Redraw() {
  // paste source image onto screen image
  if (!imageSource) {
    return;
  }

  boost::intrusive_ptr<Resource<Surface>> sourceSurfaceRes = imageSource->GetImage();
  if (!sourceSurfaceRes) {
    return;
  }
  sourceSurfaceRes->resourceMutex.lock();

  SDL_Surface* imageSurfTmp = nullptr;
  const bool hasRenderableSurface = HasRenderableSurface(imageSource, imageSurfTmp);

  int x, y, w, h;
  windowManager->GetCoordinates(x_percent, y_percent, width_percent, height_percent, x, y, w, h);

  if (!hasRenderableSurface || w <= 0 || h <= 0) {
    sourceSurfaceRes->resourceMutex.unlock();
    return;
  }

  const double zoomx = static_cast<double>(w) / imageSurfTmp->w;
  const double zoomy = static_cast<double>(h) / imageSurfTmp->h;
  SDL_Surface* imageSurf = zoomSurface(imageSurfTmp, zoomx, zoomy, SMOOTHING_ON);
  sourceSurfaceRes->resourceMutex.unlock();

  if (!imageSurf) {
    Log(e_Warning, "Gui2Image", "Redraw", "Failed to scale source image");
    return;
  }

  boost::intrusive_ptr<Resource<Surface>> targetSurfaceRes = image->GetImage();
  targetSurfaceRes->resourceMutex.lock();
  targetSurfaceRes->GetResource()->SetData(imageSurf);
  targetSurfaceRes->resourceMutex.unlock();

  image->OnChange();
}

void Gui2Image::GetImages(std::vector<boost::intrusive_ptr<Image2D>>& target) {
  target.push_back(image);
  Gui2View::GetImages(target);
}

void Gui2Image::SetSize(float new_width_percent, float new_height_percent) {
  Gui2View::SetSize(new_width_percent, new_height_percent);

  int x, y, w, h;
  windowManager->GetCoordinates(x_percent, y_percent, width_percent, height_percent, x, y, w, h);
  // printf("resized to %i %i\n", w, h);

  if (w <= 0 || h <= 0) {
    Log(e_Warning, "Gui2Image", "SetSize", "Ignoring resize to a non-positive image size");
    return;
  }

  image->Resize(w, h);
  Redraw();
}

void Gui2Image::SetZoom(float zoomx, float zoomy) {
  // paste source image onto screen image
  if (!imageSource) {
    return;
  }

  boost::intrusive_ptr<Resource<Surface>> sourceSurfaceRes = imageSource->GetImage();
  if (!sourceSurfaceRes) {
    return;
  }
  sourceSurfaceRes->resourceMutex.lock();

  SDL_Surface* imageSurfTmp = nullptr;
  const bool hasRenderableSurface = HasRenderableSurface(imageSource, imageSurfTmp);

  int x, y, w, h;
  windowManager->GetCoordinates(x_percent, y_percent, width_percent, height_percent, x, y, w, h);

  if (!hasRenderableSurface || w <= 0 || h <= 0) {
    sourceSurfaceRes->resourceMutex.unlock();
    return;
  }

  const double zoomx1 = static_cast<double>(w) / imageSurfTmp->w * zoomx;
  const double zoomy1 = static_cast<double>(h) / imageSurfTmp->h * zoomy;
  SDL_Surface* imageSurf = zoomSurface(imageSurfTmp, zoomx1, zoomy1, SMOOTHING_ON);
  sourceSurfaceRes->resourceMutex.unlock();

  if (!imageSurf) {
    Log(e_Warning, "Gui2Image", "SetZoom", "Failed to scale source image");
    return;
  }

  image->DrawRectangle(0, 0, w, h, Vector3(0, 0, 0), 0);

  boost::intrusive_ptr<Resource<Surface>> targetSurfaceRes = image->GetImage();
  targetSurfaceRes->resourceMutex.lock();

  SDL_Surface* surface = targetSurfaceRes->GetResource()->GetData();
  SDL_Rect rect;
  rect.x = w * 0.5 - imageSurf->w * 0.5;
  rect.y = h * 0.5 - imageSurf->h * 0.5;
  SDL_BlitSurface(imageSurf, nullptr, surface, &rect);

  targetSurfaceRes->resourceMutex.unlock();

  SDL_FreeSurface(imageSurf);

  image->OnChange();
}

}  // namespace blunted
