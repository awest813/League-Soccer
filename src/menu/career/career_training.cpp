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
  const bool isCoach = (save.mode == CareerMode::COACH);
  const int formDelta = isCoach ? 6 : 3;
  const int moraleDelta = isCoach ? 3 : 1;
  for (auto& player : save.roster) {
    player.matchForm = std::min(100, player.matchForm + formDelta);
    player.morale = std::min(100, player.morale + moraleDelta);
  }
  events.AddEvent("training",
                  isCoach ? "Head Coach conducted masterclass squad tactical drills."
                          : "Conducted squad training session.",
                  1, false);
  return true;
}

bool TrainFocus(CareerSave& save, CareerCommon::CareerEvents& events,
                const std::string& focusArea) {
  if (save.trainingPoints <= 0)
    return false;
  save.trainingPoints--;
  const bool isCoach = (save.mode == CareerMode::COACH);
  int playersImproved = 0;

  for (auto& player : save.roster) {
    bool eligible = false;
    const std::string& pos = player.preferredPosition;
    
    if (focusArea == "Individual") {
      if (player.databaseID == save.controlledEntityID) {
        player.ovr = std::min(99, player.ovr + 1);
        player.matchForm = std::min(100, player.matchForm + 10);
        player.morale = std::min(100, player.morale + 10);
        events.AddEvent("training", "Completed intense individual training. Attributes improved.", 1, false);
      }
      continue;
    }

    if (focusArea == "Attacking" || focusArea == "Shooting") {
      eligible = (pos == "CF" || pos == "ST" || pos == "AM" || pos == "LW" || pos == "RW" || pos == "FW");
    } else if (focusArea == "Defending") {
      eligible = (pos == "CB" || pos == "LB" || pos == "RB" || pos == "DM" || pos == "GK");
    } else if (focusArea == "Tactical") {
      eligible = (pos == "CM" || pos == "DM" || pos == "AM" || pos == "LM" || pos == "RM" || pos == "WM");
    } else if (focusArea == "Physical") {
      eligible = (player.age <= 28);
    }

    if (eligible) {
      if (player.ovr < player.pot || isCoach) {
        player.ovr = std::min(99, player.ovr + 1);
        playersImproved++;
      }
    }
    player.matchForm = std::min(100, player.matchForm + (isCoach ? 5 : 3));
    if (isCoach) {
      player.morale = std::min(100, player.morale + 2);
    }
  }

  if (focusArea != "Individual") {
    events.AddEvent("training",
                    "Focused training on " + focusArea + " (" + std::to_string(playersImproved) +
                        " players improved" + (isCoach ? " - Coach Boost Active" : "") + ")",
                    1, false);
  }
  return true;
}

void SetStrategy(CareerSave& save, CareerCommon::CareerEvents& events,
                 const std::string& strategy) {
  save.activeStrategy = strategy;
  events.AddEvent("strategy", "Changed team strategy to " + strategy, 0, false);
}

bool MotivatePlayer(CareerSave& save, CareerCommon::CareerEvents& events,
                    const std::string& playerName) {
  for (auto& p : save.roster) {
    if (p.name == playerName) {
      p.morale = std::min(100, p.morale + 15);
      p.matchForm = std::min(100, p.matchForm + 6);
      events.AddEvent("talk", "Head Coach conducted motivational 1-on-1 talk with " + playerName + ".", 1, false);
      return true;
    }
  }
  return false;
}

bool DrillPlayer(CareerSave& save, CareerCommon::CareerEvents& events,
                 const std::string& playerName) {
  if (save.trainingPoints <= 0)
    return false;
  for (auto& p : save.roster) {
    if (p.name == playerName) {
      save.trainingPoints--;
      p.ovr = std::min(99, p.ovr + 1);
      p.matchForm = std::min(100, p.matchForm + 10);
      events.AddEvent("training", "Conducted intensive individual tactical drill with " + playerName + " (+1 OVR).", 1, false);
      return true;
    }
  }
  return false;
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
