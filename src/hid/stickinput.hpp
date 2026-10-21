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

// Pure helpers for gamepad stick/trigger handling, kept free of SDL so they can
// be unit tested.

#ifndef _HPP_HID_STICKINPUT
#define _HPP_HID_STICKINPUT

#include <cmath>

#include "base/math/vector3.hpp"

namespace stickinput {

// A controller mapping >= 0 is a raw button. A negative mapping encodes one
// direction of an axis: -(axis * 2 + 1) is the negative half and
// -(axis * 2 + 2) the positive half, e.g. -1 = axis 0 negative, -12 = axis 5
// positive. Returns false for button mappings.
inline bool DecodeAxisBinding(int mapping, int* axis, int* sign) {
  if (mapping >= 0)
    return false;
  const int encoded = -mapping - 1;
  *axis = encoded / 2;
  *sign = (encoded % 2) * 2 - 1;
  return true;
}

inline int EncodeAxisBinding(int axis, int sign) {
  return -(axis * 2 + (sign > 0 ? 2 : 1));
}

// Analog triggers rest at one end of their range (about -32768) rather than in
// the middle like sticks. When the device's initial axis value says so, use it
// as the calibration rest so a released trigger reads 0 and a full pull reads 1;
// otherwise keep the stick-style centre rest.
inline float DefaultAxisRest(float initialState) {
  return std::fabs(initialState) > 16384.0f ? initialState : 0.0f;
}

// Radial deadzone for a stick direction. Inputs inside the deadzone read as
// zero; others keep their magnitude (capped at 1) so callers can still map
// partial deflection to speed. Unlike a per-axis deadzone this does not snap
// near-cardinal directions onto the axes.
inline blunted::Vector3 ApplyRadialDeadzone(const blunted::Vector3& input, float deadzone) {
  const float length = input.GetLength();
  if (length < deadzone || length <= 0.0f)
    return blunted::Vector3(0);
  if (length > 1.0f)
    return input * (1.0f / length);
  return input;
}

}  // namespace stickinput

#endif
