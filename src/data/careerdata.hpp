#pragma once

#include <map>
#include <string>
#include <vector>

enum class CareerMode { PLAYER, MANAGER, GM, COACH };

struct SeasonRecord {
  int season = 0;
  int teamID = 0;
  int wins = 0;
  int draws = 0;
  int losses = 0;
  int goalsFor = 0;
  int goalsAgainst = 0;
  int leaguePosition = 0;
  bool wonTitle = false;
};

struct CareerSave {
  int saveID = 0;
  CareerMode mode = CareerMode::MANAGER;
  int teamID = 0;
  int controlledEntityID = 0;  // playerID or coachID
  int currentSeason = 1;
  int budget = 0;
  std::string objective;
  std::vector<SeasonRecord> history;
};

class CareerDatabase {
 public:
  CareerDatabase() = default;

  bool Load(const std::string& path);
  bool Save(const std::string& path) const;

  int CreateSave(const CareerSave& save);
  CareerSave* GetSave(int saveID);
  void DeleteSave(int saveID);
  const std::vector<CareerSave>& GetAllSaves() const { return m_saves; }

  void RecordSeason(int saveID, const SeasonRecord& record);
  void AdvanceSeason(int saveID);

 private:
  std::vector<CareerSave> m_saves;
  int m_nextSaveID = 1;
};
