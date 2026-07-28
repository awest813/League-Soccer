#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include <gtest/gtest.h>

#include "base/properties.hpp"
#include "league/leaguecode.hpp"
#include "sqlite3.h"
#include "utils/database.hpp"

using blunted::Database;
using blunted::Properties;

namespace {

Database g_testDatabase;
Properties g_testConfiguration;
std::string g_activeSaveDirectory;
std::uint64_t g_workspaceCounter = 0;

std::filesystem::path MakeUniqueWorkspaceRoot() {
  const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
  return std::filesystem::temp_directory_path() /
         ("gf-test-" + std::to_string(now) + "-" + std::to_string(++g_workspaceCounter));
}

}  // namespace

Database* GetDB() {
  return &g_testDatabase;
}

Properties* GetConfiguration() {
  return &g_testConfiguration;
}

std::string GetActiveSaveDirectory() {
  return g_activeSaveDirectory;
}

void SetActiveSaveDirectory(const std::string& dir) {
  g_activeSaveDirectory = dir;
}

namespace {

class ScopedCurrentPath {
public:
  explicit ScopedCurrentPath(const std::filesystem::path& target)
      : original_(std::filesystem::current_path()) {
    std::filesystem::current_path(target);
  }

  ~ScopedCurrentPath() { std::filesystem::current_path(original_); }

private:
  std::filesystem::path original_;
};

class ScopedWorkspace {
public:
  ScopedWorkspace() {
    root_ = MakeUniqueWorkspaceRoot();
    std::filesystem::create_directories(root_);
  }

  ~ScopedWorkspace() {
    std::error_code error;
    std::filesystem::remove_all(root_, error);
  }

  const std::filesystem::path& root() const { return root_; }

private:
  std::filesystem::path root_;
};

void ExecSql(sqlite3* db, const std::string& sql) {
  char* errorMessage = nullptr;
  const int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errorMessage);
  ASSERT_EQ(rc, SQLITE_OK) << (errorMessage ? errorMessage : "unknown sqlite error");
  if (errorMessage) {
    sqlite3_free(errorMessage);
  }
}

void WriteTextFile(const std::filesystem::path& path, const std::string& contents) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream output(path, std::ios::binary);
  ASSERT_TRUE(output.is_open()) << path.string();
  output << contents;
}

void CreateFoundationDatabase(const std::filesystem::path& root, const std::string& databaseName,
                              const std::string& playerStatsColumnName = "profile_xml") {
  const auto databaseDir = root / "databases" / databaseName;
  std::filesystem::create_directories(databaseDir);

  sqlite3* db = nullptr;
  ASSERT_EQ(sqlite3_open((databaseDir / "database.sqlite").string().c_str(), &db), SQLITE_OK);

  ExecSql(db,
          "CREATE TABLE leagues(id INTEGER PRIMARY KEY, name TEXT);"
          "CREATE TABLE teams(id INTEGER PRIMARY KEY, league_id INTEGER, name TEXT, logo_url TEXT, "
          "kit_url TEXT);"
          "CREATE TABLE players(id INTEGER PRIMARY KEY, team_id INTEGER, " +
              playerStatsColumnName + " TEXT, base_stat INTEGER);");

  ExecSql(db,
          "INSERT INTO leagues(id, name) VALUES (1, 'Premier Test League');"
          "INSERT INTO teams(id, league_id, name, logo_url, kit_url) VALUES "
          "(1, 1, 'Alpha FC', 'images_teams/test/alpha_logo.png', 'images_teams/test/alpha'),"
          "(2, 1, 'Bravo FC', 'images_teams/test/bravo_logo.png', 'images_teams/test/bravo'),"
          "(3, 1, 'Charlie FC', 'images_teams/test/charlie_logo.png', 'images_teams/test/charlie'),"
          "(4, 1, 'Delta FC', 'images_teams/test/delta_logo.png', 'images_teams/test/delta');");

  for (int playerId = 1; playerId <= 4; ++playerId) {
    const int teamId = playerId;
    ExecSql(db, "INSERT INTO players(id, team_id, " + playerStatsColumnName +
                    ", base_stat) VALUES (" + std::to_string(playerId) + ", " +
                    std::to_string(teamId) + ", '<profile><pace>0.5</pace></profile>', 55);");
  }

  ASSERT_EQ(sqlite3_close(db), SQLITE_OK);

  WriteTextFile(databaseDir / "images_teams" / "test" / "alpha_logo.png", "alpha");
  WriteTextFile(databaseDir / "images_teams" / "test" / "alpha" / "kit_home.png", "kit");
  WriteTextFile(databaseDir / "images_teams" / "test" / "bravo_logo.png", "bravo");
}

int QuerySingleInt(Database* db, const std::string& sql) {
  auto result = db->Query(sql);
  EXPECT_FALSE(result->data.empty()) << sql;
  EXPECT_FALSE(result->data.at(0).empty()) << sql;
  return result->data.empty() || result->data.at(0).empty() ? 0
                                                            : std::stoi(result->data.at(0).at(0));
}

std::string QuerySingleString(Database* db, const std::string& sql) {
  auto result = db->Query(sql);
  EXPECT_FALSE(result->data.empty()) << sql;
  EXPECT_FALSE(result->data.at(0).empty()) << sql;
  return result->data.empty() || result->data.at(0).empty() ? "" : result->data.at(0).at(0);
}

TEST(LeagueBootstrapIntegrationTest, CreateNewLeagueSaveCopiesDatabaseAndKnownAssets) {
  ScopedWorkspace workspace;
  ScopedCurrentPath cwd(workspace.root());
  CreateFoundationDatabase(workspace.root(), "default");

  EXPECT_EQ(CreateNewLeagueSave("default", "season_one"), 0);

  const auto saveDir = workspace.root() / "saves" / "season_one";
  EXPECT_TRUE(std::filesystem::exists(saveDir / "database.sqlite"));
  EXPECT_TRUE(std::filesystem::exists(saveDir / "autosave.sqlite"));
  EXPECT_TRUE(std::filesystem::exists(saveDir / "images_teams" / "test" / "alpha_logo.png"));
  EXPECT_TRUE(
      std::filesystem::exists(saveDir / "images_teams" / "test" / "alpha" / "kit_home.png"));
  EXPECT_FALSE(std::filesystem::exists(saveDir / "images_teams" / "test" / "bravo"));
}

TEST(LeagueBootstrapIntegrationTest, PrepareDatabaseForLeagueSupportsProfileXmlSchema) {
  ScopedWorkspace workspace;
  ScopedCurrentPath cwd(workspace.root());
  CreateFoundationDatabase(workspace.root(), "default", "profile_xml");

  ASSERT_TRUE(
      GetDB()->Load((workspace.root() / "databases" / "default" / "database.sqlite").string()));
  ASSERT_TRUE(PrepareDatabaseForLeague());

  EXPECT_EQ(
      QuerySingleInt(GetDB(),
                     "SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name='settings'"),
      1);
  EXPECT_EQ(
      QuerySingleInt(GetDB(),
                     "SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name='calendar'"),
      1);
  EXPECT_EQ(
      QuerySingleInt(
          GetDB(), "SELECT COUNT(*) FROM pragma_table_info('players') WHERE name='stats_temporal'"),
      1);

  const auto temporalStats =
      QuerySingleString(GetDB(), "SELECT stats_temporal FROM players WHERE id = 1 LIMIT 1");
  EXPECT_NE(temporalStats.find("<current>"), std::string::npos);
  EXPECT_NE(temporalStats.find("<pace>"), std::string::npos);
}

TEST(LeagueBootstrapIntegrationTest, GenerateSeasonCalendarsCreatesRoundRobinFixtures) {
  ScopedWorkspace workspace;
  ScopedCurrentPath cwd(workspace.root());
  CreateFoundationDatabase(workspace.root(), "default");

  ASSERT_TRUE(
      GetDB()->Load((workspace.root() / "databases" / "default" / "database.sqlite").string()));
  ASSERT_TRUE(PrepareDatabaseForLeague());

  GetDB()->Query(
      "INSERT INTO settings(managername, team_id, currency, difficulty, seasonyear, timestamp) "
      "VALUES ('Coach Test', 1, 'euro', 0.5, 2013, '2013-10-23')");
  GenerateSeasonCalendars();

  EXPECT_EQ(QuerySingleInt(GetDB(), "SELECT COUNT(*) FROM calendar"), 12);
  EXPECT_EQ(QuerySingleInt(GetDB(), "SELECT COUNT(DISTINCT timestamp) FROM calendar"), 6);
  EXPECT_NE(QuerySingleString(GetDB(), "SELECT timestamp FROM settings LIMIT 1"), "2013-10-23");
}

TEST(LeagueBootstrapIntegrationTest, StepLeagueTimeAdvancesMatchDaysAndRollsSeason) {
  ScopedWorkspace workspace;
  ScopedCurrentPath cwd(workspace.root());
  CreateFoundationDatabase(workspace.root(), "default");

  ASSERT_TRUE(
      GetDB()->Load((workspace.root() / "databases" / "default" / "database.sqlite").string()));
  ASSERT_TRUE(PrepareDatabaseForLeague());

  GetDB()->Query(
      "INSERT INTO settings(managername, team_id, currency, difficulty, seasonyear, timestamp) "
      "VALUES ('Coach Test', 1, 'euro', 0.5, 2013, '2013-10-23')");

  ASSERT_TRUE(StepLeagueTime());
  EXPECT_EQ(QuerySingleString(GetDB(), "SELECT timestamp FROM settings LIMIT 1"), "2013-10-26");

  GetDB()->Query("UPDATE settings SET timestamp = '2014-01-01', seasonyear = 2013");
  ASSERT_TRUE(StepLeagueTime());
  EXPECT_EQ(QuerySingleInt(GetDB(), "SELECT seasonyear FROM settings LIMIT 1"), 2014);
  EXPECT_GT(QuerySingleInt(GetDB(), "SELECT COUNT(*) FROM calendar"), 0);
}

}  // namespace
