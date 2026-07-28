// written by bastiaan konings schuiling 2008 - 2015
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not
// be used for anything important. i do not offer support, so don't ask. to be used for inspiration
// :)

#include "leaguecode.hpp"

#define BOOST_FILESYSTEM_VERSION 3

#include <algorithm>
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

bool DatabaseHasColumn(Database* database, const std::string& tableName, const std::string& columnName) {
  auto result = database->Query("PRAGMA table_info(" + tableName + ")");
  for (const auto& row : result->data) {
    if (row.size() > 1 && row.at(1) == columnName) {
      return true;
    }
  }
  return false;
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
        if (!std::filesystem::exists(destfile) &&
            CopyDirectory(sourcefile, destfile) != 0) {
          errorCode = 4;
        }
      } else {
        std::error_code error;
        // printf("copying from %s to %s\n", sourcefile.string().c_str(), destfile.string().c_str());
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

static void GenerateRoundRobinFixtures(int leagueID, int,
                                       const std::string& startDate) {
  auto teamsResult = GetDB()->Query(
      "SELECT id FROM teams WHERE league_id = " + int_to_str(leagueID) + " ORDER BY id");
  if (teamsResult->data.empty()) return;

  std::vector<int> teamIDs;
  for (const auto& row : teamsResult->data) {
    teamIDs.push_back(atoi(row.at(0).c_str()));
  }

  int n = static_cast<int>(teamIDs.size());
  if (n < 2) return;

  bool needsBye = (n % 2 != 0);
  if (needsBye) teamIDs.push_back(-1);
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
          if (roundOffset % 2 == 1) std::swap(t1, t2);
          roundMatches.push_back({t1, t2});
        }
      }

      int effectiveMatchDay = roundOffset * numRounds + round + 1;

      for (const auto& m : roundMatches) {
        GetDB()->Query(
            "INSERT INTO calendar (timestamp, team1_id, team2_id, competition_id, tournament_id) "
            "VALUES (date('" + startDate + "', '+" +
            int_to_str((effectiveMatchDay - 1) * 7) + " day'), " +
            int_to_str(m.team1) + ", " + int_to_str(m.team2) + ", " +
            int_to_str(leagueID) + ", NULL)");
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
  auto result = GetDB()->Query(
      "SELECT timestamp, seasonyear FROM settings LIMIT 1");
  if (result->data.empty() || result->data.at(0).size() < 2) return;

  std::string startDate = result->data.at(0).at(0);

  auto leaguesResult = GetDB()->Query("SELECT id FROM leagues ORDER BY id");
  if (leaguesResult->data.empty()) return;

  int seasonYear = atoi(result->data.at(0).at(1).c_str());

  GetDB()->Query("DELETE FROM calendar");

  for (const auto& row : leaguesResult->data) {
    int leagueID = atoi(row.at(0).c_str());
    GenerateRoundRobinFixtures(leagueID, seasonYear, startDate);
  }

  int totalFixtures = 0;
  auto countResult = GetDB()->Query("SELECT COUNT(*) FROM calendar");
  if (!countResult->data.empty() && !countResult->data.at(0).empty()) {
    totalFixtures = atoi(countResult->data.at(0).at(0).c_str());
  }

  int numWeeks = totalFixtures > 0 ? 20 : 0;
  result = GetDB()->Query("UPDATE settings SET timestamp = date(timestamp, '+" +
                          int_to_str(numWeeks * 7) + " day'), seasonyear = " +
                          int_to_str(seasonYear));
}

bool StepLeagueTime() {
  auto result = GetDB()->Query(
      "SELECT strftime(\"%w\", timestamp), seasonyear FROM settings LIMIT 1");
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
