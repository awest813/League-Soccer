#ifndef _HPP_PROCEDURALPITCH
#define _HPP_PROCEDURALPITCH

#include "base/sdl_surface.hpp"
#include "defines.hpp"

using namespace blunted;

Uint32 GetPitchDiffuseColor(SDL_Surface* pitchSurf, float xCoord, float yCoord);
Uint32 GetPitchSpecularColor(SDL_Surface* pitchSurf, float xCoord, float yCoord);
Uint32 GetPitchNormalColor(SDL_Surface* pitchSurf, float xCoord, float yCoord);
void DrawLines(SDL_PixelFormat* pixelFormat, Uint32* diffuseBitmap, int resX, int resY,
               signed int offsetW, signed int offsetH);
void GeneratePitch(int resX, int resY, int resSpecularX, int resSpecularY, int resNormalX,
                   int resNormalY);

#endif
