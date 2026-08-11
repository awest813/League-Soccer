// Human-only movement speed profile. The canonical velocity constants remain the
// animation-state boundaries used by both human and CPU players.

#ifndef _HPP_FOOTBALL_ONTHEPITCH_HUMANSPEED
#define _HPP_FOOTBALL_ONTHEPITCH_HUMANSPEED

#include <algorithm>

enum class HumanSpeedType { SlowDribble, Run, Sprint };

inline constexpr float kMinimumHumanSlowDribbleSpeed = 2.0f;
inline constexpr float kMaximumHumanSlowDribbleSpeed = 4.1f;
inline constexpr float kDefaultHumanSlowDribbleSpeed = 3.5f;

inline constexpr float kMinimumHumanRunSpeed = 4.3f;
inline constexpr float kMaximumHumanRunSpeed = 5.9f;
inline constexpr float kDefaultHumanRunSpeed = 5.0f;

inline constexpr float kMinimumHumanSprintSpeed = 6.0f;
inline constexpr float kMaximumHumanSprintSpeed = 12.0f;
inline constexpr float kDefaultHumanSprintSpeed = 8.0f;

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
      return 22;  // 0.1 m/s increments
    case HumanSpeedType::Run:
      return 17;  // 0.1 m/s increments
    case HumanSpeedType::Sprint:
      return 25;  // 0.25 m/s increments
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
