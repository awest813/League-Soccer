// Copyright 2019 Google LLC & Bastiaan Konings
// Licensed under the Apache License, Version 2.0 (the "License");

#ifndef _HPP_SYSTEM_GRAPHICS_RESOURCE_TEXTUREFORMATS
#define _HPP_SYSTEM_GRAPHICS_RESOURCE_TEXTUREFORMATS

namespace blunted {

enum e_PixelFormat {
  e_PixelFormat_Alpha,
  e_PixelFormat_RGB,
  e_PixelFormat_RGBA,
  e_PixelFormat_DepthComponent,
  e_PixelFormat_Luminance
};

enum e_InternalPixelFormat {
  e_InternalPixelFormat_RGB8,
  e_InternalPixelFormat_SRGB8,
  e_InternalPixelFormat_RGB16,
  e_InternalPixelFormat_RGBA8,
  e_InternalPixelFormat_SRGBA8,
  e_InternalPixelFormat_RGBA16,
  e_InternalPixelFormat_RGBA16F,
  e_InternalPixelFormat_RGBA32F,
  e_InternalPixelFormat_RGBA4,
  e_InternalPixelFormat_RGB5_A1,
  e_InternalPixelFormat_DepthComponent,
  e_InternalPixelFormat_DepthComponent16,
  e_InternalPixelFormat_DepthComponent24,
  e_InternalPixelFormat_DepthComponent32,
  e_InternalPixelFormat_DepthComponent32F,
  e_InternalPixelFormat_StencilIndex8
};

}  // namespace blunted

#endif  // _HPP_SYSTEM_GRAPHICS_RESOURCE_TEXTUREFORMATS
