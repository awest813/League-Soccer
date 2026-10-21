// Invariants of the bundled database (data/databases/default). The engine
// reads it with little validation, so a bad row shows up as a crash or a
// wrong screen rather than an error.

#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <sqlite3.h>

#include "data/dbroles.hpp"

namespace {

const char* const kDatabasePath = "data/databases/default/database.sqlite";

class BundledDatabase : public ::testing::Test {
protected:
  void SetUp() override {
    ASSERT_EQ(sqlite3_open_v2(kDatabasePath, &db, SQLITE_OPEN_READONLY, nullptr), SQLITE_OK);
  }
  void TearDown() override { sqlite3_close(db); }

  std::vector<std::vector<std::string>> Rows(const std::string& sql) {
    std::vector<std::vector<std::string>> rows;
    sqlite3_stmt* stmt = nullptr;
    EXPECT_EQ(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr), SQLITE_OK) << sql;
    while (stmt && sqlite3_step(stmt) == SQLITE_ROW) {
      std::vector<std::string> row;
      for (int c = 0; c < sqlite3_column_count(stmt); ++c) {
        const unsigned char* text = sqlite3_column_text(stmt, c);
        row.push_back(text ? reinterpret_cast<const char*>(text) : "");
      }
      rows.push_back(row);
    }
    sqlite3_finalize(stmt);
    return rows;
  }

  int Count(const std::string& sql) { return std::stoi(Rows(sql).at(0).at(0)); }

  sqlite3* db = nullptr;
};

TEST_F(BundledDatabase, PassesSqliteIntegrityCheck) {
  EXPECT_EQ(Rows("PRAGMA integrity_check").at(0).at(0), "ok");
}

TEST_F(BundledDatabase, EveryRowReferencesAnExistingParent) {
  EXPECT_EQ(Count("SELECT COUNT(*) FROM countries WHERE region_id NOT IN (SELECT id FROM regions)"),
            0);
  EXPECT_EQ(
      Count("SELECT COUNT(*) FROM leagues WHERE country_id NOT IN (SELECT id FROM countries)"), 0);
  EXPECT_EQ(Count("SELECT COUNT(*) FROM teams WHERE league_id NOT IN (SELECT id FROM leagues)"), 0);
  EXPECT_EQ(Count("SELECT COUNT(*) FROM players WHERE team_id NOT IN (SELECT id FROM teams)"), 0);
}

// TeamData uses the first eleven by formation order as starters, in the
// formation's slot order; slot 1 is the goalkeeper.
TEST_F(BundledDatabase, EveryTeamCanFieldAnElevenWithAKeeperInGoal) {
  for (const auto& team : Rows("SELECT id, name FROM teams")) {
    const auto players =
        Rows("SELECT role, formationorder FROM players WHERE team_id = " + team[0] +
             " ORDER BY formationorder");
    ASSERT_GE(players.size(), 11u) << team[1];
    EXPECT_EQ(dbroles::PositionFromDbRole(players[0][0]), "GK") << team[1];
    std::set<std::string> orders;
    for (const auto& player : players)
      EXPECT_TRUE(orders.insert(player[1]).second) << team[1] << " repeats order " << player[1];
  }
}

TEST_F(BundledDatabase, PlayersHaveUsableNamesAndStats) {
  // The HUD and goal messages show the surname; one-name players keep it there.
  EXPECT_EQ(Count("SELECT COUNT(*) FROM players WHERE lastname IS NULL OR lastname = ''"), 0);
  EXPECT_EQ(Count("SELECT COUNT(*) FROM players WHERE base_stat <= 0 OR base_stat > 1"), 0);
  EXPECT_EQ(Count("SELECT COUNT(*) FROM players WHERE age < 15 OR age > 45"), 0);
  EXPECT_EQ(Count("SELECT COUNT(*) FROM players WHERE skincolor < 1 OR skincolor > 4"), 0);
  EXPECT_EQ(Count("SELECT COUNT(*) FROM players WHERE height < 1.5 OR height > 2.1"), 0);
  // PlayerData asserts every stat it is asked for exists in the profile.
  for (const auto& player : Rows("SELECT lastname, profile_xml FROM players")) {
    for (const char* stat : {"physical_velocity", "technical_shot", "mental_vision"}) {
      EXPECT_NE(player[1].find(std::string("<") + stat + ">"), std::string::npos)
          << player[0] << " lacks " << stat;
    }
  }
}

TEST_F(BundledDatabase, ImagesExist) {
  namespace fs = std::filesystem;
  const fs::path root = fs::path(kDatabasePath).parent_path();
  for (const auto& row : Rows("SELECT logo_url FROM leagues UNION ALL SELECT logo_url FROM teams"))
    EXPECT_TRUE(fs::exists(root / row[0])) << row[0];
  for (const auto& row : Rows("SELECT kit_url FROM teams")) {
    for (const char* kit : {"_kit_01.png", "_kit_02.png"})
      EXPECT_TRUE(fs::exists(root / (row[0] + kit))) << row[0] << kit;
  }
}

}  // namespace
