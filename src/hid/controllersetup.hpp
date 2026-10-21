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

// Keeps a controller-to-team setup consistent when a device disappears.
// Templated on the side type so it can be unit tested without the menu.

#ifndef _HPP_HID_CONTROLLERSETUP
#define _HPP_HID_CONTROLLERSETUP

#include <vector>

namespace controllersetup {

// The setup refers to devices by their index in the controller list. When the
// device at removedIndex is removed, later devices move down one index; drop
// the removed device's entry and renumber the rest. Returns the side the
// removed device was playing for (-1 or 1), or 0 if it was not playing.
template <typename Side>
int RemoveControllerFromSetup(std::vector<Side>& sides, int removedIndex) {
  int removedSide = 0;
  for (auto it = sides.begin(); it != sides.end();) {
    if (it->controllerID == removedIndex) {
      if (it->side != 0)
        removedSide = it->side;
      it = sides.erase(it);
      continue;
    }
    if (it->controllerID > removedIndex)
      it->controllerID--;
    ++it;
  }
  return removedSide;
}

}  // namespace controllersetup

#endif
