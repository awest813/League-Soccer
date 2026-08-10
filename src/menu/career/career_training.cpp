#include "career_training.hpp"

#include <algorithm>
#include <random>
#include <string>
#include <vector>

#include "career_common.hpp"

namespace blunted {
namespace CareerTraining {

bool TrainSquad(CareerSave& save, CareerCommon::CareerEvents& events) {
  if (save.trainingPoints <= 0)
    return false;
  save.trainingPoints--;
  for (auto& player : save.roster)
    player.matchForm = std::min(100, player.matchForm + 3);
  events.AddEvent("training", "Conducted squad training session", 1, false);
  return true;
}

bool TrainFocus(CareerSave& save, CareerCommon::CareerEvents& events,
                const std::string& focusArea) {
  if (save.trainingPoints <= 0)
    return false;
  save.trainingPoints--;
  int playersImproved = 0;
  for (auto& player : save.roster) {
    if ((focusArea == "Attacking" || focusArea == "Shooting") &&
        (player.preferredPosition == "CF" || player.preferredPosition == "AM")) {
      player.ovr++;
      playersImproved++;
    } else if (focusArea == "Defending" &&
               (player.preferredPosition == "CB" || player.preferredPosition == "LB" ||
                player.preferredPosition == "RB")) {
      player.ovr++;
      playersImproved++;
    }
    player.matchForm = std::min(100, player.matchForm + 3);
  }
  events.AddEvent("training",
                  "Focused training on " + focusArea + " (" + std::to_string(playersImproved) +
                      " players improved)",
                  1, false);
  return true;
}

void SetStrategy(CareerSave& save, CareerCommon::CareerEvents& events,
                 const std::string& strategy) {
  save.activeStrategy = strategy;
  events.AddEvent("strategy", "Changed team strategy to " + strategy, 0, false);
}

void ScoutYouthPlayer(CareerSave& save, CareerCommon::CareerEvents& events) {
  int scoutCost = 50000 * save.scoutingNetworkLevel;
  if (save.transferBudget < scoutCost)
    return;
  save.transferBudget -= scoutCost;
  save.finance.transferBudget = save.transferBudget;

  static const std::vector<std::string> firstNames = {"Leo", "Kai", "Ravi", "Mateo", "Yuki"};
  static const std::vector<std::string> lastNames = {"Martinez", "Tanaka", "Okafor", "Silva",
                                                     "Kim"};
  static const std::vector<std::string> positions = {"CF", "CM", "CB", "AM", "GK"};
  static std::mt19937 rng(std::random_device{}());
  std::uniform_int_distribution<int> fd(0, static_cast<int>(firstNames.size()) - 1);
  std::uniform_int_distribution<int> ld(0, static_cast<int>(lastNames.size()) - 1);
  std::uniform_int_distribution<int> pd(0, static_cast<int>(positions.size()) - 1);
  std::uniform_int_distribution<int> ad(15, 18);

  PlayerCareerState youth;
  youth.name = firstNames[fd(rng)] + " " + lastNames[ld(rng)];
  youth.position = positions[pd(rng)];
  youth.preferredPosition = youth.position;
  youth.age = ad(rng);
  youth.ovr = 50 + (rng() % 10);
  youth.pot = 70 + (rng() % 15);
  youth.wage = 500;
  youth.value = 100000;
  youth.isYouth = true;
  save.youthAcademy.push_back(youth);
  events.AddEvent("scouting",
                  "Scout returned with prospect: " + youth.name + " (" + youth.position + ")", 0,
                  false);
}

void PromoteYouthPlayer(CareerSave& save, CareerCommon::CareerEvents& events,
                        const std::string& playerName) {
  auto it =
      std::find_if(save.youthAcademy.begin(), save.youthAcademy.end(),
                   [&playerName](const PlayerCareerState& p) { return p.name == playerName; });
  if (it == save.youthAcademy.end())
    return;
  PlayerCareerState promoted = *it;
  promoted.contract.yearsRemaining = 4;
  promoted.isYouth = false;
  promoted.morale = 85;
  promoted.matchForm = 55;
  save.roster.push_back(promoted);
  save.youthAcademy.erase(it);
  events.AddEvent("academy", "Promoted academy player " + playerName + " to senior squad.", 1,
                  false);
}

}  // namespace CareerTraining
}  // namespace blunted
