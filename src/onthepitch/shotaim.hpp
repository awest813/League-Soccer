#ifndef _HPP_FOOTBALL_ONTHEPITCH_SHOTAIM
#define _HPP_FOOTBALL_ONTHEPITCH_SHOTAIM

#include <algorithm>
#include <cmath>

#include "base/math/vector3.hpp"

// Preserve the animation's commitment while allowing a small last-moment aim
// correction. GetAngle2D handles wraparound at +/- pi.
inline blunted::Vector3 RefineShotDirection(const blunted::Vector3& committed,
                                            const blunted::Vector3& requested, float maxDeviation) {
  // No usable aim input: keep the committed direction rather than a zero vector.
  if (requested.Get2D().GetLength() < 1e-4f)
    return committed;
  const float angle = requested.Get2D().GetAngle2D(committed.Get2D());
  const float limit = std::max(0.0f, maxDeviation);
  if (std::fabs(angle) <= limit)
    return requested;
  return committed.GetRotated2D(std::clamp(angle, -limit, limit));
}

enum class ShotStyle { Normal, Chip, Finesse };

inline ShotStyle ResolveShotStyle(bool chip, bool finesse) {
  return chip ? ShotStyle::Chip : (finesse ? ShotStyle::Finesse : ShotStyle::Normal);
}

#endif
