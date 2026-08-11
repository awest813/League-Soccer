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

#ifndef _HPP_UTILS_LOCALIZATION
#define _HPP_UTILS_LOCALIZATION

#include <string>
#include <unordered_map>
#include <vector>

// Localization / i18n scaffolding (task 4.6)
//
// Usage:
//   Localization::GetInstance().Load("es");   // load Spanish
//   std::string s = TR("menu_match");         // "Partido"
//
// Translation files live under  data/locale/<lang_code>.ini
// Each line is:  key=Translated string

class Localization {
public:
  static Localization& GetInstance();

  // Load a locale file from data/locale/<languageCode>.ini.
  // Returns true on success.  Falls back to English on failure.
  bool Load(const std::string& languageCode);

  // Return the translated string for key. Missing entries fall back to the
  // English catalog, then to the key itself.
  std::string Translate(const std::string& key) const;

  // Translate a key, then replace {0}, {1}, ... with the corresponding values.
  // Falls back to the key (with placeholders intact) when not found.
  std::string TranslateAndFormat(const std::string& key,
                                 const std::vector<std::string>& args) const;

  // The language code that is currently active, e.g. "en", "es", "fr".
  const std::string& GetCurrentLanguage() const;

  // Language codes that have a locale file in data/locale/.
  static std::vector<std::string> GetAvailableLanguages();

  // Human-readable display name for a language code, e.g. "es" -> "Español".
  static std::string GetLanguageDisplayName(const std::string& code);

private:
  Localization() = default;
  Localization(const Localization&) = delete;
  Localization& operator=(const Localization&) = delete;

  std::unordered_map<std::string, std::string> strings_;
  std::unordered_map<std::string, std::string> fallbackStrings_;
  std::string currentLanguage_{"en"};
};

// Global helper – mirrors gettext-style usage.
inline std::string TR(const std::string& key) {
  return Localization::GetInstance().Translate(key);
}

// Translate a key then substitute {0}, {1}, ... placeholders with the given
// values. Lets UI code keep one source of truth (the locale file) for strings
// that embed dynamic data such as names, ratings or money.
inline std::string TRF(const std::string& key, const std::vector<std::string>& args) {
  return Localization::GetInstance().TranslateAndFormat(key, args);
}

#endif  // _HPP_UTILS_LOCALIZATION
