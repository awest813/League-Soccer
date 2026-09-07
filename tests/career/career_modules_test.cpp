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
  EXPECT_EQ(TR("career_menu_coach"), "Head Coach\nMatchday leadership");
  EXPECT_EQ(TRF("career_progress_line", {"2", "38", "1", "0", "1", "3", "2"}),
            "Week 2/38 | W 1  D 0  L 1 | GF 3  GA 2");
  EXPECT_EQ(TR("career_hub_club_snapshot"), "Club Snapshot");
  EXPECT_EQ(TRF("career_release_confirm", {"Alex Morgan"}),
            "Release Alex Morgan? This cannot be undone.");
  EXPECT_EQ(TRF("career_owner_fin_body", {"EUR 1,000", "EUR 500", "Stable", "25"}),
            "Net Worth: EUR 1,000\nTransfer Budget: EUR 500\nFinancial Health: Stable\nTicket "
            "Price: EUR 25");

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

namespace {
PlayerCareerState DevelopmentPlayer() {
  PlayerCareerState player;
  player.name = "Young prospect";
  player.age = 19;
  player.ovr = 60;
  player.pot = 80;
  player.fitness = 80;
  return player;
}

TEST(CareerDevelopmentTest, PlansTradeDevelopmentForFitnessWithoutSpendingPoints) {
  RecordingEvents events;
  CareerSave balanced, intensive, recovery;
  balanced.roster = intensive.roster = recovery.roster = {DevelopmentPlayer()};
  intensive.trainingPlan = CareerTrainingPlan::DEVELOPMENT;
  recovery.trainingPlan = CareerTrainingPlan::RECOVERY;
  for (auto* save : {&balanced, &intensive, &recovery})
    blunted::CareerTraining::DevelopAfterMatch(*save, events);
  EXPECT_EQ(balanced.roster[0].developmentPoints, 5);
  EXPECT_EQ(balanced.roster[0].fitness, 82);
  EXPECT_EQ(intensive.roster[0].developmentPoints, 9);
  EXPECT_EQ(intensive.roster[0].fitness, 76);
  EXPECT_EQ(recovery.roster[0].developmentPoints, 0);
  EXPECT_EQ(recovery.roster[0].fitness, 90);
  EXPECT_EQ(intensive.trainingPoints, 10);
}

TEST(CareerDevelopmentTest, ProgressCarriesUntilPotentialAndAcademySurvivesPromotion) {
  RecordingEvents events;
  CareerSave save;
  auto player = DevelopmentPlayer();
  player.developmentPoints = 98;
  save.youthAcademy.push_back(player);
  save.trainingPlan = CareerTrainingPlan::RECOVERY;
  blunted::CareerTraining::DevelopAfterMatch(save, events);
  ASSERT_EQ(save.youthAcademy[0].ovr, 61);
  EXPECT_EQ(save.youthAcademy[0].developmentPoints, 3);
  blunted::CareerTraining::PromoteYouthPlayer(save, events, player.name);
  ASSERT_EQ(save.roster.size(), 1u);
  EXPECT_EQ(save.roster[0].developmentPoints, 3);
  save.roster[0].pot = 62;
  save.roster[0].developmentPoints = 99;
  save.trainingPlan = CareerTrainingPlan::BALANCED;
  for (int i = 0; i < 40; ++i) blunted::CareerTraining::DevelopAfterMatch(save, events);
  EXPECT_EQ(save.roster[0].ovr, 62);
  EXPECT_EQ(save.roster[0].developmentPoints, 0);
}

TEST(CareerDevelopmentTest, TiredAndInjuredPlayersRestBeforeDeveloping) {
  RecordingEvents events;
  CareerSave save;
  save.trainingPlan = CareerTrainingPlan::DEVELOPMENT;
  save.roster = {DevelopmentPlayer(), DevelopmentPlayer()};
  save.roster[0].fitness = 59;
  save.roster[1].injury = InjuryStatus::OUT_SHORT_TERM;
  blunted::CareerTraining::DevelopAfterMatch(save, events);
  EXPECT_EQ(save.roster[0].fitness, 69);
  EXPECT_EQ(save.roster[1].fitness, 90);
  EXPECT_EQ(save.roster[0].developmentPoints, 0);
  EXPECT_EQ(save.roster[1].developmentPoints, 0);
  blunted::CareerTraining::DevelopAfterMatch(save, events);
  EXPECT_EQ(save.roster[0].developmentPoints, 9);
  EXPECT_EQ(save.roster[1].developmentPoints, 0);
}

TEST(CareerDevelopmentTest, CompletedFacilitiesAndActiveCoachesSupportDevelopment) {
  RecordingEvents events;
  CareerSave save;
  save.roster = {DevelopmentPlayer()};
  StadiumUpgrade facility;
  facility.name = "Training Complex";
  facility.seasonsRemaining = 1;
  save.stadium.upgrades.push_back(facility);
  save.staff.emplace_back("Expired", "Assistant Coach", 90, 100, 0);
  blunted::CareerTraining::DevelopAfterMatch(save, events);
  EXPECT_EQ(save.roster[0].developmentPoints, 5);
  save.stadium.upgrades[0].seasonsRemaining = 0;
  save.staff.emplace_back("Coach", "Assistant Coach", 85, 100, 2);
  save.staff.emplace_back("Duplicate", "Youth Coach", 90, 100, 2);
  blunted::CareerTraining::DevelopAfterMatch(save, events);
  EXPECT_EQ(save.roster[0].developmentPoints, 13);
}

TEST(CareerDevelopmentTest, VeteransDevelopMoreSlowly) {
  RecordingEvents events;
  CareerSave save;
  save.roster = {DevelopmentPlayer(), DevelopmentPlayer()};
  save.roster[1].age = 32;
  blunted::CareerTraining::DevelopAfterMatch(save, events);
  EXPECT_GT(save.roster[0].developmentPoints, save.roster[1].developmentPoints);
}

TEST(CareerDevelopmentTest, PlayedAndSimulatedResultsApplyOneDevelopmentStep) {
  RecordingEvents events;
  CareerSave simulated, played;
  simulated.roster = played.roster = {DevelopmentPlayer()};
  blunted::CareerSim::ApplyMatchResult(simulated, events, 1, 0, "Opponent");
  blunted::CareerSim::Process3DMatchResult(played, events, 1, 0);
  EXPECT_EQ(simulated.roster[0].developmentPoints, 5);
  EXPECT_EQ(played.roster[0].developmentPoints, 5);
}

TEST(CareerDevelopmentTest, LegacyAndInvalidPlayerProgressLoadSafely) {
  const std::string legacy = "Prospect|ST|19|60|80|100000|500|50|50|90|0|0|0";
  EXPECT_EQ(blunted::CareerCommon::PlayerFromRecord(legacy).developmentPoints, 0);
  EXPECT_EQ(blunted::CareerCommon::PlayerFromRecord(legacy + "|oops").developmentPoints, 0);
  EXPECT_EQ(blunted::CareerCommon::PlayerFromRecord(legacy + "|-5").developmentPoints, 0);
  EXPECT_EQ(blunted::CareerCommon::PlayerFromRecord(legacy + "|500").developmentPoints, 99);
  auto player = DevelopmentPlayer();
  player.developmentPoints = 57;
  player.injury = InjuryStatus::OUT_LONG_TERM;
  auto loaded = blunted::CareerCommon::PlayerFromRecord(blunted::CareerCommon::PlayerToRecord(player));
  EXPECT_EQ(loaded.developmentPoints, 57);
  EXPECT_EQ(loaded.injury, InjuryStatus::OUT_LONG_TERM);
}
}  // namespace

TEST(CareerDevelopmentTest, FatiguedSquadsPerformWorseAcrossIdenticalSimulationSeeds) {
  CareerSave fresh, tired;
  fresh.roster = tired.roster = {DevelopmentPlayer()};
  fresh.roster[0].fitness = 100;
  tired.roster[0].fitness = 40;
  int freshDifference = 0, tiredDifference = 0;
  for (unsigned int seed = 0; seed < 300; ++seed) {
    blunted::CareerCommon::SeedRng(seed);
    auto fitResult = blunted::CareerSim::SimulateMatchResult(fresh, "Opponent", "1", true);
    blunted::CareerCommon::SeedRng(seed);
    auto tiredResult = blunted::CareerSim::SimulateMatchResult(tired, "Opponent", "1", true);
    freshDifference += fitResult.homeGoals - fitResult.awayGoals;
    tiredDifference += tiredResult.homeGoals - tiredResult.awayGoals;
  }
  EXPECT_GT(freshDifference, tiredDifference);
}
