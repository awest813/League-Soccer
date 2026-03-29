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

#include "localization.hpp"

#include <fstream>
#include <sstream>

#include "base/log.hpp"

using namespace blunted;

// ---------------------------------------------------------------------------
// Singleton accessor
// ---------------------------------------------------------------------------

Localization& Localization::GetInstance() {
  static Localization instance;
  return instance;
}

// ---------------------------------------------------------------------------
// Load
// ---------------------------------------------------------------------------

bool Localization::Load(const std::string& languageCode) {
  std::string path = "data/locale/" + languageCode + ".ini";
  std::ifstream file(path);
  if (!file.is_open()) {
    Log(e_Warning, "Localization", "Load",
        "Could not open locale file: " + path + " – falling back to English");
    if (languageCode != "en") {
      return Load("en");
    }
    return false;
  }

  strings_.clear();
  std::string line;
  while (std::getline(file, line)) {
    // Skip blank lines and comments (# or ;)
    if (line.empty() || line[0] == '#' || line[0] == ';')
      continue;
    auto sep = line.find('=');
    if (sep == std::string::npos)
      continue;
    std::string key = line.substr(0, sep);
    std::string value = line.substr(sep + 1);
    strings_[key] = value;
  }

  currentLanguage_ = languageCode;
  Log(e_Notice, "Localization", "Load",
      "Loaded locale '" + languageCode + "' (" + std::to_string(strings_.size()) + " strings)");
  return true;
}

// ---------------------------------------------------------------------------
// Translate
// ---------------------------------------------------------------------------

const std::string& Localization::Translate(const std::string& key) const {
  auto it = strings_.find(key);
  if (it != strings_.end())
    return it->second;
  // Return the key itself so the UI is always human-readable.
  return key;
}

// ---------------------------------------------------------------------------
// GetCurrentLanguage
// ---------------------------------------------------------------------------

const std::string& Localization::GetCurrentLanguage() const {
  return currentLanguage_;
}

// ---------------------------------------------------------------------------
// GetAvailableLanguages
// ---------------------------------------------------------------------------

std::vector<std::string> Localization::GetAvailableLanguages() {
  // Statically enumerated list that matches the locale files shipped in
  // data/locale/.  Extend this list when adding new translations.
  return {"en", "es", "fr", "de", "pt"};
}

// ---------------------------------------------------------------------------
// GetLanguageDisplayName
// ---------------------------------------------------------------------------

std::string Localization::GetLanguageDisplayName(const std::string& code) {
  if (code == "en")
    return "English";
  if (code == "es")
    return "Español";
  if (code == "fr")
    return "Français";
  if (code == "de")
    return "Deutsch";
  if (code == "pt")
    return "Português";
  return code;
}
