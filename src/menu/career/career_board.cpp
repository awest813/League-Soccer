#include "career_board.hpp"

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

// Financial bar: high-reputation clubs must bank a real margin, low-tier clubs
// merely need to avoid deep losses. Revenue is clamped for early saves where
// totals may not be initialized yet.
long long FinancialTarget(const CareerSave& save) {
  if (save.reputation >= 70)
    return save.finances.totalExpenses / 4;
  if (save.reputation < 30)
    return -(save.finances.totalRevenue / 10);
  return 0;
}

bool WantsTitle(int reputation) {
  return reputation >= 70;
}

int LeagueFinish(const CareerSave& save) {
  const int liveMatches = save.seasonWins + save.seasonDraws + save.seasonLosses;
  if (liveMatches > 0) {
    return CareerSim::EstimateLeaguePosition(save.seasonWins, save.seasonDraws, save.seasonLosses);
  }
  if (!save.history.empty())
    return save.history.back().leaguePosition;
  return 20;
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
}

void EvaluateBoardObjectives(CareerSave& save, CareerCommon::CareerEvents& events) {
  const int finish = LeagueFinish(save);
  const int promotionTarget = PromotionTarget(save.reputation);
  const int fanBaseTarget = FanBaseTarget(save.reputation);
  const long long financialTarget = FinancialTarget(save);

  for (auto& objective : save.boardObjectives) {
    bool completed = false;
    bool nearMiss = false;
    switch (objective.type) {
      case OwnerObjectiveType::FINANCIAL_STABILITY:
        completed = CareerFinance::GetSeasonProfit(save) >= financialTarget;
        nearMiss = CareerFinance::GetSeasonProfit(save) >= 0;
        break;
      case OwnerObjectiveType::GROW_FANBASE:
        completed = save.fanBase >= fanBaseTarget;
        nearMiss = save.fanBase >= fanBaseTarget - 10;
        break;
      case OwnerObjectiveType::PROMOTION:
        completed = finish <= promotionTarget;
        nearMiss = finish <= promotionTarget + 1;
        break;
      case OwnerObjectiveType::AVOID_RELEGATION:
        completed = finish <= 17;
        nearMiss = finish <= 18;
        break;
      case OwnerObjectiveType::WIN_TITLE:
        completed = finish == 1;
        nearMiss = finish <= 3;
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
  }
}

}  // namespace CareerBoard
}  // namespace blunted