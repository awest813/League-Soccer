#ifndef _HPP_UTILS_DIFFICULTY
#define _HPP_UTILS_DIFFICULTY

#include <algorithm>
#include <cmath>
#include <string>

#include "utils/localization.hpp"

inline constexpr float kDefaultDifficulty = 0.8f;

inline int DifficultyToStep(float difficulty) {
  int diff = static_cast<int>(std::round(std::clamp(difficulty, 0.0f, 1.0f) * 4.0f));
  return std::clamp(diff, 0, 4);
}

inline float DifficultyFromStep(int step) {
  return std::clamp(step, 0, 4) * 0.25f;
}

inline std::string GetDifficultyLocaleKey(int step) {
  switch (std::clamp(step, 0, 4)) {
    case 0:
      return "difficulty_beginner";
    case 1:
      return "difficulty_amateur";
    case 2:
      return "difficulty_regular";
    case 3:
      return "difficulty_professional";
    case 4:
    default:
      return "difficulty_top_player";
  }
}

inline std::string GetDifficultyName(float difficulty) {
  const int step = DifficultyToStep(difficulty);
  return Localization::GetInstance().Translate(GetDifficultyLocaleKey(step));
}

#endif  // _HPP_UTILS_DIFFICULTY
