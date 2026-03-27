// written by bastiaan konings schuiling 2008 - 2015
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not
// be used for anything important. i do not offer support, so don't ask. to be used for inspiration
// :)

#ifndef _HPP_FOOTBALL_ONTHEPITCH_VELOCITYSTATE
#define _HPP_FOOTBALL_ONTHEPITCH_VELOCITYSTATE

// Velocity enum, constants, and quantisation helpers.
// This header has no SDL / OpenGL dependency and can be included in
// unit tests that run without a display.

enum e_Velocity { e_Velocity_Idle, e_Velocity_Dribble, e_Velocity_Walk, e_Velocity_Sprint };

inline constexpr float idleVelocity = 0.0f;
inline constexpr float dribbleVelocity = 3.5f;
inline constexpr float walkVelocity = 5.0f;
inline constexpr float sprintVelocity = 8.0f;

inline constexpr float animSprintVelocity = 7.0f;

inline constexpr float idleDribbleSwitch = 1.8f;
inline constexpr float dribbleWalkSwitch = 4.2f;
inline constexpr float walkSprintSwitch = 6.0f;

// Maps a continuous velocity value to the nearest quantised level.
inline float RangeVelocity(float velocity) {
  float retVelocity = idleVelocity;
  if (velocity >= idleDribbleSwitch && velocity < dribbleWalkSwitch)
    retVelocity = dribbleVelocity;
  else if (velocity >= dribbleWalkSwitch && velocity < walkSprintSwitch)
    retVelocity = walkVelocity;
  else if (velocity >= walkSprintSwitch)
    retVelocity = sprintVelocity;
  return retVelocity;
}

// Clamps velocity to the legal [0, sprintVelocity] range.
inline float ClampVelocity(float velocity) {
  if (velocity < 0)
    return 0;
  if (velocity > sprintVelocity)
    return sprintVelocity;
  return velocity;
}

// Maps a velocity to the minimum quantised level at or above the input.
// Note: 0 and negative values map to walkVelocity (not idleVelocity), while
// any strictly positive value below dribbleVelocity maps to dribbleVelocity.
inline float FloorVelocity(float velocity) {
  float retVelocity = idleVelocity;
  if (velocity > 0 && velocity < dribbleVelocity)
    retVelocity = dribbleVelocity;
  else if (velocity <= walkVelocity)
    retVelocity = walkVelocity;
  else
    retVelocity = sprintVelocity;
  return retVelocity;
}

// Converts an e_Velocity enum value to its representative float.
inline float EnumToFloatVelocity(e_Velocity velocity) {
  switch (velocity) {
    case e_Velocity_Idle:
      return idleVelocity;
    case e_Velocity_Dribble:
      return dribbleVelocity;
    case e_Velocity_Walk:
      return walkVelocity;
    case e_Velocity_Sprint:
      return sprintVelocity;
  }
  return 0.0f;
}

// Converts a float velocity to the corresponding e_Velocity enum value.
inline e_Velocity FloatToEnumVelocity(float velocity) {
  float rangedVelocity = RangeVelocity(velocity);
  if (rangedVelocity == idleVelocity)
    return e_Velocity_Idle;
  else if (rangedVelocity == dribbleVelocity)
    return e_Velocity_Dribble;
  else if (rangedVelocity == walkVelocity)
    return e_Velocity_Walk;
  else if (rangedVelocity == sprintVelocity)
    return e_Velocity_Sprint;
  else
    return e_Velocity_Idle;
}

#endif  // _HPP_FOOTBALL_ONTHEPITCH_VELOCITYSTATE
