#include <filesystem>
#include <fstream>
#include <string>

#include <gtest/gtest.h>

#include "menu/career/career_database.hpp"
#include "menu/career/career_persistence.hpp"
#include "menu/career/career_sim.hpp"

namespace {

using blunted::CareerDatabase;
using blunted::CareerPendingFixture;
using blunted::CareerSave;
using blunted::FixtureResult;
using blunted::PlayerCareerState;
using blunted::CareerPersistence::CareerSaveSummary;
using blunted::CareerSim::CareerLeagueTableRow;
using blunted::CareerSim::CareerTopScorer;

namespace fs = std::filesystem;

std::string UniqueTempDir(const std::string& label) {
  const char* testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
  return (fs::temp_directory_path() / ("league_soccer_" + std::string(testName) + "_" + label))
      .string();
}

TEST(CareerFixtureTest, PendingFixtureLifecycle) {
  fs::path dir = UniqueTempDir("lifecycle");
  fs::remove_all(dir);
  fs::create_directories(dir);

  CareerDatabase& db = CareerDatabase::GetInstance();
  db.Initialize(dir.string());
  ASSERT_TRUE(db.CreateNewCareer("Lifecycle FC", "manager", "Test Coach"));

  // Initially, no fixture is pending.
  EXPECT_FALSE(db.HasPendingFixture());

  // Arm a home match fixture.
  db.SetPendingFixture(true, 1, 204, "City Rivals");
  EXPECT_TRUE(db.HasPendingFixture());
  {
    const CareerPendingFixture& fix = db.GetPendingFixture();
    EXPECT_TRUE(fix.hasFixture);
    EXPECT_TRUE(fix.isHome);
    EXPECT_EQ(fix.userTeamDBID, 1);
    EXPECT_EQ(fix.opponentTeamDBID, 204);
    EXPECT_EQ(fix.opponentName, "City Rivals");
  }

  // Clearing manually works as expected.
  db.ClearPendingFixture();
  EXPECT_FALSE(db.HasPendingFixture());
}

TEST(CareerFixtureTest, ConsumeWithoutPendingFixtureReturnsFalse) {
  fs::path dir = UniqueTempDir("no_fixture");
  fs::remove_all(dir);
  fs::create_directories(dir);

  CareerDatabase& db = CareerDatabase::GetInstance();
  db.Initialize(dir.string());
  ASSERT_TRUE(db.CreateNewCareer("Safe FC", "manager", "Test Coach"));
  db.ClearPendingFixture();

  EXPECT_FALSE(db.HasPendingFixture());
  // Non-career match endings must not alter career stats.
  EXPECT_FALSE(db.ConsumePlayedFixture(2, 0));
}

TEST(CareerFixtureTest, ConsumePlayedFixtureHomeWinAndLoss) {
  fs::path dir = UniqueTempDir("home_matches");
  fs::remove_all(dir);
  fs::create_directories(dir);

  CareerDatabase& db = CareerDatabase::GetInstance();
  db.Initialize(dir.string());
  ASSERT_TRUE(db.CreateNewCareer("Home FC", "manager", "Manager H"));

  CareerSave* save = db.GetActiveSave();
  ASSERT_NE(save, nullptr);
  save->season.currentWeek = 1;
  save->seasonWins = 0;
  save->seasonDraws = 0;
  save->seasonLosses = 0;
  save->seasonGoalsFor = 0;
  save->seasonGoalsAgainst = 0;

  // Match 1: Home Win (3-1)
  // Engine: team0 (Home / User) = 3, team1 (Away / Opponent) = 1
  db.SetPendingFixture(true, 1, 101, "Away Town");
  EXPECT_TRUE(db.HasPendingFixture());
  EXPECT_TRUE(db.ConsumePlayedFixture(3, 1));
  EXPECT_FALSE(db.HasPendingFixture());

  EXPECT_EQ(save->seasonWins, 1);
  EXPECT_EQ(save->seasonDraws, 0);
  EXPECT_EQ(save->seasonLosses, 0);
  EXPECT_EQ(save->seasonGoalsFor, 3);
  EXPECT_EQ(save->seasonGoalsAgainst, 1);
  EXPECT_EQ(save->season.currentWeek, 2);

  // Match 2: Home Loss (0-2)
  // Engine: team0 (Home / User) = 0, team1 (Away / Opponent) = 2
  db.SetPendingFixture(true, 1, 102, "Top Club");
  EXPECT_TRUE(db.HasPendingFixture());
  EXPECT_TRUE(db.ConsumePlayedFixture(0, 2));
  EXPECT_FALSE(db.HasPendingFixture());

  EXPECT_EQ(save->seasonWins, 1);
  EXPECT_EQ(save->seasonDraws, 0);
  EXPECT_EQ(save->seasonLosses, 1);
  EXPECT_EQ(save->seasonGoalsFor, 3);
  EXPECT_EQ(save->seasonGoalsAgainst, 3);
  EXPECT_EQ(save->season.currentWeek, 3);
}

TEST(CareerFixtureTest, ConsumePlayedFixtureAwayWinAndLossInversion) {
  fs::path dir = UniqueTempDir("away_matches");
  fs::remove_all(dir);
  fs::create_directories(dir);

  CareerDatabase& db = CareerDatabase::GetInstance();
  db.Initialize(dir.string());
  ASSERT_TRUE(db.CreateNewCareer("Away FC", "manager", "Manager A"));

  CareerSave* save = db.GetActiveSave();
  ASSERT_NE(save, nullptr);
  save->season.currentWeek = 1;
  save->seasonWins = 0;
  save->seasonDraws = 0;
  save->seasonLosses = 0;
  save->seasonGoalsFor = 0;
  save->seasonGoalsAgainst = 0;

  // Match 1: Away Win (1-2)
  // Engine: team0 (Home / Opponent) = 1, team1 (Away / User) = 2
  // User wins 2-1 on the road!
  db.SetPendingFixture(false, 1, 103, "Host Athletic");
  EXPECT_TRUE(db.HasPendingFixture());
  EXPECT_TRUE(db.ConsumePlayedFixture(1, 2));
  EXPECT_FALSE(db.HasPendingFixture());

  EXPECT_EQ(save->seasonWins, 1);
  EXPECT_EQ(save->seasonDraws, 0);
  EXPECT_EQ(save->seasonLosses, 0);
  EXPECT_EQ(save->seasonGoalsFor, 2);
  EXPECT_EQ(save->seasonGoalsAgainst, 1);
  EXPECT_EQ(save->season.currentWeek, 2);

  // Match 2: Away Loss (3-0)
  // Engine: team0 (Home / Opponent) = 3, team1 (Away / User) = 0
  db.SetPendingFixture(false, 1, 104, "Giant City");
  EXPECT_TRUE(db.HasPendingFixture());
  EXPECT_TRUE(db.ConsumePlayedFixture(3, 0));
  EXPECT_FALSE(db.HasPendingFixture());

  EXPECT_EQ(save->seasonWins, 1);
  EXPECT_EQ(save->seasonDraws, 0);
  EXPECT_EQ(save->seasonLosses, 1);
  EXPECT_EQ(save->seasonGoalsFor, 2);
  EXPECT_EQ(save->seasonGoalsAgainst, 4);
  EXPECT_EQ(save->season.currentWeek, 3);
}

TEST(CareerFixtureTest, MultiSlotSaveLoadDelete) {
  fs::path dir = UniqueTempDir("slots");
  fs::remove_all(dir);
  fs::create_directories(dir);

  CareerDatabase& db = CareerDatabase::GetInstance();
  db.Initialize(dir.string());
  ASSERT_TRUE(db.CreateNewCareer("Slot FC", "manager", "Slot Master"));

  CareerSave* save = db.GetActiveSave();
  ASSERT_NE(save, nullptr);
  save->boardConfidence = 88;
  save->transferBudget = 12500000;
  save->season.currentSeason = 2;
  save->season.currentWeek = 14;

  // Deleting an empty slot returns false.
  EXPECT_FALSE(db.DeleteCareerSlot(5));

  // Save to slot 1 and slot 4.
  EXPECT_TRUE(db.SaveCareerSlot(1));
  EXPECT_TRUE(db.SaveCareerSlot(4));

  // Slot 1 summary check
  CareerSaveSummary s1;
  EXPECT_TRUE(db.GetSlotSummary(1, s1));
  EXPECT_TRUE(s1.isValid);
  EXPECT_EQ(s1.clubName, "Slot FC");
  EXPECT_EQ(s1.season, 2);
  EXPECT_EQ(s1.week, 14);
  EXPECT_EQ(s1.boardConfidence, 88);
  EXPECT_EQ(s1.transferBudget, 12500000);

  // Slot 2 summary should report invalid / empty
  CareerSaveSummary s2;
  EXPECT_FALSE(db.GetSlotSummary(2, s2) && s2.isValid);

  // Slot 4 summary check
  CareerSaveSummary s4;
  EXPECT_TRUE(db.GetSlotSummary(4, s4));
  EXPECT_TRUE(s4.isValid);
  EXPECT_EQ(s4.clubName, "Slot FC");

  // Modify active save name and reload slot 1
  db.GetActiveSave()->name = "Altered Name";
  EXPECT_EQ(db.GetActiveSave()->name, "Altered Name");
  EXPECT_TRUE(db.LoadCareerSlot(1));
  EXPECT_EQ(db.GetActiveSave()->name, "Slot FC");

  // Delete slot 1
  EXPECT_TRUE(db.DeleteCareerSlot(1));
  EXPECT_FALSE(db.GetSlotSummary(1, s1) && s1.isValid);

  // Slot 4 remains intact
  EXPECT_TRUE(db.GetSlotSummary(4, s4) && s4.isValid);

  // Delete slot 4
  EXPECT_TRUE(db.DeleteCareerSlot(4));
  EXPECT_FALSE(db.GetSlotSummary(4, s4) && s4.isValid);
}

TEST(CareerSeasonTest, GenerateLeagueStandingsAndTopScorers) {
  fs::path dir = UniqueTempDir("standings_test");
  fs::remove_all(dir);
  fs::create_directories(dir);

  CareerDatabase& db = CareerDatabase::GetInstance();
  db.Initialize(dir.string());
  ASSERT_TRUE(db.CreateNewCareer("Apex United", "manager", "Apex Boss"));

  CareerSave* save = db.GetActiveSave();
  ASSERT_NE(save, nullptr);
  save->seasonWins = 10;
  save->seasonDraws = 2;
  save->seasonLosses = 1;
  save->seasonGoalsFor = 25;
  save->seasonGoalsAgainst = 8;
  save->season.currentWeek = 14;

  // Add players with goals
  PlayerCareerState p1;
  p1.name = "Marcus Sharp";
  p1.careerGoals = 12;
  save->roster.push_back(p1);

  PlayerCareerState p2;
  p2.name = "David Strike";
  p2.careerGoals = 7;
  save->roster.push_back(p2);

  std::vector<CareerLeagueTableRow> standings = db.GetLeagueStandings();
  EXPECT_EQ(standings.size(), 20u);

  // Check sorting: points descending, then GD descending
  for (size_t i = 1; i < standings.size(); ++i) {
    if (standings[i - 1].points == standings[i].points) {
      EXPECT_GE(standings[i - 1].goalDiff, standings[i].goalDiff);
    } else {
      EXPECT_GT(standings[i - 1].points, standings[i].points);
    }
  }

  // Find user team
  bool foundUser = false;
  for (const auto& row : standings) {
    if (row.isUserTeam) {
      foundUser = true;
      EXPECT_EQ(row.wins, 10);
      EXPECT_EQ(row.draws, 2);
      EXPECT_EQ(row.losses, 1);
      EXPECT_EQ(row.points, 32);
      EXPECT_EQ(row.goalDiff, 17);
      EXPECT_FALSE(row.form.empty());
      break;
    }
  }
  EXPECT_TRUE(foundUser);

  // Check top scorers
  std::vector<CareerTopScorer> scorers = db.GetTopScorers();
  EXPECT_FALSE(scorers.empty());
  EXPECT_LE(scorers.size(), 10u);

  bool foundSharp = false;
  for (const auto& s : scorers) {
    if (s.playerName == "Marcus Sharp") {
      foundSharp = true;
      EXPECT_EQ(s.goals, 12);
      EXPECT_TRUE(s.isUserPlayer);
    }
  }
  EXPECT_TRUE(foundSharp);
}

TEST(CareerSeasonTest, PrizeMoneyAndTitleAward) {
  // 1. Verify prize money ladder
  EXPECT_EQ(blunted::CareerSim::CalculateSeasonPrizeMoney(1), 35000000);
  EXPECT_EQ(blunted::CareerSim::CalculateSeasonPrizeMoney(2), 28000000);
  EXPECT_EQ(blunted::CareerSim::CalculateSeasonPrizeMoney(4), 18000000);
  EXPECT_EQ(blunted::CareerSim::CalculateSeasonPrizeMoney(17), 5000000);
  EXPECT_EQ(blunted::CareerSim::CalculateSeasonPrizeMoney(20), 3000000);

  fs::path dir = UniqueTempDir("prize_title_test");
  fs::remove_all(dir);
  fs::create_directories(dir);

  CareerDatabase& db = CareerDatabase::GetInstance();
  db.Initialize(dir.string());
  ASSERT_TRUE(db.CreateNewCareer("Champion FC", "manager", "Title Boss"));

  CareerSave* save = db.GetActiveSave();
  ASSERT_NE(save, nullptr);
  // Dominant record to guarantee 1st place
  save->seasonWins = 35;
  save->seasonDraws = 2;
  save->seasonLosses = 1;
  save->seasonGoalsFor = 95;
  save->seasonGoalsAgainst = 15;
  save->season.currentWeek = 38;

  int initialTitles = save->legacyStats["titles"];
  long long initialBudget = save->transferBudget;

  db.AdvanceSeason();

  // Validate history entry
  ASSERT_FALSE(save->history.empty());
  const auto& lastHistory = save->history.back();
  EXPECT_EQ(lastHistory.leaguePosition, 1);
  EXPECT_TRUE(lastHistory.wonTitle);

  // Validate title increment & prize money
  EXPECT_EQ(save->legacyStats["titles"], initialTitles + 1);
  EXPECT_EQ(save->transferBudget, initialBudget + 35000000);
}

TEST(CareerSeasonTest, PlayerContractExpiration) {
  fs::path dir = UniqueTempDir("contracts_test");
  fs::remove_all(dir);
  fs::create_directories(dir);

  CareerDatabase& db = CareerDatabase::GetInstance();
  db.Initialize(dir.string());
  ASSERT_TRUE(db.CreateNewCareer("Contract FC", "manager", "Contract Boss"));

  CareerSave* save = db.GetActiveSave();
  ASSERT_NE(save, nullptr);
  save->roster.clear();
  save->freeAgents.clear();

  // Create 16 players: 14 with 3-year contracts, 2 with 1-year contracts
  for (int i = 0; i < 14; ++i) {
    PlayerCareerState p;
    p.name = "Core Player " + std::to_string(i + 1);
    p.contract.yearsRemaining = 3;
    p.wage = 10000;
    save->roster.push_back(p);
  }
  PlayerCareerState exp1;
  exp1.name = "Departing Player 1";
  exp1.contract.yearsRemaining = 1;
  exp1.wage = 12000;
  save->roster.push_back(exp1);

  PlayerCareerState exp2;
  exp2.name = "Departing Player 2";
  exp2.contract.yearsRemaining = 1;
  exp2.wage = 15000;
  save->roster.push_back(exp2);

  EXPECT_EQ(save->roster.size(), 16u);

  // Advancing the season decrements contract years.
  // The two 1-year players hit 0 years and should depart since squad > 14.
  db.AdvanceSeason();

  EXPECT_EQ(save->roster.size(), 14u);
  EXPECT_EQ(save->freeAgents.size(), 2u);
  EXPECT_EQ(save->freeAgents[0].name, "Departing Player 1");
  EXPECT_EQ(save->freeAgents[1].name, "Departing Player 2");

  // Now test emergency extension guard when squad is at minimum (14):
  // Set one remaining player to 1 year
  save->roster[0].contract.yearsRemaining = 1;
  long long prevWage = save->roster[0].wage;

  db.AdvanceSeason();

  // Squad size must NOT fall below 14: emergency extension triggered
  EXPECT_EQ(save->roster.size(), 14u);
  EXPECT_EQ(save->roster[0].contract.yearsRemaining, 1);
  EXPECT_GT(save->roster[0].wage, prevWage);
}

TEST(CareerSeasonTest, FixturePersistenceAcrossSaveLoad) {
  fs::path dir = UniqueTempDir("fixture_persist");
  fs::remove_all(dir);
  fs::create_directories(dir);

  CareerDatabase& db = CareerDatabase::GetInstance();
  db.Initialize(dir.string());
  ASSERT_TRUE(db.CreateNewCareer("Persist FC", "manager", "Persist Boss"));

  CareerSave* save = db.GetActiveSave();
  ASSERT_NE(save, nullptr);
  save->season.fixtures.clear();

  FixtureResult f1{101, 1, 2, 3, 1, true};
  FixtureResult f2{102, 3, 1, 0, 2, true};
  FixtureResult f3{103, 1, 4, 1, 1, true};
  save->season.fixtures = {f1, f2, f3};

  // Save to slot 2
  ASSERT_TRUE(db.SaveCareerSlot(2));

  // Clear fixtures and change name in active memory
  save->season.fixtures.clear();
  save->name = "Empty Memory FC";

  // Reload slot 2
  ASSERT_TRUE(db.LoadCareerSlot(2));
  CareerSave* reloaded = db.GetActiveSave();
  ASSERT_NE(reloaded, nullptr);
  EXPECT_EQ(reloaded->name, "Persist FC");
  ASSERT_EQ(reloaded->season.fixtures.size(), 3u);

  EXPECT_EQ(reloaded->season.fixtures[0].fixtureID, 101);
  EXPECT_EQ(reloaded->season.fixtures[0].homeTeamID, 1);
  EXPECT_EQ(reloaded->season.fixtures[0].awayTeamID, 2);
  EXPECT_EQ(reloaded->season.fixtures[0].homeGoals, 3);
  EXPECT_EQ(reloaded->season.fixtures[0].awayGoals, 1);
  EXPECT_TRUE(reloaded->season.fixtures[0].played);

  EXPECT_EQ(reloaded->season.fixtures[1].fixtureID, 102);
  EXPECT_EQ(reloaded->season.fixtures[1].homeTeamID, 3);
  EXPECT_EQ(reloaded->season.fixtures[1].awayTeamID, 1);
  EXPECT_EQ(reloaded->season.fixtures[1].homeGoals, 0);
  EXPECT_EQ(reloaded->season.fixtures[1].awayGoals, 2);
  EXPECT_TRUE(reloaded->season.fixtures[1].played);

  EXPECT_EQ(reloaded->season.fixtures[2].fixtureID, 103);
  EXPECT_EQ(reloaded->season.fixtures[2].homeTeamID, 1);
  EXPECT_EQ(reloaded->season.fixtures[2].awayTeamID, 4);
  EXPECT_EQ(reloaded->season.fixtures[2].homeGoals, 1);
  EXPECT_EQ(reloaded->season.fixtures[2].awayGoals, 1);
  EXPECT_TRUE(reloaded->season.fixtures[2].played);
}

TEST(CareerPersistenceAuditTest, AtomicSaveAndBackupRecovery) {
  fs::path dir = UniqueTempDir("atomic_backup");
  fs::remove_all(dir);
  fs::create_directories(dir);

  CareerDatabase& db = CareerDatabase::GetInstance();
  db.Initialize(dir.string());
  ASSERT_TRUE(db.CreateNewCareer("Backup FC", "manager", "Dr. Healer"));

  CareerSave* save = db.GetActiveSave();
  ASSERT_NE(save, nullptr);
  save->transferBudget = 42000000;
  save->seasonWins = 7;

  // First save to slot 1: creates slot_1.save
  ASSERT_TRUE(db.SaveCareerSlot(1));
  fs::path primarySave = dir / "career_slot_1.save";
  fs::path backupSave = dir / "career_slot_1.save.bak";
  fs::path tempSave = dir / "career_slot_1.save.tmp";

  EXPECT_TRUE(fs::exists(primarySave));
  // .tmp should be cleaned up after successful save
  EXPECT_FALSE(fs::exists(tempSave));

  // Modify active state and save again: should create .bak containing previous version
  save->transferBudget = 99000000;
  save->seasonWins = 12;
  ASSERT_TRUE(db.SaveCareerSlot(1));

  EXPECT_TRUE(fs::exists(primarySave));
  EXPECT_TRUE(fs::exists(backupSave));
  EXPECT_FALSE(fs::exists(tempSave));

  // Now simulate primary file corruption by writing garbage into primarySave
  {
    std::ofstream corrupt(primarySave.string(), std::ios::binary | std::ios::trunc);
    corrupt << "CORRUPTED_SQLITE_GARBAGE_DATA_1234567890\n";
  }

  // Reload slot 1: should detect primary corruption, fallback to .bak, and self-heal primary
  EXPECT_TRUE(db.LoadCareerSlot(1));
  CareerSave* healed = db.GetActiveSave();
  ASSERT_NE(healed, nullptr);
  EXPECT_EQ(healed->name, "Backup FC");
  // Should have recovered state from .bak (budget 42M, wins 7)
  EXPECT_EQ(healed->transferBudget, 42000000);
  EXPECT_EQ(healed->seasonWins, 7);

  // Verify primary was self-healed
  CareerSaveSummary summary;
  EXPECT_TRUE(db.GetSlotSummary(1, summary));
  EXPECT_TRUE(summary.isValid);
  EXPECT_EQ(summary.clubName, "Backup FC");
}

TEST(CareerPersistenceAuditTest, AutoSaveWorkflow) {
  fs::path dir = UniqueTempDir("autosave_wf");
  fs::remove_all(dir);
  fs::create_directories(dir);

  CareerDatabase& db = CareerDatabase::GetInstance();
  db.Initialize(dir.string());
  // CreateNewCareer automatically calls AutoSave()
  ASSERT_TRUE(db.CreateNewCareer("AutoSave FC", "manager", "Auto Master"));

  EXPECT_TRUE(db.HasSaveSlot(-1));
  fs::path autoSavePath = dir / "career_autosave.save";
  EXPECT_TRUE(fs::exists(autoSavePath));

  CareerSave* save = db.GetActiveSave();
  ASSERT_NE(save, nullptr);
  save->name = "Mutated FC";
  save->seasonWins = 15;

  // Explicit AutoSave
  EXPECT_TRUE(db.AutoSave());

  // Mutate memory
  save->name = "Different FC";

  // Reload from autosave slot -1
  EXPECT_TRUE(db.LoadCareerSlot(-1));
  EXPECT_EQ(db.GetActiveSave()->name, "Mutated FC");
  EXPECT_EQ(db.GetActiveSave()->seasonWins, 15);
}

TEST(CareerPersistenceAuditTest, DeleteSlotCleansAllArtifacts) {
  fs::path dir = UniqueTempDir("delete_artifacts");
  fs::remove_all(dir);
  fs::create_directories(dir);

  CareerDatabase& db = CareerDatabase::GetInstance();
  db.Initialize(dir.string());
  ASSERT_TRUE(db.CreateNewCareer("Clean FC", "manager", "Mr. Clean"));

  // Save twice so primary, backup both exist
  ASSERT_TRUE(db.SaveCareerSlot(3));
  db.GetActiveSave()->seasonWins = 5;
  ASSERT_TRUE(db.SaveCareerSlot(3));

  fs::path primary = dir / "career_slot_3.save";
  fs::path backup = dir / "career_slot_3.save.bak";
  fs::path temp = dir / "career_slot_3.save.tmp";

  // Create a leftover .tmp artifact
  {
    std::ofstream tmpFile(temp.string());
    tmpFile << "leftover temp";
  }

  EXPECT_TRUE(fs::exists(primary));
  EXPECT_TRUE(fs::exists(backup));
  EXPECT_TRUE(fs::exists(temp));

  // Delete slot 3
  EXPECT_TRUE(db.DeleteCareerSlot(3));

  // All 3 artifacts must be cleaned up
  EXPECT_FALSE(fs::exists(primary));
  EXPECT_FALSE(fs::exists(backup));
  EXPECT_FALSE(fs::exists(temp));
  EXPECT_FALSE(db.HasSaveSlot(3));
}

}  // namespace
