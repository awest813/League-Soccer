#include <gtest/gtest.h>

#include "data/careerdata.hpp"

// Unit tests for Phase 6 CareerDatabase new functionality:
//   6.13 ApplyReputationDelta
//   6.16 SetLeagueExpansionSettings / ComputePromotionRelegation
//   6.17 SetCustomLeague

namespace {

// ---------------------------------------------------------------------------
// Helper – create a fresh save and return its ID
// ---------------------------------------------------------------------------
static int MakeSave(CareerDatabase& db) {
  CareerSave s;
  s.mode = CareerMode::MANAGER;
  s.teamID = 1;
  s.reputation = 50;
  return db.CreateSave(s);
}

// ---------------------------------------------------------------------------
// 6.13 – Press conference reputation
// ---------------------------------------------------------------------------

TEST(CareerDataTest, ReputationClampsAtHundred) {
  CareerDatabase db;
  int id = MakeSave(db);
  db.ApplyReputationDelta(id, 100);
  EXPECT_EQ(db.GetSave(id)->reputation, 100);
}

TEST(CareerDataTest, ReputationClampsAtZero) {
  CareerDatabase db;
  int id = MakeSave(db);
  db.ApplyReputationDelta(id, -200);
  EXPECT_EQ(db.GetSave(id)->reputation, 0);
}

TEST(CareerDataTest, ReputationPositiveDelta) {
  CareerDatabase db;
  int id = MakeSave(db);
  db.ApplyReputationDelta(id, 5);
  EXPECT_EQ(db.GetSave(id)->reputation, 55);
}

TEST(CareerDataTest, ReputationNegativeDelta) {
  CareerDatabase db;
  int id = MakeSave(db);
  db.ApplyReputationDelta(id, -10);
  EXPECT_EQ(db.GetSave(id)->reputation, 40);
}

TEST(CareerDataTest, ReputationNoopForUnknownSave) {
  CareerDatabase db;
  // Should not crash even when saveID does not exist
  db.ApplyReputationDelta(999, 10);
}

// ---------------------------------------------------------------------------
// 6.16 – League expansion / relegation
// ---------------------------------------------------------------------------

TEST(CareerDataTest, SetLeagueExpansionSettingsPersists) {
  CareerDatabase db;
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

  auto relegated = CareerDatabase::ComputePromotionRelegation(settings, standings);

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
  auto relegated = CareerDatabase::ComputePromotionRelegation(settings, standings);

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
  auto relegated = CareerDatabase::ComputePromotionRelegation(settings, standings);
  EXPECT_TRUE(relegated.empty());
}

// ---------------------------------------------------------------------------
// 6.17 – Custom league
// ---------------------------------------------------------------------------

TEST(CareerDataTest, SetCustomLeaguePersists) {
  CareerDatabase db;
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
  CareerDatabase db;
  CustomLeagueConfig cfg;
  cfg.leagueName = "Ghost League";
  // Should not crash when save does not exist
  db.SetCustomLeague(999, cfg);
}

}  // namespace
