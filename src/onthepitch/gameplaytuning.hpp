// Copyright 2019 Google LLC & Bastiaan Konings
// Licensed under the Apache License, Version 2.0 (the "License");

#ifndef _HPP_GAMEPLAY_TUNING
#define _HPP_GAMEPLAY_TUNING

#include <algorithm>
#include <cmath>

namespace GameplayTuning {

inline float Clamp01(float value) {
  return std::max(0.0f, std::min(value, 1.0f));
}

// Adds a small, explainable first-touch penalty under close pressure or when a
// fast incoming ball arrives from outside the player's field of view.
inline float GetFirstTouchContextPenalty(float opponentDistance, float calmness, float balance,
                                         float incomingBallSpeed, float incomingFacingAlignment,
                                         int playerCondition = 3) {
  const float pressure = Clamp01((1.6f - opponentDistance) / 1.2f);
  const float composure = Clamp01(calmness * 0.65f + balance * 0.35f);
  const float ballPace = Clamp01((incomingBallSpeed - 4.0f) / 10.0f);
  const float blindSide = Clamp01((0.25f - incomingFacingAlignment) / 1.25f);

  // PES 5/6: pressure penalty is sharper (0.10 vs 0.08) — being closed down
  // under a hard incoming ball is genuinely dangerous.
  const float pressurePenalty = pressure * (1.0f - composure * 0.65f) * 0.10f;
  const float orientationPenalty = ballPace * blindSide * (1.0f - composure * 0.4f) * 0.10f;
  
  // PES condition penalty (3 is neutral, 1 is terrible, 5 is excellent)
  // PES condition penalty: 3 is neutral, 1 is poor, 5 is excellent.
  // Weight 0.03 makes a 2-star condition gap produce ~0.06 extra error —
  // enough to be perceptible but not overwhelming.
  float conditionMod = (3 - playerCondition) * 0.03f;
  
  return std::min(std::max(0.0f, pressurePenalty + orientationPenalty + conditionMod), 0.25f);
}

// Distance remains the primary fatigue input. This workload factor makes
// jogging slightly cheaper and repeated sprinting slightly more expensive.
inline float GetFatigueWorkloadFactor(float movementSpeed, float maximumSpeed, bool carryingBall) {
  if (maximumSpeed <= 0.0f)
    return 1.0f;

  const float speedRatio = Clamp01(movementSpeed / maximumSpeed);
  const float sprintLoad = Clamp01((speedRatio - 0.55f) / 0.45f);
  // PES 5/6: sprinting is notably more taxing than jogging (0.36 vs 0.28)
  // and carrying the ball while at full pace costs extra (0.06 vs 0.04).
  // Late in a match, fatigued players visibly slow down.
  float workload = 0.90f + sprintLoad * sprintLoad * 0.36f;
  if (carryingBall)
    workload += sprintLoad * 0.06f;
  return workload;
}

inline bool IsGoalMouthThreat(float lateralPosition, float ballHeight, float goalHalfWidth,
                              float goalHeight, float anticipationFactor) {
  const float anticipation = std::max(anticipationFactor, 1.0f);
  const float anticipatedHalfWidth = goalHalfWidth * anticipation;
  const float anticipatedHeight = goalHeight + 0.11f + (anticipation - 1.0f) * 0.25f;
  return std::fabs(lateralPosition) <= anticipatedHalfWidth && ballHeight >= 0.0f &&
         ballHeight <= anticipatedHeight;
}

}  // namespace GameplayTuning

#endif  // _HPP_GAMEPLAY_TUNING
