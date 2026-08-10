#ifndef CAREER_SIM_HPP
#define CAREER_SIM_HPP

#include <string>
#include <vector>

#include "../../data/careerdata.hpp"
#include "career_common.hpp"

namespace blunted {
namespace CareerSim {

// Estimates a 20-team league finish from a W/D/L record (deterministic).
int EstimateLeaguePosition(int wins, int draws, int losses);

// Advances a player's attributes, form, morale, and fitness across a season.
void ProcessPlayerGrowth(PlayerCareerState& player);

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

// Rolls the club over to the next season: records history, grows/decrements
// players, staff and sponsors, advances the calendar, and clears transient
// transfer state. Caller is responsible for persisting.
void AdvanceSeason(CareerSave& save, CareerCommon::CareerEvents& events,
                   std::vector<TransferBid>& bids, std::vector<TransferTarget>& targets);

}  // namespace CareerSim
}  // namespace blunted

#endif  // CAREER_SIM_HPP
