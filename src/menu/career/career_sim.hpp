#ifndef CAREER_SIM_HPP
#define CAREER_SIM_HPP

#include <string>
#include <vector>

#include "../../data/careerdata.hpp"
#include "career_common.hpp"

namespace blunted {
namespace CareerSim {

struct CareerLeagueTableRow {
  int teamID = 0;
  std::string name;
  int played = 0;
  int wins = 0;
  int draws = 0;
  int losses = 0;
  int goalsFor = 0;
  int goalsAgainst = 0;
  int goalDiff = 0;
  int points = 0;
  std::string form;  // e.g. "WDWWL"
  bool isUserTeam = false;
};

struct CareerTopScorer {
  std::string playerName;
  std::string teamName;
  int goals = 0;
  bool isUserPlayer = false;
};

// Generates the full sorted league standings table for the active career.
std::vector<CareerLeagueTableRow> GenerateLeagueStandings(
    const CareerSave& save, const std::vector<std::pair<int, std::string>>& leagueClubs = {});

// Generates the top goalscorers leaderboard for the league.
std::vector<CareerTopScorer> GetTopScorers(const CareerSave& save);

// Calculates season prize money based on final league position (1..20).
long long CalculateSeasonPrizeMoney(int leaguePosition);

// Estimates a 20-team league finish from a W/D/L record (deterministic).
int EstimateLeaguePosition(int wins, int draws, int losses);

// Advances a player's attributes, form, morale, and fitness across a season.
void ProcessPlayerGrowth(PlayerCareerState& player, const CareerSave* save = nullptr);

// Recomputes a player's market value / wage from current attributes.
void UpdatePlayerValue(PlayerCareerState& player);

// Applies goal/assist bookkeeping to a named roster player after a match.
void RecordMatchStats(CareerSave& save, const std::string& playerName, int goals, int assists);

// Simulates a fixture result (and scorer names) for the current roster.
SimulatedMatch SimulateMatchResult(CareerSave& save, const std::string& opponentName,
                                   const std::string& opponentTeamDBID, bool isHome = true);

// Applies a finished match to season W/D/L, goals, board confidence,
// reputation, and optional scorer bookkeeping. Shared by sim and 3D paths.
void ApplyMatchResult(CareerSave& save, CareerCommon::CareerEvents& events, int homeGoals,
                      int awayGoals, const std::string& opponentLabel,
                      const std::vector<std::string>& scorers = {});

void Process3DMatchResult(CareerSave& save, CareerCommon::CareerEvents& events, int homeGoals,
                          int awayGoals);

// Rolls the club over to the next season: records history, awards prize money,
// grows/decrements players, staff and sponsors, advances the calendar, and clears
// transient transfer state. Caller is responsible for persisting.
void AdvanceSeason(CareerSave& save, CareerCommon::CareerEvents& events,
                   std::vector<TransferBid>& bids, std::vector<TransferTarget>& targets);

}  // namespace CareerSim
}  // namespace blunted

#endif  // CAREER_SIM_HPP
