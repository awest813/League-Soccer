// Regressions for defects found in the October 2026 career audit
// (docs/gap-list.md). Each test pins a fixed behaviour so it cannot silently
// return.

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "data/careerdata.hpp"
#include "menu/career/career_board.hpp"
#include "menu/career/career_common.hpp"
#include "menu/career/career_database.hpp"
#include "menu/career/career_finance.hpp"
#include "menu/career/career_persistence.hpp"
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

// --- Second career pass ---------------------------------------------------

class RecordingEvents : public CareerEvents {
public:
  void AddEvent(const std::string&, const std::string& description, int, bool) override {
    descriptions.push_back(description);
  }
  void ModifyBoardConfidence(int) override {}
  std::vector<std::string> descriptions;
};

std::filesystem::path FreshDir(const std::string& name) {
  namespace fs = std::filesystem;
  const fs::path dir = fs::temp_directory_path() / ("league_soccer_" + name);
  fs::remove_all(dir);
  fs::create_directories(dir);
  return dir;
}

// Bids were saved without the market they were placed in, so an accepted bid
// could never complete after Continue/Load.
TEST(CareerAuditRegression, AcceptedBidCompletesAfterLoad) {
  const auto dir = FreshDir("bid_after_load");
  blunted::CareerDatabase& db = blunted::CareerDatabase::GetInstance();
  db.Initialize(dir.string());
  ASSERT_TRUE(db.CreateNewCareer("Bidders", "owner_gm", "Boss"));
  CareerSave* save = db.GetActiveSave();
  ASSERT_NE(save, nullptr);
  save->transferBudget = 2000000000LL;
  save->wageBudget = 100000000;
  db.PopulateTransferMarket();
  const auto targets = db.GetTransferTargets();
  ASSERT_FALSE(targets.empty());
  const TransferTarget target = targets.front();
  db.PlaceBid(target.name, target.askingPrice, static_cast<int>(target.wage), 3);
  db.ProcessPendingBids();
  ASSERT_TRUE(db.SaveCareerData());

  ASSERT_TRUE(db.LoadCareerSlot(0));
  EXPECT_EQ(db.GetTransferTargets().size(), targets.size());
  ASSERT_TRUE(db.CompleteTransfer(target.name));
  save = db.GetActiveSave();
  const bool signed_ =
      std::any_of(save->roster.begin(), save->roster.end(),
                  [&](const PlayerCareerState& player) { return player.name == target.name; });
  EXPECT_TRUE(signed_);
  std::filesystem::remove_all(dir);
}

// Lifecycle commits always wrote slot 0, so playing a career loaded from
// slot 3 overwrote the career in slot 0.
TEST(CareerAuditRegression, CommitsGoToTheActiveSlot) {
  const auto dir = FreshDir("active_slot");
  blunted::CareerDatabase& db = blunted::CareerDatabase::GetInstance();
  db.Initialize(dir.string());
  ASSERT_TRUE(db.CreateNewCareer("Slot Three", "manager", "Boss"));
  ASSERT_TRUE(db.SaveCareerSlot(3));
  EXPECT_EQ(db.GetActiveSlot(), 3);
  ASSERT_TRUE(db.LoadCareerSlot(3));
  ASSERT_TRUE(db.CompleteFixture(1, 1, true, 1, 2, "Rivals", 2, 0));

  blunted::CareerPersistence::CareerSaveSummary slot0, slot3;
  ASSERT_TRUE(db.GetSlotSummary(0, slot0));
  ASSERT_TRUE(db.GetSlotSummary(3, slot3));
  EXPECT_EQ(slot0.week, 1);
  EXPECT_EQ(slot3.week, 2);
  std::filesystem::remove_all(dir);
}

TEST(CareerAuditRegression, LeagueSettingsSurviveSaveAndLoad) {
  const auto dir = FreshDir("league_settings");
  blunted::CareerDatabase& db = blunted::CareerDatabase::GetInstance();
  db.Initialize(dir.string());
  ASSERT_TRUE(db.CreateNewCareer("Expanders", "owner_gm", "Boss"));
  CareerSave* save = db.GetActiveSave();
  save->leagueSettings.enabled = false;
  save->leagueSettings.divisions.push_back({"Premier Division", 20, 3, 3, 0});
  save->leagueSettings.divisions.push_back({"Second Division", 22, 2, 4, 4});
  save->customLeague.leagueName = "My League";
  save->customLeague.numDivisions = 2;
  save->customLeague.cupCompetition = true;
  save->customLeague.cupName = "My Cup";
  ASSERT_TRUE(db.SaveCareerData());

  ASSERT_TRUE(db.LoadCareerSlot(0));
  save = db.GetActiveSave();
  EXPECT_FALSE(save->leagueSettings.enabled);
  ASSERT_EQ(save->leagueSettings.divisions.size(), 2u);
  EXPECT_EQ(save->leagueSettings.divisions[1].name, "Second Division");
  EXPECT_EQ(save->leagueSettings.divisions[1].numTeams, 22);
  EXPECT_EQ(save->leagueSettings.divisions[1].playOffSpots, 4);
  EXPECT_EQ(save->customLeague.leagueName, "My League");
  EXPECT_EQ(save->customLeague.numDivisions, 2);
  EXPECT_TRUE(save->customLeague.cupCompetition);
  EXPECT_EQ(save->customLeague.cupName, "My Cup");
  std::filesystem::remove_all(dir);
}

// matchesPlayed counted goals: only scorers gained an appearance, once per goal.
TEST(CareerAuditRegression, AppearancesCountMatchesNotGoals) {
  NullEvents events;
  CareerSave save;
  for (int i = 0; i < 16; ++i)
    save.roster.push_back(MakePlayer("Player " + std::to_string(i), 25, 3));
  save.roster[0].preferredPosition = "GK";
  save.roster[5].ovr = 80;

  blunted::CareerSim::ApplyMatchResult(save, events, 2, 0, "Rivals", {"Player 5", "Player 5"});
  int appeared = 0;
  for (const auto& player : save.roster)
    appeared += player.matchesPlayed;
  EXPECT_EQ(appeared, 11);
  EXPECT_EQ(save.roster[0].matchesPlayed, 1);  // the keeper plays
  EXPECT_EQ(save.roster[5].matchesPlayed, 1);
  EXPECT_EQ(save.roster[5].careerGoals, 2);
}

// 3D matches passed no scorers, so played fixtures recorded no personal stats.
TEST(CareerAuditRegression, PlayedMatchCreditsScorersByDatabaseID) {
  const auto dir = FreshDir("played_scorers");
  blunted::CareerDatabase& db = blunted::CareerDatabase::GetInstance();
  db.Initialize(dir.string());
  ASSERT_TRUE(db.CreateNewCareer("Strikers", "manager", "Boss"));
  CareerSave* save = db.GetActiveSave();
  save->roster.clear();
  for (int i = 0; i < 12; ++i) {
    save->roster.push_back(MakePlayer("Player " + std::to_string(i), 25, 3));
    save->roster.back().databaseID = 100 + i;
  }
  ASSERT_TRUE(db.SetPendingFixture(false, 1, 2, "Hosts"));
  // Away fixture: the user is MatchData team 1.
  ASSERT_TRUE(db.ConsumePlayedFixture(1, 2, {200}, {107, 107}));
  save = db.GetActiveSave();
  EXPECT_EQ(save->roster[7].careerGoals, 2);
  EXPECT_EQ(save->roster[7].matchesPlayed, 1);
  EXPECT_EQ(save->seasonWins, 1);
  std::filesystem::remove_all(dir);
}

// AI records were regenerated from a new seed every week, so a club's points
// could go down, and the default database showed a two-club table.
TEST(CareerAuditRegression, StandingsAccumulateAcrossRounds) {
  NullEvents events;
  CareerSave save;
  save.name = "Accumulators";
  for (int i = 0; i < 14; ++i)
    save.roster.push_back(MakePlayer("Player " + std::to_string(i), 25, 3));

  std::vector<int> previousPoints;
  for (int round = 1; round <= 12; ++round) {
    blunted::CareerSim::ApplyMatchResult(save, events, round % 3, 1, "Rivals");
    const auto table = blunted::CareerSim::GenerateLeagueStandings(save, {{1, "A"}, {2, "B"}});
    ASSERT_EQ(table.size(), 20u);
    int wins = 0, losses = 0;
    std::vector<int> points;
    for (const auto& row : table) {
      EXPECT_EQ(row.played, round) << row.name;
      wins += row.wins;
      losses += row.losses;
    }
    EXPECT_EQ(wins, losses);  // every win is someone's loss
    for (const auto& entry : save.leagueTable)
      points.push_back(entry.wins * 3 + entry.draws);
    if (!previousPoints.empty()) {
      for (size_t i = 0; i < points.size(); ++i)
        EXPECT_GE(points[i], previousPoints[i]);
    }
    previousPoints = points;
  }
  EXPECT_EQ(save.seasonForm.size(), 5u);
}

// Objectives were judged against targets recomputed from the reputation after
// the season's changes, and regenerating them wiped the outcome.
TEST(CareerAuditRegression, BoardObjectivesKeepTheirTargetAndReportTheOutcome) {
  CareerSave save;
  save.reputation = 50;
  blunted::CareerBoard::GenerateBoardObjectives(save);
  ASSERT_FALSE(save.boardObjectives.empty());
  ASSERT_EQ(save.boardObjectives.front().type, OwnerObjectiveType::PROMOTION);
  EXPECT_EQ(save.boardObjectives.front().target, 10);

  SeasonRecord last;
  last.leaguePosition = 8;  // top half, but not top six
  save.history.push_back(last);
  save.reputation = 75;  // a title-chasing club would need a top-six finish
  RecordingEvents events;
  blunted::CareerBoard::EvaluateBoardObjectives(save, events);
  EXPECT_TRUE(save.boardObjectives.front().completed);
  const bool reported =
      std::any_of(events.descriptions.begin(), events.descriptions.end(),
                  [](const std::string& text) { return text.rfind("Objective met: ", 0) == 0; });
  EXPECT_TRUE(reported);
}

}  // namespace
