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

// Stable gamepad identity for saved settings, plus migration of settings that
// were saved under the old per-connection identity. Pure; unit tested.

#ifndef _HPP_HID_DEVICEIDENTITY
#define _HPP_HID_DEVICEIDENTITY

#include <map>
#include <string>
#include <vector>

namespace deviceidentity {

// Saved gamepad settings used to be keyed by "<name> #<SDL device index>", so
// the same pad lost (or swapped) its mapping when pads enumerated in another
// order. The SDL GUID identifies the model, and the duplicate index tells two
// identical pads apart in connection order.
inline std::string GamepadIdentifier(const std::string& guid, int duplicateIndex) {
  const std::string base = guid.empty() ? std::string("gamepad") : guid;
  return duplicateIndex > 0 ? base + "-" + std::to_string(duplicateIndex + 1) : base;
}

inline std::string LegacyGamepadIdentifier(const std::string& name, int deviceIndex) {
  return (name.empty() ? std::string("Gamepad") : name) + " #" + std::to_string(deviceIndex);
}

inline bool HasKeyWithPrefix(const std::map<std::string, std::string>& properties,
                             const std::string& prefix) {
  auto it = properties.lower_bound(prefix);
  return it != properties.end() && it->first.compare(0, prefix.size(), prefix) == 0;
}

// Copies every "<fromPrefix>..." key to "<toPrefix>..." when nothing has been
// saved under toPrefix yet. Old keys are left in place so an older build can
// still read them. Returns the number of keys copied.
inline int MigratePrefixedKeys(std::map<std::string, std::string>& properties,
                               const std::string& fromPrefix, const std::string& toPrefix) {
  if (fromPrefix == toPrefix || HasKeyWithPrefix(properties, toPrefix))
    return 0;
  std::vector<std::pair<std::string, std::string>> copies;
  for (auto it = properties.lower_bound(fromPrefix);
       it != properties.end() && it->first.compare(0, fromPrefix.size(), fromPrefix) == 0; ++it) {
    copies.emplace_back(toPrefix + it->first.substr(fromPrefix.size()), it->second);
  }
  for (const auto& copy : copies)
    properties[copy.first] = copy.second;
  return static_cast<int>(copies.size());
}

}  // namespace deviceidentity

#endif
