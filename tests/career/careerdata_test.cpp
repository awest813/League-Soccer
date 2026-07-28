#include <gtest/gtest.h>

#include "data/careerdata.hpp"

// Unit tests for Phase 6 CareerSaveRegistry new functionality:
//   6.13 ApplyReputationDelta
//   6.16 SetLeagueExpansionSettings / ComputePromotionRelegation
//   6.17 SetCustomLeague

namespace {

// ---------------------------------------------------------------------------
// Helper – create a fresh save and return its ID
// ---------------------------------------------------------------------------
static int MakeSave(CareerSaveRegistry& db) {
  CareerSave s;
  s.mode = CareerMode::MANAGER;
  s.club.clubID = 1;
  s.reputation = 50;
  return db.CreateSave(s);
}

// ---------------------------------------------------------------------------
// 6.13 – Press conference reputation
// ---------------------------------------------------------------------------

TEST(CareerDataTest, ReputationClampsAtHundred) {
  CareerSaveRegistry db;
  int id = MakeSave(db);
  db.ApplyReputationDelta(id, 100);
  EXPECT_EQ(db.GetSave(id)->reputation, 100);
}

TEST(CareerDataTest, ReputationClampsAtZero) {
  CareerSaveRegistry db;
  int id = MakeSave(db);
  db.ApplyReputationDelta(id, -200);
  EXPECT_EQ(db.GetSave(id)->reputation, 0);
}

TEST(CareerDataTest, ReputationPositiveDelta) {
  CareerSaveRegistry db;
  int id = MakeSave(db);
  db.ApplyReputationDelta(id, 5);
  EXPECT_EQ(db.GetSave(id)->reputation, 55);
}

TEST(CareerDataTest, ReputationNegativeDelta) {
  CareerSaveRegistry db;
  int id = MakeSave(db);
  db.ApplyReputationDelta(id, -10);
  EXPECT_EQ(db.GetSave(id)->reputation, 40);
}

TEST(CareerDataTest, ReputationNoopForUnknownSave) {
  CareerSaveRegistry db;
  // Should not crash even when saveID does not exist
  db.ApplyReputationDelta(999, 10);
}

// ---------------------------------------------------------------------------
// 6.16 – League expansion / relegation
// ---------------------------------------------------------------------------

TEST(CareerDataTest, SetLeagueExpansionSettingsPersists) {
  CareerSaveRegistry db;
  int id = MakeSave(db);

  LeagueExpansionSettings settings;
  settings.enabled = true;
  DivisionConfig top;
  top.name = "Premier Division";
  top.numTeams = 20;
  top.promotionSpots = 0;
  top.relegationSpots = 3;
  settings.divisions.push_back(top);

  db.SetLeagueExpansionSettings(id, settings);

  const CareerSave* s = db.GetSave(id);
  ASSERT_NE(s, nullptr);
  EXPECT_TRUE(s->leagueSettings.enabled);
  ASSERT_EQ(s->leagueSettings.divisions.size(), 1u);
  EXPECT_EQ(s->leagueSettings.divisions[0].relegationSpots, 3);
}

TEST(CareerDataTest, ComputePromotionRelegation_BottomTeamsRelegated) {
  LeagueExpansionSettings settings;
  DivisionConfig d1;
  d1.relegationSpots = 2;
  d1.numTeams = 4;
  settings.divisions.push_back(d1);
  DivisionConfig d2;
  d2.relegationSpots = 0;
  d2.numTeams = 4;
  settings.divisions.push_back(d2);

  // Division 0 standings: teams 10, 20, 30, 40 (best to worst)
  std::vector<std::vector<int>> standings = {{10, 20, 30, 40}, {50, 60, 70, 80}};

  auto relegated = CareerSaveRegistry::ComputePromotionRelegation(settings, standings);

  // Teams 30 and 40 should be relegated from division 0
  ASSERT_EQ(relegated.size(), 2u);
  EXPECT_EQ(relegated[0].first, 0);
  EXPECT_EQ(relegated[0].second, 30);
  EXPECT_EQ(relegated[1].first, 0);
  EXPECT_EQ(relegated[1].second, 40);
}

TEST(CareerDataTest, ComputePromotionRelegation_LastDivisionNotRelegated) {
  LeagueExpansionSettings settings;
  DivisionConfig d1;
  d1.relegationSpots = 2;
  settings.divisions.push_back(d1);
  DivisionConfig d2;
  d2.relegationSpots = 2;  // Bottom division - should not cause further relegation
  settings.divisions.push_back(d2);

  std::vector<std::vector<int>> standings = {{1, 2, 3, 4}, {5, 6, 7, 8}};
  auto relegated = CareerSaveRegistry::ComputePromotionRelegation(settings, standings);

  // Only division 0 teams can be relegated (div 1 is the bottom)
  for (const auto& p : relegated) {
    EXPECT_EQ(p.first, 0);
  }
}

TEST(CareerDataTest, ComputePromotionRelegation_EmptyStandings) {
  LeagueExpansionSettings settings;
  DivisionConfig d;
  d.relegationSpots = 3;
  settings.divisions.push_back(d);

  std::vector<std::vector<int>> standings;  // empty
  auto relegated = CareerSaveRegistry::ComputePromotionRelegation(settings, standings);
  EXPECT_TRUE(relegated.empty());
}

// ---------------------------------------------------------------------------
// 6.17 – Custom league
// ---------------------------------------------------------------------------

TEST(CareerDataTest, SetCustomLeaguePersists) {
  CareerSaveRegistry db;
  int id = MakeSave(db);

  CustomLeagueConfig cfg;
  cfg.leagueName = "My Super League";
  cfg.numDivisions = 2;
  cfg.cupCompetition = true;
  cfg.cupName = "Golden Cup";

  db.SetCustomLeague(id, cfg);

  const CareerSave* s = db.GetSave(id);
  ASSERT_NE(s, nullptr);
  EXPECT_EQ(s->customLeague.leagueName, "My Super League");
  EXPECT_EQ(s->customLeague.numDivisions, 2);
  EXPECT_TRUE(s->customLeague.cupCompetition);
  EXPECT_EQ(s->customLeague.cupName, "Golden Cup");
}

TEST(CareerDataTest, SetCustomLeagueNoopForUnknownSave) {
  CareerSaveRegistry db;
  CustomLeagueConfig cfg;
  cfg.leagueName = "Ghost League";
  // Should not crash when save does not exist
  db.SetCustomLeague(999, cfg);
}

// ---------------------------------------------------------------------------
// Owner mode – CareerMode::OWNER enum support
// ---------------------------------------------------------------------------

TEST(CareerDataTest, OwnerModeEnumExists) {
  CareerSave s;
  s.mode = CareerMode::OWNER;
  EXPECT_EQ(s.mode, CareerMode::OWNER);
}

TEST(CareerDataTest, OwnerModeCreateSaveAndRetrieve) {
  CareerSaveRegistry db;
  CareerSave s;
  s.mode = CareerMode::OWNER;
  s.controlledEntityID = 5;
  // CreateSave derives the top-level reputation/budget from these canonical
  // sub-structs, so populate those rather than the derived fields.
  s.club.reputation = 60;
  s.finance.transferBudget = 60000000;
  int id = db.CreateSave(s);

  CareerSave* retrieved = db.GetSave(id);
  ASSERT_NE(retrieved, nullptr);
  EXPECT_EQ(retrieved->mode, CareerMode::OWNER);
  EXPECT_EQ(retrieved->budget, 60000000);
  EXPECT_EQ(retrieved->reputation, 60);
}

TEST(CareerDataTest, OwnerReputationClampingWorks) {
  CareerSaveRegistry db;
  CareerSave s;
  s.mode = CareerMode::OWNER;
  s.club.reputation = 90;
  int id = db.CreateSave(s);

  db.ApplyReputationDelta(id, 20);
  EXPECT_EQ(db.GetSave(id)->reputation, 100);

  db.ApplyReputationDelta(id, -150);
  EXPECT_EQ(db.GetSave(id)->reputation, 0);
}

TEST(CareerDataTest, OwnerModeSeasonAdvances) {
  CareerSaveRegistry db;
  CareerSave s;
  s.mode = CareerMode::OWNER;
  s.currentSeason = 1;
  int id = db.CreateSave(s);

  db.AdvanceSeason(id);
  EXPECT_EQ(db.GetSave(id)->currentSeason, 2);

  db.AdvanceSeason(id);
  EXPECT_EQ(db.GetSave(id)->currentSeason, 3);
}

TEST(CareerDataTest, OwnerModeLeagueExpansion) {
  CareerSaveRegistry db;
  CareerSave s;
  s.mode = CareerMode::OWNER;
  int id = db.CreateSave(s);

  LeagueExpansionSettings settings;
  settings.enabled = true;
  DivisionConfig d;
  d.name = "Owner League";
  d.numTeams = 18;
  d.relegationSpots = 2;
  settings.divisions.push_back(d);

  db.SetLeagueExpansionSettings(id, settings);
  EXPECT_TRUE(db.GetSave(id)->leagueSettings.enabled);
  EXPECT_EQ(db.GetSave(id)->leagueSettings.divisions[0].name, "Owner League");
}

TEST(CareerDataTest, OwnerModeCustomLeague) {
  CareerSaveRegistry db;
  CareerSave s;
  s.mode = CareerMode::OWNER;
  int id = db.CreateSave(s);

  CustomLeagueConfig cfg;
  cfg.leagueName = "Owner Super League";
  cfg.numDivisions = 3;
  cfg.cupCompetition = true;
  cfg.cupName = "Owner Cup";

  db.SetCustomLeague(id, cfg);
  EXPECT_EQ(db.GetSave(id)->customLeague.leagueName, "Owner Super League");
  EXPECT_EQ(db.GetSave(id)->customLeague.numDivisions, 3);
  EXPECT_TRUE(db.GetSave(id)->customLeague.cupCompetition);
}

TEST(CareerDataTest, OwnerDeleteSave) {
  CareerSaveRegistry db;
  CareerSave s;
  s.mode = CareerMode::OWNER;
  int id = db.CreateSave(s);

  ASSERT_NE(db.GetSave(id), nullptr);
  db.DeleteSave(id);
  EXPECT_EQ(db.GetSave(id), nullptr);
}

TEST(CareerDataTest, OwnerRecordSeason) {
  CareerSaveRegistry db;
  CareerSave s;
  s.mode = CareerMode::OWNER;
  int id = db.CreateSave(s);

  SeasonRecord rec;
  rec.season = 1;
  rec.wins = 20;
  rec.draws = 10;
  rec.losses = 8;
  rec.goalsFor = 55;
  rec.goalsAgainst = 30;
  rec.leaguePosition = 3;
  rec.wonTitle = false;

  db.RecordSeason(id, rec);
  ASSERT_EQ(db.GetSave(id)->history.size(), 1u);
  EXPECT_EQ(db.GetSave(id)->history[0].wins, 20);
  EXPECT_EQ(db.GetSave(id)->history[0].leaguePosition, 3);
}

// ---------------------------------------------------------------------------
// DraftSystem – projectedPick ordering
// ---------------------------------------------------------------------------

}  // namespace

#include "data/draftdata.hpp"
#include "data/scoutingdata.hpp"

namespace {

TEST(DraftDataTest, ProjectedPickMatchesSortedOrder) {
  DraftSystem ds;
  ds.GenerateProspects(2025, 10);
  const auto& prospects = ds.GetProspects();
  ASSERT_EQ(static_cast<int>(prospects.size()), 10);

  // Prospects are sorted best-to-worst by actualRating; projectedPick must be 1..N in order
  for (int i = 0; i < static_cast<int>(prospects.size()); ++i) {
    EXPECT_EQ(prospects[i].projectedPick, i + 1)
        << "Pick " << i << " has wrong projectedPick";
    if (i > 0) {
      EXPECT_GE(prospects[i - 1].actualRating, prospects[i].actualRating)
          << "Prospects are not sorted descending by actualRating";
    }
  }
}

TEST(DraftDataTest, ProjectedPickIsOneForBestProspect) {
  DraftSystem ds;
  ds.GenerateProspects(2025, 5);
  const auto& prospects = ds.GetProspects();
  ASSERT_FALSE(prospects.empty());
  EXPECT_EQ(prospects.front().projectedPick, 1);
}

// ---------------------------------------------------------------------------
// ScoutingNetwork – uncertainty formula boundary values
// ---------------------------------------------------------------------------

TEST(ScoutingNetworkTest, UncertaintyAtSkill100Is5) {
  ScoutingNetwork net;
  Scout s;
  s.skill = 100;
  int scoutID = net.HireScout(s);
  ScoutReport report = net.GenerateReport(scoutID, 1, 1, 1);
  // skill 100 -> baseUncertainty = 30 - (100 * 25 / 100) = 30 - 25 = 5
  for (const auto& kv : report.attributeUncertainty) {
    EXPECT_EQ(kv.second, 5) << "Attribute: " << kv.first;
  }
}

TEST(ScoutingNetworkTest, UncertaintyAtSkill0Is30) {
  ScoutingNetwork net;
  Scout s;
  s.skill = 0;
  int scoutID = net.HireScout(s);
  ScoutReport report = net.GenerateReport(scoutID, 1, 1, 1);
  // skill 0 -> baseUncertainty = 30 - 0 = 30
  for (const auto& kv : report.attributeUncertainty) {
    EXPECT_EQ(kv.second, 30) << "Attribute: " << kv.first;
  }
}

TEST(ScoutingNetworkTest, UncertaintyAtSkill50Is18) {
  ScoutingNetwork net;
  Scout s;
  s.skill = 50;
  int scoutID = net.HireScout(s);
  ScoutReport report = net.GenerateReport(scoutID, 1, 1, 1);
  // skill 50 -> 30 - int(50 * 25.0f / 100.0f) = 30 - int(12.5f) = 30 - 12 = 18
  // (with float arithmetic, 50*25/100 = 12.5 -> truncated to 12)
  for (const auto& kv : report.attributeUncertainty) {
    EXPECT_EQ(kv.second, 18) << "Attribute: " << kv.first;
  }
}

TEST(ScoutingNetworkTest, NoScoutUsesDefaultSkill50) {
  ScoutingNetwork net;
  // Generate report with non-existent scoutID -> falls back to skill=50
  ScoutReport report = net.GenerateReport(999, 1, 1, 1);
  for (const auto& kv : report.attributeUncertainty) {
    EXPECT_EQ(kv.second, 18) << "Attribute: " << kv.first;
  }
}

}  // namespace
