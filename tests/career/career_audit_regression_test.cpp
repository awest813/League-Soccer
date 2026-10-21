// Regressions for defects found in the October 2026 career audit
// (docs/gap-list.md). Each test pins a fixed behaviour so it cannot silently
// return.

#include <filesystem>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "data/careerdata.hpp"
#include "menu/career/career_common.hpp"
#include "menu/career/career_database.hpp"
#include "menu/career/career_finance.hpp"
#include "menu/career/career_sim.hpp"
#include "menu/career/career_sponsors.hpp"
#include "menu/career/career_transfers.hpp"

namespace {

using blunted::CareerCommon::CareerEvents;
using blunted::CareerCommon::PositionFromDbRole;

class NullEvents : public CareerEvents {
public:
  void AddEvent(const std::string&, const std::string&, int, bool) override {}
  void ModifyBoardConfidence(int) override {}
};

PlayerCareerState MakePlayer(const std::string& name, int age, int contractYears) {
  PlayerCareerState player;
  player.name = name;
  player.position = "CM";
  player.preferredPosition = "CM";
  player.age = age;
  player.ovr = 65;
  player.pot = 70;
  player.wage = 10000;
  player.contract.yearsRemaining = contractYears;
  return player;
}

// The default database stores Football Manager role notation. Before the fix
// every outfield player except "DM" became "CM", flattening goal-scorer
// weights and position-focused training.
TEST(CareerAuditRegression, DatabaseRolesMapToCareerPositions) {
  EXPECT_EQ(PositionFromDbRole("GK"), "GK");
  EXPECT_EQ(PositionFromDbRole("D C"), "CB");
  EXPECT_EQ(PositionFromDbRole("SW, D C"), "CB");
  EXPECT_EQ(PositionFromDbRole("D RL"), "RB");
  EXPECT_EQ(PositionFromDbRole("D LC"), "CB");
  EXPECT_EQ(PositionFromDbRole("D/WB L, DM"), "LB");
  EXPECT_EQ(PositionFromDbRole("D/WB/AM R"), "RB");
  EXPECT_EQ(PositionFromDbRole("DM, AM C"), "DM");
  EXPECT_EQ(PositionFromDbRole("M C"), "CM");
  EXPECT_EQ(PositionFromDbRole("AM RLC, F C"), "AM");
  EXPECT_EQ(PositionFromDbRole("AM L, ST"), "LM");
  EXPECT_EQ(PositionFromDbRole("AM/F C"), "AM");
  EXPECT_EQ(PositionFromDbRole("ST"), "CF");
  EXPECT_EQ(PositionFromDbRole("CB"), "CB");
  EXPECT_EQ(PositionFromDbRole(""), "CM");
  EXPECT_EQ(PositionFromDbRole("???"), "CM");
}

// Toggling the ticket price between two values used to raise fan base and
// satisfaction on every click because each call was measured against 40.
TEST(CareerAuditRegression, TicketPriceTogglingIsNeutral) {
  CareerSave save;
  save.fanBase = 50;
  save.stadium.fanSatisfaction = 60;
  for (int i = 0; i < 20; ++i) {
    blunted::CareerFinance::SetTicketPrice(save, 30);
    blunted::CareerFinance::SetTicketPrice(save, 40);
  }
  EXPECT_EQ(save.finances.ticketPrice, 40);
  EXPECT_EQ(save.fanBase, 50);
  EXPECT_EQ(save.stadium.fanSatisfaction, 60);

  // A single cut still pleases fans; a rise still costs them.
  blunted::CareerFinance::SetTicketPrice(save, 20);
  EXPECT_GT(save.fanBase, 50);
  blunted::CareerFinance::SetTicketPrice(save, 60);
  EXPECT_LT(save.fanBase, 50);
}

// Sponsor revenue was credited at signing and again at season end, and an
// active sponsor was re-offered so identical deals could stack.
TEST(CareerAuditRegression, SponsorRevenueIsPaidOnceAndDealsDoNotStack) {
  NullEvents events;
  CareerSave save;
  save.reputation = 100;
  blunted::CareerSponsors::GenerateSponsorOffers(save);
  ASSERT_FALSE(save.availableSponsorOffers.empty());
  const std::string sponsorName = save.availableSponsorOffers.front().sponsorName;
  const long long netWorthBefore = save.finances.netWorth;

  ASSERT_TRUE(blunted::CareerSponsors::AcceptSponsorDeal(save, events, 0));
  EXPECT_EQ(save.finances.netWorth, netWorthBefore);

  blunted::CareerSponsors::GenerateSponsorOffers(save);
  for (const auto& offer : save.availableSponsorOffers)
    EXPECT_NE(offer.sponsorName, sponsorName);
}

// Transfer spending accumulated forever and was charged against every later
// season's profit.
TEST(CareerAuditRegression, TransferTotalsAreChargedForOneSeasonOnly) {
  CareerSave save;
  save.finances.transferSpending = 20000000;
  save.finances.transferIncome = 5000000;
  blunted::CareerFinance::ProcessSeasonFinances(save);
  const long long firstSeasonExpenses = save.finances.totalExpenses;
  EXPECT_EQ(save.finances.transferSpending, 0);
  EXPECT_EQ(save.finances.transferIncome, 0);

  blunted::CareerFinance::ProcessSeasonFinances(save);
  EXPECT_EQ(save.finances.totalExpenses, firstSeasonExpenses - 20000000);
}

// Free agents carry no contract; signing one used to leave yearsRemaining at 0
// so they walked out again at the next rollover.
TEST(CareerAuditRegression, SignedFreeAgentReceivesContract) {
  NullEvents events;
  CareerSave save;
  save.wageBudget = 1000000;
  save.freeAgents.push_back(MakePlayer("Loose Agent", 27, 0));
  blunted::CareerTransfers::RecruitFreeAgent(save, events, "Loose Agent");
  ASSERT_EQ(save.roster.size(), 1u);
  EXPECT_GE(save.roster.front().contract.yearsRemaining, 2);
  EXPECT_EQ(save.roster.front().contract.wage, save.roster.front().wage);
}

// Academy prospects and free agents never aged; departing players must age
// exactly once and release their wage.
TEST(CareerAuditRegression, SeasonRolloverAgesEveryoneOnceAndRefundsExpiredWages) {
  NullEvents events;
  CareerSave save;
  for (int i = 0; i < 15; ++i)
    save.roster.push_back(MakePlayer("Squad " + std::to_string(i), 25, 3));
  save.roster.push_back(MakePlayer("Expiring", 30, 1));
  save.youthAcademy.push_back(MakePlayer("Prospect", 16, 0));
  save.freeAgents.push_back(MakePlayer("Veteran", 33, 0));
  save.wageBudget = 0;

  std::vector<TransferBid> bids;
  std::vector<TransferTarget> targets;
  blunted::CareerSim::AdvanceSeason(save, events, bids, targets);

  EXPECT_EQ(save.youthAcademy.front().age, 17);
  bool foundVeteran = false;
  long long expiringWage = -1;
  for (const auto& agent : save.freeAgents) {
    if (agent.name == "Veteran") {
      foundVeteran = true;
      EXPECT_EQ(agent.age, 34);
    } else if (agent.name == "Expiring") {
      expiringWage = agent.wage;
      EXPECT_EQ(agent.age, 31);
    }
  }
  EXPECT_TRUE(foundVeteran);
  ASSERT_GT(expiringWage, 0);
  EXPECT_EQ(save.wageBudget, expiringWage);
}

// Individual training matched databaseID == controlledEntityID, so a save
// without a controlled identity trained every player whose ID was 0.
TEST(CareerAuditRegression, IndividualTrainingRequiresControlledIdentity) {
  namespace fs = std::filesystem;
  const fs::path dir = fs::temp_directory_path() / "league_soccer_individual_training_identity";
  fs::remove_all(dir);
  fs::create_directories(dir);

  blunted::CareerDatabase& db = blunted::CareerDatabase::GetInstance();
  db.Initialize(dir.string());
  ASSERT_TRUE(db.CreateNewCareer("Rovers", "coach", "Boss"));
  CareerSave* save = db.GetActiveSave();
  ASSERT_NE(save, nullptr);
  save->roster.clear();
  save->roster.push_back(MakePlayer("Signing A", 24, 2));
  save->roster.push_back(MakePlayer("Signing B", 24, 2));
  save->controlledEntityID = 0;
  save->trainingPoints = 5;

  EXPECT_FALSE(db.TrainFocus("Individual"));
  for (const auto& player : save->roster)
    EXPECT_EQ(player.ovr, 65);
  EXPECT_EQ(save->trainingPoints, 5);

  fs::remove_all(dir);
}

}  // namespace
