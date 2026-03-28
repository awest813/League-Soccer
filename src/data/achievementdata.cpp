#include "achievementdata.hpp"

#include <algorithm>

AchievementManager::AchievementManager() {
  // Seed the full list of achievements
  m_achievements = {
      {1, "First Title", "Win your first league title", false, 0, 0},
      {2, "Hat-trick Hero", "Score a hat-trick in a single match", false, 0, 0},
      {3, "Clean Sheet King", "Keep 10 clean sheets in one season", false, 0, 0},
      {4, "Century Striker", "Score 100 career goals", false, 0, 0},
      {5, "Transfer Mastermind", "Complete 10 transfers in one window", false, 0, 0},
      {6, "Youth Guru", "Promote 5 youth players to the first team", false, 0, 0},
      {7, "Continental Conqueror", "Win an international cup", false, 0, 0},
      {8, "Unbeaten Season", "Go an entire season without losing", false, 0, 0},
      {9, "Hall of Fame", "Complete 10 seasons in career mode", false, 0, 0},
      {10, "Scout Mastermind", "Sign a player rated 80+ discovered by your scout", false, 0, 0},
  };
}

Achievement* AchievementManager::FindByName(const std::string& name) {
  for (auto& a : m_achievements) {
    if (a.name == name) return &a;
  }
  return nullptr;
}

void AchievementManager::Unlock(const std::string& name, int saveID) {
  Achievement* a = FindByName(name);
  if (a && !a->isUnlocked) {
    a->isUnlocked = true;
    a->careerSaveID = saveID;
  }
}

bool AchievementManager::CheckAndUnlock(const std::string& event, int saveID) {
  static const struct {
    const char* event;
    const char* achievement;
  } kMapping[] = {
      {"win_title", "First Title"},
      {"hat_trick", "Hat-trick Hero"},
      {"10_clean_sheets", "Clean Sheet King"},
      {"100_goals", "Century Striker"},
      {"10_transfers", "Transfer Mastermind"},
      {"5_youth_promoted", "Youth Guru"},
      {"win_international_cup", "Continental Conqueror"},
      {"unbeaten_season", "Unbeaten Season"},
      {"10_seasons", "Hall of Fame"},
      {"scout_sign_80plus", "Scout Mastermind"},
  };

  for (const auto& m : kMapping) {
    if (event == m.event) {
      Achievement* a = FindByName(m.achievement);
      if (a && !a->isUnlocked) {
        Unlock(m.achievement, saveID);
        return true;
      }
      return false;
    }
  }
  return false;
}

std::vector<Achievement> AchievementManager::GetUnlocked() const {
  std::vector<Achievement> result;
  for (const auto& a : m_achievements) {
    if (a.isUnlocked) result.push_back(a);
  }
  return result;
}
