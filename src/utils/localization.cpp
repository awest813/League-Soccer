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

#include <cstdlib>
#include <fstream>
#include <sstream>

#include "base/log.hpp"

using namespace blunted;

namespace {

std::string UnescapeLocaleValue(const std::string& value) {
  std::string result;
  result.reserve(value.size());
  for (size_t i = 0; i < value.size(); ++i) {
    if (value[i] != '\\' || i + 1 >= value.size()) {
      result += value[i];
      continue;
    }

    const char escaped = value[++i];
    switch (escaped) {
      case 'n':
        result += '\n';
        break;
      case 'r':
        result += '\r';
        break;
      case 't':
        result += '\t';
        break;
      case '\\':
        result += '\\';
        break;
      default:
        result += '\\';
        result += escaped;
        break;
    }
  }
  return result;
}

bool LoadLocaleFile(const std::string& languageCode,
                    std::unordered_map<std::string, std::string>* strings,
                    std::string* loadedPath) {
  const std::string paths[] = {
      "data/locale/" + languageCode + ".ini",
      "locale/" + languageCode + ".ini",
  };

  for (const std::string& path : paths) {
    std::ifstream file(path);
    if (!file.is_open())
      continue;

    strings->clear();
    std::string line;
    while (std::getline(file, line)) {
      if (line.empty() || line[0] == '#' || line[0] == ';')
        continue;
      const size_t separator = line.find('=');
      if (separator == std::string::npos)
        continue;

      const std::string key = line.substr(0, separator);
      (*strings)[key] = UnescapeLocaleValue(line.substr(separator + 1));
    }
    if (loadedPath)
      *loadedPath = path;
    return true;
  }
  return false;
}

}  // namespace

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
  std::string englishPath;
  fallbackStrings_.clear();
  if (!LoadLocaleFile("en", &fallbackStrings_, &englishPath)) {
    Log(e_Warning, "Localization", "Load",
        "Could not open English locale file from data/locale or locale");
    strings_.clear();
    return false;
  }

  if (languageCode == "en") {
    strings_ = fallbackStrings_;
    currentLanguage_ = "en";
    Log(e_Notice, "Localization", "Load",
        "Loaded locale 'en' from " + englishPath + " (" +
            std::to_string(strings_.size()) + " strings)");
    return true;
  }

  std::string localizedPath;
  if (!LoadLocaleFile(languageCode, &strings_, &localizedPath)) {
    Log(e_Warning, "Localization", "Load",
        "Could not open locale '" + languageCode +
            "' from data/locale or locale; falling back to English");
    strings_ = fallbackStrings_;
    currentLanguage_ = "en";
    return true;
  }

  currentLanguage_ = languageCode;
  Log(e_Notice, "Localization", "Load",
      "Loaded locale '" + languageCode + "' from " + localizedPath + " (" +
          std::to_string(strings_.size()) + " translated, " +
          std::to_string(fallbackStrings_.size()) + " English fallbacks)");
  return true;
}

// ---------------------------------------------------------------------------
// Translate
// ---------------------------------------------------------------------------

std::string Localization::Translate(const std::string& key) const {
  auto it = strings_.find(key);
  if (it != strings_.end())
    return it->second;
  auto fallback = fallbackStrings_.find(key);
  if (fallback != fallbackStrings_.end())
    return fallback->second;
  return key;
}

// ---------------------------------------------------------------------------
// TranslateAndFormat
// ---------------------------------------------------------------------------

std::string Localization::TranslateAndFormat(const std::string& key,
                                             const std::vector<std::string>& args) const {
  std::string template_ = Translate(key);
  std::string result;
  result.reserve(template_.size() + 32);
  size_t i = 0;
  while (i < template_.size()) {
    if (template_[i] == '{') {
      size_t end = template_.find('}', i + 1);
      if (end != std::string::npos) {
        bool numeric = true;
        for (size_t k = i + 1; k < end; ++k) {
          if (template_[k] < '0' || template_[k] > '9') {
            numeric = false;
            break;
          }
        }
        if (numeric && !template_.substr(i + 1, end - i - 1).empty()) {
          size_t index = static_cast<size_t>(
              atoi(template_.substr(i + 1, end - i - 1).c_str()));
          if (index < args.size()) {
            result += args[index];
            i = end + 1;
            continue;
          }
        }
      }
    }
    result += template_[i];
    ++i;
  }
  return result;
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
