#pragma once

#include <string>
#include <vector>

struct Achievement {
  int id = 0;
  std::string name;
  std::string description;
  bool isUnlocked = false;
  int unlockedSeason = 0;
  int careerSaveID = 0;
};

class AchievementManager {
 public:
  AchievementManager();

  // Call after in-game events; returns true if a new achievement was unlocked
  bool CheckAndUnlock(const std::string& event, int saveID = 0);

  const std::vector<Achievement>& GetAll() const { return m_achievements; }
  std::vector<Achievement> GetUnlocked() const;

 private:
  std::vector<Achievement> m_achievements;

  Achievement* FindByName(const std::string& name);
  void Unlock(const std::string& name, int saveID);
};
