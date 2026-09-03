#include "leaguecode.hpp"

#define BOOST_FILESYSTEM_VERSION 3

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <vector>

#include "base/utils.hpp"
#include "utils/database.hpp"
#include "utils/xmlloader.hpp"

namespace {

bool DatabaseHasTable(Database* database, const std::string& tableName) {
  auto result = database->Query("SELECT name FROM sqlite_master WHERE type = 'table' AND name = '" +
                                tableName + "' LIMIT 1");
  return !result->data.empty();
}

bool DatabaseHasColumn(Database* database, const std::string& tableName,
                       const std::string& columnName) {
  auto result = database->Query("PRAGMA table_info(" + tableName + ")");
  for (const auto& row : result->data) {
    if (row.size() > 1 && row.at(1) == columnName) {
      return true;
    }
  }
  return false;
}

LeagueFixtureInfo g_pendingFixture;
bool g_pendingFixtureActive = false;
std::string g_lastMatchdayDate;

const std::string& StringOrEmpty(const std::vector<std::string>& row, size_t index) {
  static const std::string kEmpty;
  return index < row.size() ? row.at(index) : kEmpty;
}

LeagueFixtureInfo FixtureFromRow(const std::vector<std::string>& row) {
  // c.id, c.team1_id, c.team2_id, c.competition_id, t1.name, t2.name, l.name, date(c.timestamp)
  LeagueFixtureInfo fixture;
  fixture.calendarID = atoi(StringOrEmpty(row, 0).c_str());
  fixture.homeTeamID = atoi(StringOrEmpty(row, 1).c_str());
  fixture.awayTeamID = atoi(StringOrEmpty(row, 2).c_str());
  fixture.competitionID = atoi(StringOrEmpty(row, 3).c_str());
  fixture.homeTeamName = StringOrEmpty(row, 4);
  fixture.awayTeamName = StringOrEmpty(row, 5);
  fixture.competitionName = StringOrEmpty(row, 6);
  fixture.date = StringOrEmpty(row, 7);
  return fixture;
}

std::string FixtureQueryJoins() {
  return "FROM calendar c "
         "JOIN teams t1 ON c.team1_id = t1.id "
         "JOIN teams t2 ON c.team2_id = t2.id "
         "LEFT JOIN leagues l ON c.competition_id = l.id ";
}

}  // namespace

int CreateNewLeagueSave(const std::string& srcDbName, const std::string& saveName) {
  // copy db file

  int errorCode = 0;
  // 1 == could not create save dir
  // 2 == could not copy db file
  // 3 == could not open copied database
  // 4 == could not copy file

  std::filesystem::path source("databases");
  source /= srcDbName;
  source /= "database.sqlite";

  std::filesystem::path dest("saves");
  dest /= saveName;

  if (!CreateDirectory(dest)) {
    errorCode = 1;  // could not create dir
  } else {
    if (!CopyFile(source, dest))
      errorCode = 2;  // could not copy file
  }

  // copy league db to tmp db

  std::error_code error;
  namespace fs = std::filesystem;
  fs::copy_file(dest / "database.sqlite", dest / "autosave.sqlite", error);

  // check db for graphics files and copy those

  Database* database;

  if (errorCode == 0) {
    database = GetDB();
    if (!database->Load(dest.string() + "/autosave.sqlite")) {
      errorCode = 3;  // could not open database
    }
  }

  if (errorCode == 0) {
    std::vector<std::string> imageList;
    auto result = database->Query("select logo_url, kit_url from teams");
    for (unsigned int r = 0; r < result->data.size(); r++) {
      imageList.push_back(result->data.at(r).at(0));
      imageList.push_back(result->data.at(r).at(1));
    }

    if (DatabaseHasTable(database, "competitions")) {
      result = database->Query("select logo_url from competitions");
      for (unsigned int r = 0; r < result->data.size(); r++) {
        imageList.push_back(result->data.at(r).at(0));
      }
    }

    // create directories, copy files

    for (unsigned int i = 0; i < imageList.size(); i++) {
      if (imageList.at(i).empty()) {
        continue;
      }

      // printf("copying %s\n", imageList.at(i).c_str());
      std::vector<std::string> tokens;
      tokenize(imageList.at(i), tokens, "/\\");
      if (tokens.empty()) {
        continue;
      }

      std::filesystem::path newdir = dest;
      for (unsigned int x = 0; x < tokens.size() - 1; x++) {
        // does directory exist?
        newdir /= tokens.at(x);
        if (!std::filesystem::exists(newdir)) {
          std::filesystem::create_directory(newdir);
          // printf("created dir: %s\n", newdir.string().c_str());
        }
        // printf("%s ", tokens.at(x).c_str());
      }
      // printf("\n");
      std::filesystem::path destfile = newdir / tokens.at(tokens.size() - 1);
      std::filesystem::path sourcefile("databases");
      sourcefile /= srcDbName;
      sourcefile /= imageList.at(i);

      if (!std::filesystem::exists(sourcefile)) {
        continue;
      }

      if (std::filesystem::is_directory(sourcefile)) {
        if (!std::filesystem::exists(destfile) && CopyDirectory(sourcefile, destfile) != 0) {
          errorCode = 4;
        }
      } else {
        std::error_code error;
        // printf("copying from %s to %s\n", sourcefile.string().c_str(),
        // destfile.string().c_str());
        if (!std::filesystem::exists(destfile)) {
          std::filesystem::copy_file(sourcefile, destfile, error);
        }
        if (error) {
          errorCode = 4;
        }
      }
      // if (error) printf("file %s could not be copied\n", imageList.at(i).c_str());
      // printf("\n");
    }
  }  // if !error

  return errorCode;
}

bool PrepareDatabaseForLeague() {
  auto result = GetDB()->Query(
      "CREATE TABLE settings(id INTEGER PRIMARY KEY AUTOINCREMENT, "
      "managername VARCHAR(32), "
      "team_id INTEGER, "
      "currency VARCHAR(32), "
      "difficulty FLOAT, "
      "seasonyear INTEGER, "
      "timestamp DATETIME)");

  result = GetDB()->Query(
      "CREATE TABLE calendar(id INTEGER PRIMARY KEY AUTOINCREMENT, "
      "timestamp DATETIME, "
      "team1_id INTEGER, "
      "team2_id INTEGER, "
      "competition_id INTEGER, "
      "tournament_id INTEGER)");

  result = GetDB()->Query(
      "CREATE TABLE match_results(id INTEGER PRIMARY KEY AUTOINCREMENT, "
      "calendar_id INTEGER, "
      "team1_id INTEGER, "
      "team2_id INTEGER, "
      "team1_goals INTEGER DEFAULT 0, "
      "team2_goals INTEGER DEFAULT 0, "
      "played INTEGER DEFAULT 0, "
      "competition_id INTEGER)");

  result = GetDB()->Query(
      "CREATE TABLE inbox_messages(id INTEGER PRIMARY KEY AUTOINCREMENT, "
      "sender VARCHAR(64), "
      "subject VARCHAR(128), "
      "body TEXT, "
      "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP, "
      "read INTEGER DEFAULT 0)");

  GetDB()->Query("DELETE FROM inbox_messages");
  GetDB()->Query(
      "INSERT INTO inbox_messages (sender, subject, body) "
      "VALUES ('League Office', 'Welcome to League Mode', "
      "'Welcome! Your season is about to begin. Check the Calendar for upcoming fixtures, "
      "the Standings to track your position, and the Dashboard for team management.')");

  if (!DatabaseHasColumn(GetDB(), "players", "stats_temporal")) {
    result = GetDB()->Query("ALTER TABLE players ADD COLUMN stats_temporal BLOB");
  }

  // copy stats XML tree into a new XML tree as subset 'current' (this tree is also going to contain
  // the archive per year)

  std::string statsColumnName;
  if (DatabaseHasColumn(GetDB(), "players", "stats")) {
    statsColumnName = "stats";
  } else if (DatabaseHasColumn(GetDB(), "players", "profile_xml")) {
    statsColumnName = "profile_xml";
  } else {
    return false;
  }

  result = GetDB()->Query("SELECT id, " + statsColumnName + " FROM players");

  std::string insertTemporalStatsQuery = "begin transaction;";

  for (unsigned int r = 0; r < result->data.size(); r++) {
    std::string playerIDString = result->data.at(r).at(0);
    std::string statsString = result->data.at(r).at(1);

    XMLLoader loader;
    XMLTree tree = loader.Load(statsString);
    //    loader.PrintTree(tree);
    //    printf("\n\n\n");
    XMLTree resultTree;
    resultTree.children.insert(std::pair<std::string, XMLTree>("current", tree));

    std::string resultTreeString = loader.GetSource(resultTree);
    insertTemporalStatsQuery += "UPDATE players SET stats_temporal ='" + resultTreeString +
                                "' WHERE id = " + playerIDString + ";";
  }

  insertTemporalStatsQuery += "commit;";
  auto insertTemporalStats = GetDB()->Query(insertTemporalStatsQuery);

  return true;
}

bool SaveAutosaveToDatabase() {
  namespace fs = std::filesystem;
  fs::path dest("saves");
  dest /= GetActiveSaveDirectory();

  std::error_code error;

  // remove previous database
  if (fs::exists(dest / "database.sqlite"))
    fs::remove(dest / "database.sqlite");

  // copy autosave to database
  fs::copy_file(dest / "autosave.sqlite", dest / "database.sqlite", error);

  if (error)
    return false;
  else
    return true;
}

bool SaveDatabaseToAutosave() {
  namespace fs = std::filesystem;
  fs::path dest("saves");
  dest /= GetActiveSaveDirectory();

  std::error_code error;

  // remove previous autosave
  if (fs::exists(dest / "autosave.sqlite"))
    fs::remove(dest / "autosave.sqlite");

  // copy database to autosave
  fs::copy_file(dest / "database.sqlite", dest / "autosave.sqlite", error);

  if (error)
    return false;
  else
    return true;
}

bool LoadLeague() {
  return true;
}

static void GenerateRoundRobinFixtures(int leagueID, int, const std::string& startDate) {
  auto teamsResult = GetDB()->Query(
      "SELECT id FROM teams WHERE league_id = " + int_to_str(leagueID) + " ORDER BY id");
  if (teamsResult->data.empty())
    return;

  std::vector<int> teamIDs;
  for (const auto& row : teamsResult->data) {
    teamIDs.push_back(atoi(row.at(0).c_str()));
  }

  int n = static_cast<int>(teamIDs.size());
  if (n < 2)
    return;

  bool needsBye = (n % 2 != 0);
  if (needsBye)
    teamIDs.push_back(-1);
  int totalTeams = static_cast<int>(teamIDs.size());

  struct Fixture {
    int team1, team2;
  };

  auto generateHalf = [&](int roundOffset) {
    std::vector<int> order = teamIDs;

    int numRounds = totalTeams - 1;
    for (int round = 0; round < numRounds; round++) {
      std::vector<Fixture> roundMatches;

      for (int i = 0; i < totalTeams / 2; i++) {
        int t1 = order[i];
        int t2 = order[totalTeams - 1 - i];
        if (t1 != -1 && t2 != -1) {
          if (roundOffset % 2 == 1)
            std::swap(t1, t2);
          roundMatches.push_back({t1, t2});
        }
      }

      int effectiveMatchDay = roundOffset * numRounds + round + 1;

      for (const auto& m : roundMatches) {
        GetDB()->Query(
            "INSERT INTO calendar (timestamp, team1_id, team2_id, competition_id, tournament_id) "
            "VALUES (date('" +
            startDate + "', '+" + int_to_str((effectiveMatchDay - 1) * 7) + " day'), " +
            int_to_str(m.team1) + ", " + int_to_str(m.team2) + ", " + int_to_str(leagueID) +
            ", NULL)");
      }

      int last = order[totalTeams - 1];
      for (int i = totalTeams - 1; i > 1; i--) {
        order[i] = order[i - 1];
      }
      order[1] = last;
    }
  };

  generateHalf(0);
  generateHalf(1);
}

void GenerateSeasonCalendars() {
  auto result = GetDB()->Query("SELECT timestamp, seasonyear FROM settings LIMIT 1");
  if (result->data.empty() || result->data.at(0).size() < 2)
    return;

  std::string startDate = result->data.at(0).at(0);

  auto leaguesResult = GetDB()->Query("SELECT id FROM leagues ORDER BY id");
  if (leaguesResult->data.empty())
    return;

  int seasonYear = atoi(result->data.at(0).at(1).c_str());

  GetDB()->Query("DELETE FROM calendar");

  for (const auto& row : leaguesResult->data) {
    int leagueID = atoi(row.at(0).c_str());
    GenerateRoundRobinFixtures(leagueID, seasonYear, startDate);
  }
}

bool StepLeagueTime() {
  auto result =
      GetDB()->Query("SELECT strftime(\"%w\", timestamp), seasonyear FROM settings LIMIT 1");
  if (result->data.empty() || result->data.at(0).size() < 2) {
    return false;
  }

  int dayOfWeek = atoi(result->data.at(0).at(0).c_str());
  int seasonyear = atoi(result->data.at(0).at(1).c_str());

  int offset = 0;
  if (dayOfWeek < 3) {
    offset = 3 - dayOfWeek;
  } else if (dayOfWeek < 6) {
    offset = 6 - dayOfWeek;
  } else {
    offset = 4;
  }

  GetDB()->Query("UPDATE settings SET timestamp = date(timestamp, '+" + int_to_str(offset) +
                 " day')");

  result = GetDB()->Query("SELECT strftime(\"%Y\", timestamp) FROM settings LIMIT 1");
  if (result->data.empty() || result->data.at(0).empty()) {
    return false;
  }

  int actualyear = atoi(result->data.at(0).at(0).c_str());
  if (actualyear > seasonyear) {
    GetDB()->Query("UPDATE settings SET seasonyear = " + int_to_str(seasonyear + 1));
    GenerateSeasonCalendars();
  }

  return true;
}

// ---------------------------------------------------------------------------
// Season matchday loop: fixture lookup, simulation, results, pending 3D match
// ---------------------------------------------------------------------------

bool LeagueHasPendingFixture() { return g_pendingFixtureActive; }

const LeagueFixtureInfo& LeagueGetPendingFixture() { return g_pendingFixture; }

void LeagueSetPendingFixture(const LeagueFixtureInfo& fixture) {
  g_pendingFixture = fixture;
  g_pendingFixtureActive = fixture.calendarID > 0;
}

void LeagueClearPendingFixture() { g_pendingFixtureActive = false; }

bool LeagueGetUserTeamID(int& teamID) {
  auto result = GetDB()->Query("SELECT team_id FROM settings LIMIT 1");
  if (result->data.empty() || result->data.at(0).empty()) {
    return false;
  }
  teamID = atoi(result->data.at(0).at(0).c_str());
  return true;
}

std::string LeagueResolveLogoPath(int teamID) {
  auto result =
      GetDB()->Query("SELECT logo_url FROM teams WHERE id = " + int_to_str(teamID));
  if (result->data.empty() || result->data.at(0).empty()) {
    return "";
  }
  std::string logoUrl = result->data.at(0).at(0);
  if (logoUrl.empty()) {
    return "";
  }

  std::string saveDir = GetActiveSaveDirectory();
  std::filesystem::path path;
  if (!saveDir.empty()) {
    path = std::filesystem::path("saves") / saveDir / logoUrl;
    if (std::filesystem::exists(path)) {
      return path.generic_string();  // forward slashes, like the rest of the asset pipeline
    }
  }
  path = std::filesystem::path("databases") / "default" / logoUrl;
  if (std::filesystem::exists(path)) {
    return path.generic_string();
  }
  return "";
}

bool LeagueGetUserNextFixture(LeagueFixtureInfo& out) {
  int userTeamID = 0;
  if (!LeagueGetUserTeamID(userTeamID)) {
    return false;
  }

  auto result = GetDB()->Query(
      "SELECT c.id, c.team1_id, c.team2_id, c.competition_id, t1.name, t2.name, l.name, "
      "date(c.timestamp) " +
      FixtureQueryJoins() +
      "JOIN settings s ON (c.team1_id = s.team_id OR c.team2_id = s.team_id) "
      "WHERE (c.team1_id = " +
      int_to_str(userTeamID) + " OR c.team2_id = " + int_to_str(userTeamID) + ") "
      "AND c.id NOT IN (SELECT calendar_id FROM match_results WHERE played = 1) "
      "ORDER BY c.timestamp LIMIT 1");

  if (result->data.empty()) {
    return false;
  }
  out = FixtureFromRow(result->data.at(0));
  return true;
}

bool LeagueGetFixtureByCalendarID(int calendarID, LeagueFixtureInfo& out) {
  auto result = GetDB()->Query(
      "SELECT c.id, c.team1_id, c.team2_id, c.competition_id, t1.name, t2.name, l.name, "
      "date(c.timestamp) " +
      FixtureQueryJoins() + "WHERE c.id = " + int_to_str(calendarID));
  if (result->data.empty()) {
    return false;
  }
  out = FixtureFromRow(result->data.at(0));
  return true;
}

bool LeagueGetNextMatchdayDate(std::string& outDate) {
  auto result = GetDB()->Query(
      "SELECT date(MIN(c.timestamp)) FROM calendar c "
      "WHERE c.id NOT IN (SELECT calendar_id FROM match_results WHERE played = 1)");
  if (result->data.empty() || result->data.at(0).empty() || result->data.at(0).at(0).empty()) {
    return false;
  }
  outDate = result->data.at(0).at(0);
  return true;
}

void LeagueRecordResult(const LeagueFixtureInfo& fixture, int homeGoals, int awayGoals) {
  GetDB()->Query("DELETE FROM match_results WHERE calendar_id = " +
                 int_to_str(fixture.calendarID));
  GetDB()->Query(
      "INSERT INTO match_results (calendar_id, team1_id, team2_id, team1_goals, team2_goals, "
      "played, competition_id) "
      "VALUES (" +
      int_to_str(fixture.calendarID) + ", " + int_to_str(fixture.homeTeamID) + ", " +
      int_to_str(fixture.awayTeamID) + ", " + int_to_str(homeGoals) + ", " +
      int_to_str(awayGoals) + ", 1, " + int_to_str(fixture.competitionID) + ")");

  std::string resultStr = fixture.homeTeamName + " " + int_to_str(homeGoals) + " - " +
                          int_to_str(awayGoals) + " " + fixture.awayTeamName;
  GetDB()->Query(
      "INSERT INTO inbox_messages (sender, subject, body) VALUES "
      "('Match Reporter', 'Match Result: " +
      resultStr +
      "', "
      "'Full-time: " +
      resultStr + ". Check the Standings page for updated league tables.')");
}

void LeagueSimulateFixture(const LeagueFixtureInfo& fixture, int& homeGoals, int& awayGoals) {
  // Strength-aware result: the league stores no explicit ratings, so derive a
  // stable rating from each team's name and give the home side an edge.
  auto teamRating = [](const std::string& name) {
    return 50 + static_cast<int>(std::hash<std::string>{}(name) % 41);  // 50..90
  };
  auto sampleGoals = [](int attack, int defense) {
    float expected = 1.35f * static_cast<float>(attack) / static_cast<float>(std::max(1, defense));
    int goals = static_cast<int>(expected);
    float frac = expected - static_cast<float>(goals);
    // Stochastic rounding keeps the expected goal average honest.
    if ((rand() % 1000) / 1000.0f < frac)
      goals += 1;
    // Occasional flair for the odd extra goal.
    if (rand() % 100 < 15)
      goals += rand() % 2;
    return std::max(0, std::min(6, goals));
  };
  int homeRating = teamRating(fixture.homeTeamName) + 6;  // home advantage
  int awayRating = teamRating(fixture.awayTeamName);
  homeGoals = sampleGoals(homeRating, awayRating);
  awayGoals = sampleGoals(awayRating, homeRating);
}

int LeagueResolveMatchday(const std::string& date) {
  auto result = GetDB()->Query(
      "SELECT c.id, c.team1_id, c.team2_id, c.competition_id, t1.name, t2.name, l.name, "
      "date(c.timestamp) " +
      FixtureQueryJoins() +
      "WHERE date(c.timestamp) <= date('" + date + "') "
      "AND c.id NOT IN (SELECT calendar_id FROM match_results WHERE played = 1) "
      "ORDER BY c.timestamp, c.id");

  int simulated = 0;
  for (const auto& row : result->data) {
    LeagueFixtureInfo fixture = FixtureFromRow(row);
    int homeGoals = 0;
    int awayGoals = 0;
    LeagueSimulateFixture(fixture, homeGoals, awayGoals);
    LeagueRecordResult(fixture, homeGoals, awayGoals);
    simulated++;
  }

  if (simulated > 0) {
    // The clock now sits on the resolved matchday; season rollover is handled
    // by the end-of-season flow, not here.
    GetDB()->Query("UPDATE settings SET timestamp = date('" + date + "')");
    g_lastMatchdayDate = date;
  }
  return simulated;
}

std::string LeagueGetLastMatchdayDate() {
  if (!g_lastMatchdayDate.empty()) {
    return g_lastMatchdayDate;
  }
  auto result = GetDB()->Query(
      "SELECT date(MAX(c.timestamp)) FROM calendar c "
      "JOIN match_results mr ON mr.calendar_id = c.id AND mr.played = 1");
  if (result->data.empty() || result->data.at(0).empty()) {
    return "";
  }
  return result->data.at(0).at(0);
}

bool LeagueConsumePlayedFixture(int homeGoals, int awayGoals) {
  if (!g_pendingFixtureActive) {
    return false;
  }

  LeagueFixtureInfo fixture = g_pendingFixture;
  g_pendingFixtureActive = false;

  // Sanity-check the fixture still exists and has not been recorded meanwhile.
  auto existing = GetDB()->Query("SELECT id FROM calendar WHERE id = " +
                                 int_to_str(fixture.calendarID));
  if (existing->data.empty()) {
    return false;
  }
  auto played = GetDB()->Query("SELECT id FROM match_results WHERE calendar_id = " +
                               int_to_str(fixture.calendarID) + " AND played = 1");
  if (!played->data.empty()) {
    return false;
  }

  LeagueRecordResult(fixture, homeGoals, awayGoals);
  LeagueResolveMatchday(fixture.date);
  return true;
}

bool LeagueSeasonComplete() {
  auto result = GetDB()->Query(
      "SELECT COUNT(*) FROM calendar c "
      "WHERE c.id NOT IN (SELECT calendar_id FROM match_results WHERE played = 1)");
  if (result->data.empty() || result->data.at(0).empty()) {
    return false;
  }
  return atoi(result->data.at(0).at(0).c_str()) == 0;
}

bool LeagueAdvanceSeason() {
  auto result = GetDB()->Query("SELECT seasonyear FROM settings LIMIT 1");
  if (result->data.empty() || result->data.at(0).empty()) {
    return false;
  }
  int newYear = atoi(result->data.at(0).at(0).c_str()) + 1;
  std::string startDate = int_to_str(newYear) + "-06-01";

  // The standings are recomputed from match_results, so a new season must
  // start with a clean results sheet.
  GetDB()->Query("DELETE FROM match_results");
  GetDB()->Query("DELETE FROM calendar");
  GetDB()->Query("UPDATE settings SET seasonyear = " + int_to_str(newYear) + ", timestamp = date('" +
                 startDate + "')");
  g_lastMatchdayDate.clear();
  GenerateSeasonCalendars();
  GetDB()->Query(
      "INSERT INTO inbox_messages (sender, subject, body) VALUES "
      "('League Office', 'New season begins', 'A new season is underway! The calendar has been "
      "regenerated and the standings reset. Good luck this year.')");
  return true;
}

bool LeagueSaveToSlot(int slotIndex) {
  if (slotIndex < 1 || slotIndex > kLeagueMaxSaveSlots) {
    return false;
  }
  namespace fs = std::filesystem;
  fs::path dest("saves");
  dest /= GetActiveSaveDirectory();
  std::error_code error;
  fs::copy_file(fs::path("saves") / GetActiveSaveDirectory() / "autosave.sqlite",
                dest / ("slot_" + int_to_str(slotIndex) + ".sqlite"),
                fs::copy_options::overwrite_existing, error);
  return !error;
}

bool LeagueLoadSlot(int slotIndex) {
  if (slotIndex < 1 || slotIndex > kLeagueMaxSaveSlots) {
    return false;
  }
  namespace fs = std::filesystem;
  fs::path slotFile = fs::path("saves") / GetActiveSaveDirectory() /
                      ("slot_" + int_to_str(slotIndex) + ".sqlite");
  if (!fs::exists(slotFile)) {
    return false;
  }

  // The loaded slot becomes the new live autosave.
  fs::path autosave = fs::path("saves") / GetActiveSaveDirectory() / "autosave.sqlite";
  std::error_code error;
  fs::copy_file(slotFile, autosave, fs::copy_options::overwrite_existing, error);
  if (error) {
    return false;
  }
  return GetDB()->Load(autosave.string());
}

int LeagueGetSlotCount() {
  namespace fs = std::filesystem;
  fs::path saveDir = fs::path("saves") / GetActiveSaveDirectory();
  int count = 0;
  for (int i = 1; i <= kLeagueMaxSaveSlots; i++) {
    if (fs::exists(saveDir / ("slot_" + int_to_str(i) + ".sqlite"))) {
      count = i;  // highest occupied slot
    }
  }
  return count;
}
