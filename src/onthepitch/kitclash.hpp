// Picks the away kit so the two sides can be told apart. Pure data, kept free
// of SDL and the engine so it can be unit tested.

#ifndef _HPP_FOOTBALL_ONTHEPITCH_KITCLASH
#define _HPP_FOOTBALL_ONTHEPITCH_KITCLASH

#include <cmath>

namespace kitclash {

struct Colour {
  float r = 0, g = 0, b = 0;
};

// Perceptual distance between two colours ("redmean"); 0 to about 765.
inline float Distance(const Colour& a, const Colour& b) {
  const float rMean = (a.r + b.r) * 0.5f;
  const float dr = a.r - b.r, dg = a.g - b.g, db = a.b - b.b;
  return std::sqrt((2.0f + rMean / 256.0f) * dr * dr + 4.0f * dg * dg +
                   (2.0f + (255.0f - rMean) / 256.0f) * db * db);
}

// Shirts closer than this are hard to tell apart on the pitch.
constexpr float kClashDistance = 120.0f;

// Returns the away kit (1 or 2) to wear against the home shirt. The preferred
// kit stays unless it clashes and the other one stands out clearly better.
// A kit whose shirt colour is unknown (missing texture) is never switched to.
inline int ChooseAwayKit(const Colour& homeShirt, const Colour* awayKit1, const Colour* awayKit2,
                         int preferred) {
  const Colour* kits[2] = {awayKit1, awayKit2};
  const int other = preferred == 1 ? 2 : 1;
  const Colour* mine = kits[preferred - 1];
  const Colour* alternative = kits[other - 1];
  if (!mine || !alternative)
    return preferred;
  const float mineDistance = Distance(homeShirt, *mine);
  if (mineDistance >= kClashDistance)
    return preferred;
  return Distance(homeShirt, *alternative) > mineDistance + 30.0f ? other : preferred;
}

}  // namespace kitclash

#endif  // _HPP_FOOTBALL_ONTHEPITCH_KITCLASH
