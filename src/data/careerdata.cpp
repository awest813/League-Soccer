#include "careerdata.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

bool CareerSaveRegistry::Load(const std::string& /*path*/) {
  // Future: deserialise from JSON/SQLite; currently a no-op stub
  return true;
}

bool CareerSaveRegistry::Save(const std::string& /*path*/) const {
  // Future: serialise to JSON/SQLite; currently a no-op stub
  return true;
}

int CareerSaveRegistry::CreateSave(const CareerSave& save) {
  CareerSave s = save;
  s.saveID = m_nextSaveID++;
  s.budget = s.finance.transferBudget;
  s.reputation = s.club.reputation;
  m_saves.push_back(s);
  return s.saveID;
}

CareerSave* CareerSaveRegistry::GetSave(int saveID) {
  for (auto& s : m_saves) {
    if (s.saveID == saveID)
      return &s;
  }
  return nullptr;
}

void CareerSaveRegistry::DeleteSave(int saveID) {
  m_saves.erase(std::remove_if(m_saves.begin(), m_saves.end(),
                               [saveID](const CareerSave& s) { return s.saveID == saveID; }),
                m_saves.end());
}

void CareerSaveRegistry::RecordSeason(int saveID, const SeasonRecord& record) {
  CareerSave* s = GetSave(saveID);
  if (s)
    s->history.push_back(record);
}

void CareerSaveRegistry::AdvanceSeason(int saveID) {
  CareerSave* s = GetSave(saveID);
  if (s) {
    s->currentSeason++;
    s->season.currentSeason = s->currentSeason;
    s->season.currentWeek = 1;
    s->season.inPreseason = true;
  }
}

// 6.13 – clamp reputation to [0, 100] after applying delta
void CareerSaveRegistry::ApplyReputationDelta(int saveID, int delta) {
  CareerSave* s = GetSave(saveID);
  if (!s)
    return;
  s->reputation = std::max(0, std::min(100, s->reputation + delta));
  s->club.reputation = s->reputation;
}

// 6.16 – replace the league structure stored in a save
void CareerSaveRegistry::SetLeagueExpansionSettings(int saveID,
                                                    const LeagueExpansionSettings& settings) {
  CareerSave* s = GetSave(saveID);
  if (s)
    s->leagueSettings = settings;
}

// 6.16 – compute which teams move between divisions.
// standings[i] is an ordered list of team IDs for division i (best to worst).
// Returns a list of (divisionIdx, teamID) pairs that are relegated from that
// division; the caller promotes the top N teams from the division below.
std::vector<std::pair<int, int>> CareerSaveRegistry::ComputePromotionRelegation(
    const LeagueExpansionSettings& settings, const std::vector<std::vector<int>>& standings) {
  std::vector<std::pair<int, int>> relegated;
  const int numDivisions = static_cast<int>(settings.divisions.size());
  for (int i = 0; i < numDivisions && i < static_cast<int>(standings.size()); ++i) {
    const DivisionConfig& div = settings.divisions[i];
    const auto& table = standings[i];
    const int numTeams = static_cast<int>(table.size());
    // Bottom N teams are relegated (unless this is the last division)
    if (i < numDivisions - 1) {
      int relSpots = std::min(div.relegationSpots, numTeams);
      for (int j = numTeams - relSpots; j < numTeams; ++j) {
        relegated.emplace_back(i, table[j]);
      }
    }
  }
  return relegated;
}

// 6.17 – attach a custom league configuration to a save
void CareerSaveRegistry::SetCustomLeague(int saveID, const CustomLeagueConfig& config) {
  CareerSave* s = GetSave(saveID);
  if (s)
    s->customLeague = config;
}
