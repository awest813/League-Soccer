#include <algorithm>
#include <string>

#include <gtest/gtest.h>

#include "data/careerdata.hpp"
#include "menu/career/career_board.hpp"
#include "menu/career/career_common.hpp"
#include "menu/career/career_finance.hpp"
#include "menu/career/career_sim.hpp"
#include "menu/career/career_training.hpp"
#include "menu/career/career_transfers.hpp"
#include "utils/localization.hpp"

namespace {

using blunted::CareerCommon::CareerEvents;
using blunted::CareerSim::ProcessPlayerGrowth;
using blunted::CareerSim::UpdatePlayerValue;

// Test double for the career event sink: records board-confidence deltas so a
// test can assert who got credited / penalized.
class RecordingEvents : public CareerEvents {
public:
  void AddEvent(const std::string&, const std::string&, int, bool) override {}

  void ModifyBoardConfidence(int delta) override { confidenceDelta += delta; }

  int confidenceDelta = 0;
};

// ---------------------------------------------------------------------------
// CareerSim: player development / valuation
// ---------------------------------------------------------------------------

TEST(CareerModuleSimTest, PlayerGrowthNeverExceedsPotential) {
  blunted::CareerCommon::SeedRng(1u);
  PlayerCareerState p;
  p.age = 19;
  p.ovr = 55;
  p.pot = 70;
  for (int i = 0; i < 40; ++i)
    ProcessPlayerGrowth(p);
  EXPECT_GT(p.ovr, 55);     // strong young-growth odds develop the player
  EXPECT_LE(p.ovr, p.pot);  // ...but never beyond his ceiling
  EXPECT_GE(p.ovr, 40);
}

TEST(CareerModuleSimTest, AgingVeteranDeclinesAtCeiling) {
  blunted::CareerCommon::SeedRng(7u);
  PlayerCareerState p;
  p.age = 40;
  p.ovr = 90;
  p.pot = 90;
  for (int i = 0; i < 20; ++i)
    ProcessPlayerGrowth(p);
  EXPECT_LT(p.ovr, 90);  // an over-33 veteran at his ceiling starts to decline
  EXPECT_GE(p.ovr, 40);
}

TEST(CareerModuleSimTest, ScoutingYouthSpendsBudgetAndAddsProspect) {
  CareerSave save;
  save.transferBudget = 5000000;
  save.scoutingNetworkLevel = 1;
  RecordingEvents events;
  blunted::CareerTraining::ScoutYouthPlayer(save, events);
  ASSERT_EQ(save.youthAcademy.size(), 1u);
  EXPECT_EQ(save.transferBudget, 5000000 - 50000);
}

// ---------------------------------------------------------------------------
// CareerTransfers: valuation and bid resolution
// ---------------------------------------------------------------------------

TEST(CareerTransferModuleTest, MarketValueFavoursYouthAndPotential) {
  const long long youngster = blunted::CareerTransfers::ComputeMarketValue(80, 92, 19);
  const long long veteran = blunted::CareerTransfers::ComputeMarketValue(80, 85, 33);
  const long long lowCeiling = blunted::CareerTransfers::ComputeMarketValue(80, 80, 22);
  EXPECT_GT(youngster, veteran) << "a 19yo with high potential must out-value an aging vet";
  EXPECT_GT(youngster, lowCeiling) << "potential headroom must add value";
  EXPECT_GE(youngster, 50000LL);
}

TEST(CareerTransferModuleTest, FullBidAcceptedAndCompleted) {
  CareerSave save;
  save.transferBudget = 100000000;
  save.wageBudget = 1000000;
  save.finance.transferBudget = save.transferBudget;

  TransferTarget target;
  target.name = "Star Striker";
  target.overallRating = 84;
  target.potentialRating = 91;
  target.age = 22;
  target.askingPrice = 20000000;
  target.wage = 400000;
  target.value = 18000000;
  std::vector<TransferTarget> targets{target};
  std::vector<TransferBid> bids;
  RecordingEvents events;

  const TransferBid bid = blunted::CareerTransfers::PlaceBid(save, events, targets, bids,
                                                             "Star Striker", 20000000, 400000, 3);
  EXPECT_EQ(bid.status, BidStatus::PENDING);
  blunted::CareerTransfers::ProcessPendingBids(save, events, targets, bids);
  EXPECT_EQ(bids[0].status, BidStatus::ACCEPTED);
  EXPECT_TRUE(
      blunted::CareerTransfers::CompleteTransfer(save, events, targets, bids, "Star Striker"));
  ASSERT_EQ(save.roster.size(), 1u);
  EXPECT_EQ(save.roster[0].name, "Star Striker");
  EXPECT_EQ(save.roster[0].contract.yearsRemaining, 3);
  EXPECT_LT(save.transferBudget, 100000000);
}

TEST(CareerTransferModuleTest, NegotiationDiscountIsProgressive) {
  CareerSave save;
  save.transferBudget = 100000000;
  save.wageBudget = 1000000;

  auto runBid = [&save](long long negotiationRounds) {
    TransferTarget target;
    target.name = "Target";
    target.overallRating = 70;
    target.potentialRating = 78;
    target.age = 24;
    target.askingPrice = 16000000;
    target.wage = 300000;
    std::vector<TransferTarget> targets{target};
    std::vector<TransferBid> bids;
    RecordingEvents events;
    TransferBid bid = blunted::CareerTransfers::PlaceBid(save, events, targets, bids, "Target",
                                                         14000000, 300000, 3);
    bid.negotiationRounds = static_cast<int>(negotiationRounds);
    bids = {bid};
    blunted::CareerTransfers::ProcessPendingBids(save, events, targets, bids);
    return bids.empty() ? BidStatus::REJECTED : bids[0].status;
  };

  EXPECT_EQ(runBid(1), BidStatus::REJECTED);  // 5% off asking still too high
  EXPECT_EQ(runBid(3), BidStatus::ACCEPTED);  // 15% off asking clears the bar
}

TEST(CareerTransferModuleTest, UnaffordableNegotiationLeavesBidUnchanged) {
  TransferBid bid;
  bid.status = BidStatus::PENDING;
  bid.bidAmount = 1000000;
  bid.agentFee = 50000;

  EXPECT_FALSE(blunted::CareerTransfers::ImprovePendingBid(bid, 1100000));
  EXPECT_EQ(bid.bidAmount, 1000000);
  EXPECT_EQ(bid.agentFee, 50000);
  EXPECT_EQ(bid.negotiationRounds, 0);

  EXPECT_TRUE(blunted::CareerTransfers::ImprovePendingBid(bid, 1200000));
  EXPECT_EQ(bid.bidAmount, 1100000);
  EXPECT_EQ(bid.agentFee, 55000);
  EXPECT_EQ(bid.negotiationRounds, 1);
}

// ---------------------------------------------------------------------------
// CareerFinance: budget / profit / health / ticket price
// ---------------------------------------------------------------------------

TEST(CareerFinanceModuleTest, ModifyBudgetKeepsMirrorsConsistent) {
  CareerSave save;
  save.transferBudget = 10000000;
  save.wageBudget = 1000000;
  blunted::CareerFinance::ModifyBudget(save, 2000000, -100000);
  EXPECT_EQ(save.transferBudget, 12000000);
  EXPECT_EQ(save.wageBudget, 900000);
  EXPECT_EQ(save.finance.transferBudget, 12000000);
  EXPECT_EQ(save.finance.wageBudget, 900000);
}

TEST(CareerFinanceModuleTest, FinancialHealthStringTiers) {
  ASSERT_TRUE(Localization::GetInstance().Load("en"));
  CareerSave save;
  save.finances.totalRevenue = 20000000;
  save.finances.totalExpenses = 15000000;  // +5M profit
  save.finances.netWorth = 200000000;
  EXPECT_EQ(blunted::CareerFinance::GetFinancialHealthString(save), "Elite");

  save.finances.netWorth = 10000000;
  EXPECT_EQ(blunted::CareerFinance::GetFinancialHealthString(save), "Critical");
}

TEST(LocalizationTest, FormatsMultilineCareerTextAndFallsBackToEnglish) {
  ASSERT_TRUE(Localization::GetInstance().Load("en"));
  EXPECT_EQ(TR("career_hub_title"), "Career Hub");
  EXPECT_EQ(TR("career_menu_coach"), "Coach\nMatchday leadership");
  EXPECT_EQ(TRF("career_progress_line", {"2", "38", "1", "0", "1", "3", "2"}),
            "Week 2/38 | W 1  D 0  L 1 | GF 3  GA 2");

  ASSERT_TRUE(Localization::GetInstance().Load("es"));
  EXPECT_EQ(TR("menu_match"), "Partido");
  EXPECT_EQ(TR("career_hub_title"), "Career Hub");
}

TEST(CareerFinanceModuleTest, SetTicketPriceClamps) {
  CareerSave save;
  save.fanBase = 60;
  blunted::CareerFinance::SetTicketPrice(save, 500);
  EXPECT_EQ(save.finances.ticketPrice, 200);
  EXPECT_LT(save.fanBase, 60);  // raising prices above 40 hurts the fan base
  blunted::CareerFinance::SetTicketPrice(save, 5);
  EXPECT_EQ(save.finances.ticketPrice, 10);
}

// ---------------------------------------------------------------------------
// CareerBoard: tier-scaled objectives and near-miss penalties
// ---------------------------------------------------------------------------

TEST(CareerBoardModuleTest, ObjectivesScaleWithReputation) {
  CareerSave elite;
  elite.reputation = 85;
  blunted::CareerBoard::GenerateBoardObjectives(elite);
  const bool hasTitle = std::any_of(
      elite.boardObjectives.begin(), elite.boardObjectives.end(),
      [](const OwnerBoardObjective& o) { return o.type == OwnerObjectiveType::WIN_TITLE; });
  EXPECT_TRUE(hasTitle);

  CareerSave minnow;
  minnow.reputation = 10;
  blunted::CareerBoard::GenerateBoardObjectives(minnow);
  const bool avoidsRelegation = std::any_of(
      minnow.boardObjectives.begin(), minnow.boardObjectives.end(),
      [](const OwnerBoardObjective& o) { return o.type == OwnerObjectiveType::AVOID_RELEGATION; });
  EXPECT_TRUE(avoidsRelegation);
}

TEST(CareerBoardModuleTest, NearMissPenaltyIsHalved) {
  CareerSave save;
  save.reputation = 50;
  save.boardConfidence = 50;
  save.fanBase = 55;  // just below the 60 target -> near miss
  save.boardObjectives.clear();
  save.boardObjectives.push_back(
      {OwnerObjectiveType::GROW_FANBASE, "Grow the fan base to at least 60k", false, 3, -6});
  RecordingEvents events;
  blunted::CareerBoard::EvaluateBoardObjectives(save, events);
  EXPECT_FALSE(save.boardObjectives[0].completed);
  EXPECT_EQ(events.confidenceDelta, -2);  // -6/2 halved, then cushioned to a minimum of -2

  // A comfortable shortfall takes the full penalty.
  CareerSave save2;
  save2.reputation = 50;
  save2.boardConfidence = 50;
  save2.fanBase = 20;
  save2.boardObjectives.clear();
  save2.boardObjectives.push_back(
      {OwnerObjectiveType::GROW_FANBASE, "Grow the fan base to at least 60k", false, 3, -6});
  RecordingEvents events2;
  blunted::CareerBoard::EvaluateBoardObjectives(save2, events2);
  EXPECT_EQ(events2.confidenceDelta, -6);
}

}  // namespace
