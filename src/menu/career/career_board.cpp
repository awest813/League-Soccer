#include "career_board.hpp"

#include <algorithm>
#include <string>

#include "career_common.hpp"
#include "career_finance.hpp"
#include "career_sim.hpp"

namespace blunted {
namespace CareerBoard {

namespace {
using CareerCommon::ClampInt;

// Objectives scale with club standing so elite clubs chase titles while
// rebuilding clubs get reachable survival targets.
int PromotionTarget(int reputation) {
  if (reputation >= 70)
    return 6;
  if (reputation >= 30)
    return 10;
  return 12;
}

int FanBaseTarget(int reputation) {
  if (reputation >= 70)
    return 80;
  if (reputation >= 30)
    return 60;
  return 55;
}

// Financial bar as a percentage: high-reputation clubs must bank a profit of
// 25% of expenses, mid-table clubs break even, low-tier clubs keep losses
// under 10% of revenue.
int FinancialTargetPercent(int reputation) {
  if (reputation >= 70)
    return 25;
  if (reputation < 30)
    return -10;
  return 0;
}

long long FinancialTarget(const CareerSave& save, int percent) {
  if (percent > 0)
    return save.finances.totalExpenses * percent / 100;
  if (percent < 0)
    return -(save.finances.totalRevenue * -percent / 100);
  return 0;
}

bool WantsTitle(int reputation) {
  return reputation >= 70;
}

int LeagueFinish(const CareerSave& save) {
  const int liveMatches = save.seasonWins + save.seasonDraws + save.seasonLosses;
  if (liveMatches > 0) {
    return CareerSim::LeaguePosition(save);
  }
  if (!save.history.empty())
    return save.history.back().leaguePosition;
  return 20;
}

// The target an objective would get from this reputation (also used for
// objectives loaded from saves that predate stored targets).
int DefaultTarget(OwnerObjectiveType type, int reputation) {
  switch (type) {
    case OwnerObjectiveType::WIN_TITLE:
      return 1;
    case OwnerObjectiveType::PROMOTION:
      return PromotionTarget(reputation);
    case OwnerObjectiveType::AVOID_RELEGATION:
      return 17;
    case OwnerObjectiveType::GROW_FANBASE:
      return FanBaseTarget(reputation);
    case OwnerObjectiveType::FINANCIAL_STABILITY:
      return FinancialTargetPercent(reputation);
  }
  return 0;
}

}  // namespace

void GenerateBoardObjectives(CareerSave& save) {
  save.boardObjectives.clear();
  const int reputation = save.reputation;
  if (WantsTitle(reputation)) {
    save.boardObjectives.push_back(
        {OwnerObjectiveType::WIN_TITLE, "Win the league title", false, 6, -12});
    save.boardObjectives.push_back({OwnerObjectiveType::FINANCIAL_STABILITY,
                                    "Finish with a net profit above 25% of expenses", false, 4,
                                    -8});
    save.boardObjectives.push_back(
        {OwnerObjectiveType::GROW_FANBASE, "Grow the fan base to at least 80k", false, 4, -8});
  } else if (reputation >= 30) {
    save.boardObjectives.push_back(
        {OwnerObjectiveType::PROMOTION, "Reach a top-half finish in the league", false, 5, -10});
    save.boardObjectives.push_back({OwnerObjectiveType::FINANCIAL_STABILITY,
                                    "Finish the season with positive net profit", false, 4, -8});
    save.boardObjectives.push_back(
        {OwnerObjectiveType::GROW_FANBASE, "Grow the fan base to at least 60k", false, 3, -6});
  } else {
    save.boardObjectives.push_back(
        {OwnerObjectiveType::AVOID_RELEGATION, "Avoid relegation this season", false, 4, -8});
    save.boardObjectives.push_back(
        {OwnerObjectiveType::PROMOTION, "Push for a top-half finish", false, 3, -6});
    save.boardObjectives.push_back({OwnerObjectiveType::FINANCIAL_STABILITY,
                                    "Keep losses below 10% of revenue", false, 2, -4});
  }
  for (auto& objective : save.boardObjectives) {
    objective.target = DefaultTarget(objective.type, reputation);
    objective.hasTarget = true;
  }
}

void EvaluateBoardObjectives(CareerSave& save, CareerCommon::CareerEvents& events) {
  const int finish = LeagueFinish(save);
  const long long profit = CareerFinance::GetSeasonProfit(save);
  const int reputation = save.reputation;

  for (auto& objective : save.boardObjectives) {
    const int target =
        objective.hasTarget ? objective.target : DefaultTarget(objective.type, reputation);
    bool completed = false;
    bool nearMiss = false;
    switch (objective.type) {
      case OwnerObjectiveType::FINANCIAL_STABILITY:
        completed = profit >= FinancialTarget(save, target);
        nearMiss = profit >= 0;
        break;
      case OwnerObjectiveType::GROW_FANBASE:
        completed = save.fanBase >= target;
        nearMiss = save.fanBase >= target - 10;
        break;
      case OwnerObjectiveType::PROMOTION:
      case OwnerObjectiveType::AVOID_RELEGATION:
      case OwnerObjectiveType::WIN_TITLE:
        completed = finish <= target;
        nearMiss = finish <= (objective.type == OwnerObjectiveType::WIN_TITLE ? 3 : target + 1);
        break;
    }

    objective.completed = completed;
    if (completed) {
      save.reputation = ClampInt(save.reputation + objective.reputationReward, 0, 100);
      save.club.reputation = save.reputation;
      events.ModifyBoardConfidence(3);
    } else {
      // Near misses cost a fraction of the full penalty so a single poor
      // objective or a tight finish cannot collapse the board overnight.
      int penalty = objective.confidencePenalty;
      if (nearMiss)
        penalty = std::max(-2, penalty / 2);
      events.ModifyBoardConfidence(penalty);
    }
    // The next season's objectives replace these, so record the outcome.
    events.AddEvent("board",
                    (completed ? "Objective met: " : "Objective missed: ") + objective.description,
                    0, false);
  }
}

}  // namespace CareerBoard
}  // namespace blunted