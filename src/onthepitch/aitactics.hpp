// Copyright 2019 Google LLC & Bastiaan Konings
// Licensed under the Apache License, Version 2.0 (the "License");

#ifndef _HPP_AI_TACTICS
#define _HPP_AI_TACTICS

#include <algorithm>

namespace AITactics {

inline float ClampSetting(float value) {
  return std::max(0.0f, std::min(value, 1.0f));
}

// A low setting waits for a clear break; a high setting encourages earlier
// supporting runs. The neutral value remains close to the previous threshold.
inline float GetAttackingRunThreshold(float counterAttack) {
  return 0.60f - ClampSetting(counterAttack) * 0.24f;
}

inline unsigned int GetAttackingRunDuration_ms(float counterAttack) {
  return 2800U + static_cast<unsigned int>(ClampSetting(counterAttack) * 2200.0f);
}

// Territory is -1 at the team's own goal and +1 at the opponent's goal.
inline float GetAttackingTerritory(float ballX, int teamSide, float pitchHalfLength) {
  if (pitchHalfLength <= 0.0f)
    return 0.0f;
  return std::max(-1.0f, std::min(ballX * static_cast<float>(-teamSide) / pitchHalfLength, 1.0f));
}

// Classic zone pressure is selective rather than an automatic all-pitch swarm.
// More aggressive settings activate deeper and from farther away.
inline bool ShouldStartZonePressure(float pressure, float attackingTerritory,
                                    float primaryDefenderDistance) {
  const float setting = ClampSetting(pressure);
  if (setting < 0.05f)
    return false;

  const float territoryThreshold = 0.50f - setting * 0.90f;
  const float distanceThreshold = 6.0f + setting * 10.0f;
  return attackingTerritory >= territoryThreshold && primaryDefenderDistance <= distanceThreshold;
}

inline unsigned int GetZonePressureDuration_ms(float pressure) {
  return 700U + static_cast<unsigned int>(ClampSetting(pressure) * 1600.0f);
}

inline float GetDribbleForwardDrive(float dribbleOffensiveness, float roleMindset) {
  return 0.58f + ClampSetting(dribbleOffensiveness) * 0.28f + ClampSetting(roleMindset) * 0.12f;
}

// Neutral remains the historical 0.75 force-field scale. Lower values create
// shorter passing links; higher values spread support players a little wider.
inline float GetSupportWebScale(float supportDistance) {
  return 0.65f + ClampSetting(supportDistance) * 0.20f;
}

// Defenders still join possession play, but centre-backs retain more of their
// base shape than full-backs.
inline float GetDefenderSupportScale(float roleMindset) {
  return std::min(0.72f + ClampSetting(roleMindset) * 0.52f, 1.0f);
}

// Prefer a nearby midfielder or forward as the second presser when the distance
// difference is small, preserving the back line without making pressure slow.
inline float GetSecondaryPressureRolePenalty(float roleMindset) {
  return (1.0f - ClampSetting(roleMindset)) * 1.0f;
}

// Under pressure, permit a safe lateral or slightly backward pass when it
// creates clear breathing room. This complements, rather than replaces, the
// existing preference for progressive passes.
inline bool ShouldConsiderSupportPass(float currentTacticalRating, float currentSpaceRating,
                                      float teammateTacticalRating, float teammateSpaceRating,
                                      float longPossessionFactor) {
  const float retentionNeed =
      std::max(1.0f - ClampSetting(currentSpaceRating), ClampSetting(longPossessionFactor));
  return retentionNeed >= 0.55f && teammateSpaceRating >= currentSpaceRating + 0.12f &&
         teammateTacticalRating >= currentTacticalRating - 0.15f;
}

inline float GetSupportPassBonus(float currentSpaceRating, float teammateSpaceRating,
                                 float longPossessionFactor) {
  const float spaceGain = ClampSetting((teammateSpaceRating - currentSpaceRating) / 0.4f);
  const float retentionNeed =
      std::max(1.0f - ClampSetting(currentSpaceRating), ClampSetting(longPossessionFactor));
  return spaceGain * retentionNeed * 0.35f;
}

}  // namespace AITactics

#endif
