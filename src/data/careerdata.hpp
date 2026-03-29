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

// 6.13 – Press conference dialogue option.
// Used by CareerPressConferencePage to configure question text and per-answer
// reputation impact; future extension point for dynamic question generation.
struct PressConferenceQuestion {
  std::string question;
  std::vector<std::string> answers;   // 3 selectable answers
  std::vector<int> reputationDeltas;  // +/- reputation change per answer index
};

// 6.16 – Per-division promotion/relegation configuration
struct DivisionConfig {
  std::string name;
  int numTeams = 20;
  int promotionSpots = 3;
  int relegationSpots = 3;
  int playOffSpots = 0;  // additional playoff promotion candidates
};

// 6.16 – State for the league expansion / relegation system
struct LeagueExpansionSettings {
  bool enabled = true;
  std::vector<DivisionConfig> divisions;  // top division first
};

// 6.17 – Custom league configuration
struct CustomLeagueConfig {
  std::string leagueName;
  int numDivisions = 1;
  std::vector<DivisionConfig> divisions;
  bool cupCompetition = false;
  std::string cupName;
};

struct CareerSave {
  int saveID = 0;
  CareerMode mode = CareerMode::MANAGER;
  int teamID = 0;
  int controlledEntityID = 0;  // playerID or coachID
  int currentSeason = 1;
  int budget = 0;
  int reputation = 50;  // 0-100, affects board confidence & transfer dealings
  std::string objective;
  std::vector<SeasonRecord> history;

  // 6.16 – league structure active for this save
  LeagueExpansionSettings leagueSettings;
  // 6.17 – non-empty when the save uses a custom league
  CustomLeagueConfig customLeague;
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

  // 6.13 – apply a reputation delta from a press-conference answer
  void ApplyReputationDelta(int saveID, int delta);

  // 6.16 – update league structure for a save
  void SetLeagueExpansionSettings(int saveID, const LeagueExpansionSettings& settings);

  // 6.16 – compute promotion/relegation for a finished season
  //         Returns pairs of (fromDivisionIdx, teamIndex) that move up/down.
  static std::vector<std::pair<int, int>> ComputePromotionRelegation(
      const LeagueExpansionSettings& settings, const std::vector<std::vector<int>>& standings);

  // 6.17 – configure a custom league for a save
  void SetCustomLeague(int saveID, const CustomLeagueConfig& config);

private:
  std::vector<CareerSave> m_saves;
  int m_nextSaveID = 1;
};
