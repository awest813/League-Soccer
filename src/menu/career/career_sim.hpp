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

// Builds the other clubs' records for the current season if the save has
// none yet, catching up with rounds already played. Lists of fewer than ten
// clubs are replaced by a full default league.
void EnsureLeagueTable(CareerSave& save,
                       const std::vector<std::pair<int, std::string>>& leagueClubs = {});

// Plays the other clubs' league round that goes with the user's latest
// result (already added to the season counters). Deterministic per season and
// round; does not draw from the career RNG.
void PlayLeagueRound(CareerSave& save, int userGoals, int opponentGoals);

// The user's place in the current standings (1 = top).
int LeaguePosition(const CareerSave& save);

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

// Roster indices of the players who take part in a match: the controlled
// footballer in Player mode, one goalkeeper and the strongest outfielders, up
// to 11. Deterministic for a given roster.
std::vector<int> SelectMatchSquad(const CareerSave& save);

// Records one appearance, with goals and assists, for a named roster player.
void RecordMatchStats(CareerSave& save, const std::string& playerName, int goals, int assists);

// Simulates a fixture result (and scorer names) for the current roster.
SimulatedMatch SimulateMatchResult(CareerSave& save, const std::string& opponentName,
                                   const std::string& opponentTeamDBID, bool isHome = true);

// Applies a finished match to season W/D/L, goals, board confidence and
// reputation; records an appearance for the match squad and goals for the
// named scorers. Shared by sim and 3D paths.
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
