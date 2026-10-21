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

#ifndef _HPP_GUI2_MENUREPEAT
#define _HPP_GUI2_MENUREPEAT

namespace blunted {

// Auto-repeat for a held menu direction (key, D-pad or stick): one step on
// press, then after an initial delay a steady step rate while it stays held.
// Independent of OS key-repeat settings and of the GUI tick rate.
struct MenuRepeat {
  static constexpr unsigned long kInitialDelay_ms = 400;
  static constexpr unsigned long kInterval_ms = 110;

  // Returns true when a navigation step should fire this tick.
  bool Update(bool held, unsigned long now_ms) {
    if (!held) {
      active = false;
      return false;
    }
    if (!active) {
      active = true;
      next_ms = now_ms + kInitialDelay_ms;
      return true;
    }
    if (now_ms >= next_ms) {
      next_ms = now_ms + kInterval_ms;
      return true;
    }
    return false;
  }

  bool active = false;
  unsigned long next_ms = 0;
};

}  // namespace blunted

#endif
