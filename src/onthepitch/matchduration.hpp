// Match-duration configuration helpers shared by the menu, live match, and tests.

#ifndef _HPP_FOOTBALL_ONTHEPITCH_MATCHDURATION
#define _HPP_FOOTBALL_ONTHEPITCH_MATCHDURATION

#include <algorithm>

inline constexpr float kMinimumMatchDurationMinutes = 5.0f;
inline constexpr float kMaximumMatchDurationMinutes = 90.0f;
inline constexpr float kDefaultMatchDurationMinutes = 25.0f;
inline constexpr int kMatchDurationSliderSteps = 18;  // 5, 10, ... 90 minutes

inline float MatchDurationMinutesFromSlider(float sliderValue) {
  return kMinimumMatchDurationMinutes +
         std::clamp(sliderValue, 0.0f, 1.0f) *
             (kMaximumMatchDurationMinutes - kMinimumMatchDurationMinutes);
}

inline float MatchDurationSliderFromMinutes(float minutes) {
  return (std::clamp(minutes, kMinimumMatchDurationMinutes, kMaximumMatchDurationMinutes) -
          kMinimumMatchDurationMinutes) /
         (kMaximumMatchDurationMinutes - kMinimumMatchDurationMinutes);
}

// The old normalized setting was presented to players as a 5-25 minute range.
inline float MatchDurationMinutesFromLegacySlider(float sliderValue) {
  return 5.0f + std::clamp(sliderValue, 0.0f, 1.0f) * 20.0f;
}

inline float MatchDurationFactorFromMinutes(float minutes) {
  return std::clamp(minutes, kMinimumMatchDurationMinutes, kMaximumMatchDurationMinutes) / 90.0f;
}

// Convert real elapsed time into the in-game clock. Keep this as a double so
// slider values whose scale is fractional (35, 40, 55 minutes, etc.) do not
// lose a fraction of a millisecond on every simulation tick.
inline double MatchDurationGameTimeFromRealMilliseconds(double realMilliseconds, float minutes,
                                                        float timeScale = 1.0f) {
  const double clampedMinutes =
      std::clamp(static_cast<double>(minutes),
                 static_cast<double>(kMinimumMatchDurationMinutes),
                 static_cast<double>(kMaximumMatchDurationMinutes));
  return realMilliseconds * (90.0 / clampedMinutes) *
         std::max(1.0, static_cast<double>(timeScale));
}

#endif  // _HPP_FOOTBALL_ONTHEPITCH_MATCHDURATION
