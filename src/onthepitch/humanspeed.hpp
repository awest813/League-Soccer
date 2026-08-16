// Human-only movement speed profile. The canonical velocity constants remain the
// animation-state boundaries used by both human and CPU players.

#ifndef _HPP_FOOTBALL_ONTHEPITCH_HUMANSPEED
#define _HPP_FOOTBALL_ONTHEPITCH_HUMANSPEED

#include <algorithm>

enum class HumanSpeedType { SlowDribble, Run, Sprint };

// PES 5/6 speed profile: players feel weighted and deliberate.
// Sprint is meaningfully faster than a jog but not a rocket-ship.
// Slow dribble is sticky — close control takes commitment.
inline constexpr float kMinimumHumanSlowDribbleSpeed = 1.8f;
inline constexpr float kMaximumHumanSlowDribbleSpeed = 3.8f;
inline constexpr float kDefaultHumanSlowDribbleSpeed = 3.0f;

inline constexpr float kMinimumHumanRunSpeed = 4.2f;
inline constexpr float kMaximumHumanRunSpeed = 5.6f;
inline constexpr float kDefaultHumanRunSpeed = 4.8f;

// Sprint ceiling lowered vs default (8.0→7.0) so elite pace is still
// rewarding but speed doesn't make skill irrelevant.
inline constexpr float kMinimumHumanSprintSpeed = 6.0f;
inline constexpr float kMaximumHumanSprintSpeed = 9.5f;
inline constexpr float kDefaultHumanSprintSpeed = 7.0f;

inline const char* HumanSpeedConfigKey(HumanSpeedType type) {
  switch (type) {
    case HumanSpeedType::SlowDribble:
      return "gameplay_human_slowdribblespeed";
    case HumanSpeedType::Run:
      return "gameplay_human_runspeed";
    case HumanSpeedType::Sprint:
      return "gameplay_human_sprintspeed";
  }
  return "";
}

inline float MinimumHumanSpeed(HumanSpeedType type) {
  switch (type) {
    case HumanSpeedType::SlowDribble:
      return kMinimumHumanSlowDribbleSpeed;
    case HumanSpeedType::Run:
      return kMinimumHumanRunSpeed;
    case HumanSpeedType::Sprint:
      return kMinimumHumanSprintSpeed;
  }
  return 0.0f;
}

inline float MaximumHumanSpeed(HumanSpeedType type) {
  switch (type) {
    case HumanSpeedType::SlowDribble:
      return kMaximumHumanSlowDribbleSpeed;
    case HumanSpeedType::Run:
      return kMaximumHumanRunSpeed;
    case HumanSpeedType::Sprint:
      return kMaximumHumanSprintSpeed;
  }
  return 0.0f;
}

inline float DefaultHumanSpeed(HumanSpeedType type) {
  switch (type) {
    case HumanSpeedType::SlowDribble:
      return kDefaultHumanSlowDribbleSpeed;
    case HumanSpeedType::Run:
      return kDefaultHumanRunSpeed;
    case HumanSpeedType::Sprint:
      return kDefaultHumanSprintSpeed;
  }
  return 0.0f;
}

inline int HumanSpeedSliderSteps(HumanSpeedType type) {
  switch (type) {
    case HumanSpeedType::SlowDribble:
      return 21;  // 1.8 to 3.8 m/s in 0.1 m/s increments (20 intervals + 1)
    case HumanSpeedType::Run:
      return 15;  // 4.2 to 5.6 m/s in 0.1 m/s increments (14 intervals + 1)
    case HumanSpeedType::Sprint:
      return 36;  // 6.0 to 9.5 m/s in 0.1 m/s increments (35 intervals + 1)
  }
  return 2;
}

inline float ClampHumanSpeed(float speed, HumanSpeedType type) {
  return std::clamp(speed, MinimumHumanSpeed(type), MaximumHumanSpeed(type));
}

inline float HumanSpeedFromSlider(float sliderValue, HumanSpeedType type) {
  const float minimum = MinimumHumanSpeed(type);
  return minimum + std::clamp(sliderValue, 0.0f, 1.0f) * (MaximumHumanSpeed(type) - minimum);
}

inline float HumanSpeedSliderFromSpeed(float speed, HumanSpeedType type) {
  const float minimum = MinimumHumanSpeed(type);
  const float range = MaximumHumanSpeed(type) - minimum;
  if (range <= 0.0f)
    return 0.0f;
  return (ClampHumanSpeed(speed, type) - minimum) / range;
}

template <typename Configuration>
float ReadConfiguredHumanSpeed(const Configuration& configuration, HumanSpeedType type) {
  return ClampHumanSpeed(configuration.GetReal(HumanSpeedConfigKey(type), DefaultHumanSpeed(type)),
                         type);
}

#endif  // _HPP_FOOTBALL_ONTHEPITCH_HUMANSPEED
