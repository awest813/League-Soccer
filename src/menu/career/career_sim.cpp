#include "career_sim.hpp"

#include <algorithm>
#include <functional>
#include <random>

#include "career_common.hpp"

namespace blunted {
namespace CareerSim {

namespace {
using CareerCommon::ClampInt;
using CareerCommon::RandomInt;
using CareerCommon::SafeStoi;
}  // namespace

int EstimateLeaguePosition(int wins, int draws, int losses) {
  const int played = wins + draws + losses;
  if (played <= 0)
    return 10;
  const int points = wins * 3 + draws;
  // Project points onto a 38-match season, then map onto finish bands.
  const float projected = (static_cast<float>(points) / static_cast<float>(played)) * 38.0f;
  if (projected >= 90.0f)
    return 1;
  if (projected >= 78.0f)
    return 2;
  if (projected >= 70.0f)
    return 4;
  if (projected >= 60.0f)
    return 7;
  if (projected >= 52.0f)
    return 10;
  if (projected >= 45.0f)
    return 12;
  if (projected >= 38.0f)
    return 15;
  if (projected >= 30.0f)
    return 17;
  return 19;
}

void ProcessPlayerGrowth(PlayerCareerState& player) {
  int growthChance = 15;
  if (player.age <= 21)
    growthChance = 55;
  else if (player.age <= 24)
    growthChance = 35;
  else if (player.age >= 31)
    growthChance = 8;

  if (player.matchForm >= 85)
    growthChance += 15;
  else if (player.matchForm >= 60)
    growthChance += 5;
  if (player.morale >= 75)
    growthChance += 5;

  if (player.ovr < player.pot) {
    if (RandomInt(1, 100) <= growthChance)
      player.ovr = std::min(player.ovr + 1, player.pot);
  } else if (player.age >= 31) {
    // Players at their ceiling age out so squads must turn over; the decline
    // risk rises with age instead of switching on abruptly at 32.
    int declineChance = player.age >= 33 ? 35 : 18;
    if (RandomInt(1, 100) <= declineChance)
      player.ovr = std::max(40, player.ovr - 1);
  }

  player.matchForm = ClampInt(player.matchForm - RandomInt(2, 8), 0, 100);
  player.morale = ClampInt(player.morale + RandomInt(-3, 4), 0, 100);
  player.fitness = ClampInt(player.fitness - RandomInt(0, 5), 55, 100);
}

void UpdatePlayerValue(PlayerCareerState& player) {
  long long ageModifier = 120;
  if (player.age >= 30)
    ageModifier = 85;
  else if (player.age <= 21)
    ageModifier = 135;

  long long formModifier = 80 + ClampInt(player.matchForm, 0, 100) / 5;
  long long potentialModifier = 100 + std::max(0, player.pot - player.ovr);
  long long baseValue = static_cast<long long>(player.ovr) * player.ovr * 4000;
  player.value =
      std::max(50000LL, (baseValue * ageModifier * formModifier * potentialModifier) / 1200000LL);
  player.wage = std::max(500LL, player.value / 1200LL);
}

void RecordMatchStats(CareerSave& save, const std::string& playerName, int goals, int assists) {
  auto it = std::find_if(
      save.roster.begin(), save.roster.end(),
      [&playerName](const PlayerCareerState& player) { return player.name == playerName; });
  if (it == save.roster.end())
    return;

  it->matchesPlayed++;
  it->careerGoals += std::max(0, goals);
  it->careerAssists += std::max(0, assists);
  // Keep per-match form gains modest so a hot streak cannot snowball the whole
  // squad's simulated strength across a 38-game season.
  it->matchForm = ClampInt(it->matchForm + goals * 3 + assists * 2 + 1, 0, 100);
  it->morale = ClampInt(it->morale + goals * 2 + assists * 1 + 1, 0, 100);
  UpdatePlayerValue(*it);
}

SimulatedMatch SimulateMatchResult(CareerSave& save, const std::string& opponentName,
                                   const std::string& opponentTeamDBID, bool isHome) {
  SimulatedMatch result;
  result.opponentName = opponentName;

  int teamOVR = 65;
  int opponentOVR = 65;
  int teamMorale = 70;
  int teamForm = 50;
  std::string strategy = save.activeStrategy;

  int ovrSum = 0;
  int moraleSum = 0;
  int formSum = 0;
  int count = 0;
  for (const auto& p : save.roster) {
    ovrSum += p.ovr;
    moraleSum += p.morale;
    formSum += p.matchForm;
    count++;
  }
  if (count > 0) {
    teamOVR = ovrSum / count;
    teamMorale = moraleSum / count;
    teamForm = formSum / count;
  }

  // Wider, identity-stable opponent pool (roughly 45-88) so weak and elite
  // clubs both meet realistic resistance across a season.
  if (!opponentName.empty()) {
    int seed = static_cast<int>(std::hash<std::string>{}(opponentName) % 1000);
    opponentOVR = 45 + (seed % 44);
  } else if (!opponentTeamDBID.empty()) {
    int idValue = SafeStoi(opponentTeamDBID);
    opponentOVR = 45 + ((idValue % 44) + 44) % 44;
  } else {
    opponentOVR = 55 + RandomInt(0, 30);
  }

  int baseAttack = teamOVR + (teamForm - 50) / 8 + (teamMorale - 50) / 12;
  int baseDefense = teamOVR + (teamForm - 50) / 10 + (teamMorale - 50) / 15;
  int oppAttack = opponentOVR + RandomInt(-2, 4);
  int oppDefense = opponentOVR + RandomInt(-2, 3);

  if (strategy == "Attacking") {
    baseAttack += 4;
    baseDefense -= 3;
  } else if (strategy == "Defensive") {
    baseAttack -= 3;
    baseDefense += 4;
  }

  // homeGoals/awayGoals mean "us" / "them" for ApplyMatchResult bookkeeping.
  // Expected goals are primarily OVR-gap driven so club tiers separate cleanly.
  float venueAttack = isHome ? 1.08f : 0.92f;
  float ourXG = 1.05f * venueAttack + 0.06f * static_cast<float>(baseAttack - oppDefense);
  float theirXG = 1.05f / venueAttack + 0.06f * static_cast<float>(oppAttack - baseDefense);
  ourXG = std::max(0.2f, std::min(3.8f, ourXG));
  theirXG = std::max(0.2f, std::min(3.8f, theirXG));

  std::poisson_distribution<int> ourDist(ourXG);
  std::poisson_distribution<int> theirDist(theirXG);
  int expectedHomeGoals = ourDist(CareerCommon::Rng());
  int expectedAwayGoals = theirDist(CareerCommon::Rng());

  result.homeGoals = ClampInt(expectedHomeGoals, 0, 9);
  result.awayGoals = ClampInt(expectedAwayGoals, 0, 7);
  result.homeShots = result.homeGoals + RandomInt(2, 8);
  result.awayShots = result.awayGoals + RandomInt(2, 8);
  result.homePossession = ClampInt(50 + (teamOVR - opponentOVR) + RandomInt(-5, 5) +
                                       (strategy == "Attacking"   ? 5
                                        : strategy == "Defensive" ? -5
                                                                  : 0) +
                                       (isHome ? 3 : -3),
                                   30, 70);
  result.played = true;

  std::vector<int> scorerIndices;
  int rosterSize = static_cast<int>(save.roster.size());
  if (rosterSize <= 0)
    return result;
  for (int g = 0; g < result.homeGoals; g++) {
    int attempts = 0;
    int pIdx;
    do {
      pIdx = RandomInt(0, rosterSize - 1);
      attempts++;
    } while (attempts < 20 &&
             std::find(scorerIndices.begin(), scorerIndices.end(), pIdx) != scorerIndices.end() &&
             rosterSize > static_cast<int>(scorerIndices.size()));
    scorerIndices.push_back(pIdx);
    result.scorers.push_back(save.roster[pIdx].name);
  }

  return result;
}

void ApplyMatchResult(CareerSave& save, CareerCommon::CareerEvents& events, int homeGoals,
                      int awayGoals, const std::string& opponentLabel,
                      const std::vector<std::string>& scorers) {
  const bool isWin = homeGoals > awayGoals;
  const bool isDraw = homeGoals == awayGoals;
  save.seasonWins += isWin ? 1 : 0;
  save.seasonDraws += isDraw ? 1 : 0;
  save.seasonLosses += (!isWin && !isDraw) ? 1 : 0;
  save.seasonGoalsFor += std::max(0, homeGoals);
  save.seasonGoalsAgainst += std::max(0, awayGoals);

  std::string summary =
      save.name + " " + std::to_string(homeGoals) + " - " + std::to_string(awayGoals);
  if (!opponentLabel.empty())
    summary += " " + opponentLabel;
  // Draws are reputation-neutral; only decisive results move the needle.
  // Match results are not major legacy events — season advance / transfers are.
  const int reputationDelta = isWin ? 1 : (isDraw ? 0 : -1);
  events.AddEvent("matchday", summary, reputationDelta, false);
  events.ModifyBoardConfidence(reputationDelta);

  // Squad-wide form/fitness regression after every match prevents mid-season
  // inflation from turning average clubs into perpetual title winners.
  for (auto& player : save.roster) {
    player.matchForm = ClampInt(player.matchForm - RandomInt(1, 3), 25, 100);
    player.fitness = ClampInt(player.fitness - RandomInt(0, 2), 55, 100);
  }

  for (const auto& scorerName : scorers) {
    RecordMatchStats(save, scorerName, 1, 0);
  }
}

void Process3DMatchResult(CareerSave& save, CareerCommon::CareerEvents& events, int homeGoals,
                          int awayGoals) {
  ApplyMatchResult(save, events, homeGoals, awayGoals, "(3D match)");
}

void AdvanceSeason(CareerSave& save, CareerCommon::CareerEvents& events,
                   std::vector<TransferBid>& bids, std::vector<TransferTarget>& targets) {
  SeasonRecord record;
  record.season = save.season.currentSeason;
  record.teamID = save.club.clubID;
  if (save.seasonWins > 0 || save.seasonDraws > 0 || save.seasonLosses > 0) {
    record.wins = save.seasonWins;
    record.draws = save.seasonDraws;
    record.losses = save.seasonLosses;
    record.goalsFor = save.seasonGoalsFor;
    record.goalsAgainst = save.seasonGoalsAgainst;
  } else {
    record.wins = RandomInt(8, 28);
    record.draws = RandomInt(4, 12);
    record.losses = std::max(0, 38 - record.wins - record.draws);
    record.goalsFor = RandomInt(30, 85);
    record.goalsAgainst = RandomInt(20, 70);
  }
  record.leaguePosition = EstimateLeaguePosition(record.wins, record.draws, record.losses);
  record.wonTitle = (record.leaguePosition == 1);
  save.history.push_back(record);

  for (auto& player : save.roster) {
    player.age++;
    if (player.contract.yearsRemaining > 0)
      player.contract.yearsRemaining--;
    ProcessPlayerGrowth(player);
    UpdatePlayerValue(player);
    player.matchesPlayed = 0;
    player.careerGoals = std::max(0, player.careerGoals);
    player.careerAssists = std::max(0, player.careerAssists);
  }

  for (auto& player : save.staff) {
    if (player.contractYearsRemaining > 0)
      player.contractYearsRemaining--;
  }
  save.staff.erase(
      std::remove_if(save.staff.begin(), save.staff.end(),
                     [](const StaffMember& member) { return member.contractYearsRemaining <= 0; }),
      save.staff.end());

  for (auto& sponsor : save.activeSponsors) {
    if (sponsor.yearsRemaining > 0)
      sponsor.yearsRemaining--;
  }
  save.activeSponsors.erase(
      std::remove_if(save.activeSponsors.begin(), save.activeSponsors.end(),
                     [](const SponsorDeal& sponsor) { return sponsor.yearsRemaining <= 0; }),
      save.activeSponsors.end());

  for (auto& upgrade : save.stadium.upgrades) {
    if (upgrade.seasonsRemaining > 0) {
      upgrade.seasonsRemaining--;
      if (upgrade.seasonsRemaining == 0) {
        save.stadium.capacity += upgrade.capacityIncrease;
        save.stadium.matchDayRevenue += upgrade.revenueBonus;
        events.AddEvent("stadium", "Completed stadium upgrade: " + upgrade.name, 1, false);
      }
    }
  }

  save.season.currentSeason++;
  save.currentSeason = save.season.currentSeason;
  save.season.currentWeek = 1;
  save.season.inPreseason = true;
  save.season.transferWindowOpen = true;
  save.trainingPoints = 10;
  save.availableSponsorOffers.clear();
  bids.clear();
  targets.clear();
  save.seasonWins = 0;
  save.seasonDraws = 0;
  save.seasonLosses = 0;
  save.seasonGoalsFor = 0;
  save.seasonGoalsAgainst = 0;
  events.AddEvent("season", "Advanced to season " + std::to_string(save.season.currentSeason), 1,
                  true);
}

}  // namespace CareerSim
}  // namespace blunted
