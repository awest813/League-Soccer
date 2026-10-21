// Copyright 2019 Google LLC & Bastiaan Konings
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Gamepad layout presets and per-pad analog/rumble options. Pure data, kept
// free of SDL and configuration access so it can be unit tested.

#ifndef _HPP_HID_CONTROLPRESETS
#define _HPP_HID_CONTROLPRESETS

#include <algorithm>
#include <cmath>

#include "ihidevice.hpp"

namespace controlpresets {

// Which physical button (in the pad's own mapping) drives each game function.
enum class GamepadPreset { Pes = 0, Fifa = 1 };
inline constexpr int kGamepadPresetCount = 2;

inline GamepadPreset GamepadPresetFromInt(int value) {
  return value == static_cast<int>(GamepadPreset::Fifa) ? GamepadPreset::Fifa : GamepadPreset::Pes;
}

// PES (the game's long-standing default): A short pass, B high pass,
// X shoot, Y through pass, LB switch, RB sprint, RT slow dribble / finesse.
// FIFA: A short pass, B shoot, X high pass, Y through pass, LB switch,
// RT sprint, RB slow dribble / finesse (RB + B is a finesse shot).
// Defending is the same in both: A press, B sliding tackle, X team
// pressure, Y keeper rush.
inline e_ControllerButton PresetButtonFor(GamepadPreset preset, e_ButtonFunction function) {
  const bool fifa = preset == GamepadPreset::Fifa;
  switch (function) {
    case e_ButtonFunction_Up:
      return e_ControllerButton_Up;
    case e_ButtonFunction_Right:
      return e_ControllerButton_Right;
    case e_ButtonFunction_Down:
      return e_ControllerButton_Down;
    case e_ButtonFunction_Left:
      return e_ControllerButton_Left;
    case e_ButtonFunction_LongPass:
    case e_ButtonFunction_KeeperRush:
      return e_ControllerButton_Y;
    case e_ButtonFunction_HighPass:
      return fifa ? e_ControllerButton_X : e_ControllerButton_B;
    case e_ButtonFunction_Sliding:
      return e_ControllerButton_B;
    case e_ButtonFunction_ShortPass:
    case e_ButtonFunction_Pressure:
      return e_ControllerButton_A;
    case e_ButtonFunction_Shot:
      return fifa ? e_ControllerButton_B : e_ControllerButton_X;
    case e_ButtonFunction_TeamPressure:
      return e_ControllerButton_X;
    case e_ButtonFunction_Switch:
      return e_ControllerButton_L1;
    case e_ButtonFunction_Special:
      return e_ControllerButton_L2;
    case e_ButtonFunction_Sprint:
      return fifa ? e_ControllerButton_R2 : e_ControllerButton_R1;
    case e_ButtonFunction_Dribble:
      return fifa ? e_ControllerButton_R1 : e_ControllerButton_R2;
    case e_ButtonFunction_Select:
      return e_ControllerButton_Select;
    case e_ButtonFunction_Start:
    default:
      return e_ControllerButton_Start;
  }
}

// Stick deadzone choices offered in the menu. The match controller treats
// anything under 15% deflection as standing still, so smaller values would
// have no effect.
inline constexpr float kStickDeadzoneChoices[] = {0.15f, 0.20f, 0.25f, 0.30f};
inline constexpr int kStickDeadzoneChoiceCount = 4;
inline constexpr float kDefaultStickDeadzone = 0.15f;

inline float ClampStickDeadzone(float deadzone) {
  if (!std::isfinite(deadzone))
    return kDefaultStickDeadzone;
  return std::clamp(deadzone, kStickDeadzoneChoices[0],
                    kStickDeadzoneChoices[kStickDeadzoneChoiceCount - 1]);
}

// The next deadzone choice after the current one, wrapping around.
inline float NextStickDeadzone(float current) {
  for (int i = 0; i < kStickDeadzoneChoiceCount; i++) {
    if (current < kStickDeadzoneChoices[i] - 0.001f)
      return kStickDeadzoneChoices[i];
  }
  return kStickDeadzoneChoices[0];
}

// Rumble strength: off, half, full.
inline constexpr float kRumbleChoices[] = {0.0f, 0.5f, 1.0f};
inline constexpr int kRumbleChoiceCount = 3;

inline float ClampRumbleStrength(float strength) {
  if (!std::isfinite(strength))
    return 1.0f;
  return std::clamp(strength, 0.0f, 1.0f);
}

inline float NextRumbleStrength(float current) {
  for (int i = 0; i < kRumbleChoiceCount; i++) {
    if (current < kRumbleChoices[i] - 0.001f)
      return kRumbleChoices[i];
  }
  return kRumbleChoices[0];
}

}  // namespace controlpresets

#endif
