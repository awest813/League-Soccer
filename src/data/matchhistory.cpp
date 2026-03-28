#include "matchhistory.hpp"

#include <filesystem>
#include <sstream>

#include "sqlite3.h"

std::string MatchHistory::GetDbPath() {
  return "data/saves/match_history.sqlite";
}

void MatchHistory::EnsureTable() {
  std::filesystem::create_directories("data/saves");

  sqlite3* db = nullptr;
  int rc = sqlite3_open_v2(GetDbPath().c_str(), &db,
                           SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
  if (rc != SQLITE_OK) {
    if (db) sqlite3_close(db);
    return;
  }

  const char* sql =
      "CREATE TABLE IF NOT EXISTS match_history ("
      "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "  timestamp TEXT,"
      "  team1_name TEXT, team2_name TEXT,"
      "  score1 INTEGER, score2 INTEGER,"
      "  match_time_ms INTEGER,"
      "  possession1_pct REAL, possession2_pct REAL,"
      "  shots1 INTEGER, shots2 INTEGER,"
      "  shots_on_target1 INTEGER, shots_on_target2 INTEGER,"
      "  passes1 INTEGER, passes2 INTEGER,"
      "  passes_completed1 INTEGER, passes_completed2 INTEGER,"
      "  fouls1 INTEGER, fouls2 INTEGER"
      ");";

  char* errMsg = nullptr;
  sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
  if (errMsg) sqlite3_free(errMsg);

  sqlite3_close(db);
}

void MatchHistory::SaveMatch(const MatchHistoryEntry& e) {
  sqlite3* db = nullptr;
  int rc = sqlite3_open_v2(GetDbPath().c_str(), &db,
                           SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
  if (rc != SQLITE_OK) {
    if (db) sqlite3_close(db);
    return;
  }

  std::ostringstream sql;
  sql << "INSERT INTO match_history ("
         "timestamp, team1_name, team2_name, score1, score2, match_time_ms,"
         "possession1_pct, possession2_pct, shots1, shots2,"
         "shots_on_target1, shots_on_target2, passes1, passes2,"
         "passes_completed1, passes_completed2, fouls1, fouls2"
         ") VALUES ("
      << "'" << e.timestamp << "',"
      << "'" << e.team1_name << "',"
      << "'" << e.team2_name << "',"
      << e.score1 << "," << e.score2 << "," << e.match_time_ms << ","
      << e.possession1_pct << "," << e.possession2_pct << ","
      << e.shots1 << "," << e.shots2 << ","
      << e.shots_on_target1 << "," << e.shots_on_target2 << ","
      << e.passes1 << "," << e.passes2 << ","
      << e.passes_completed1 << "," << e.passes_completed2 << ","
      << e.fouls1 << "," << e.fouls2 << ");";

  char* errMsg = nullptr;
  sqlite3_exec(db, sql.str().c_str(), nullptr, nullptr, &errMsg);
  if (errMsg) sqlite3_free(errMsg);

  sqlite3_close(db);
}

std::vector<MatchHistoryEntry> MatchHistory::LoadRecent(int limit) {
  std::vector<MatchHistoryEntry> results;

  sqlite3* db = nullptr;
  int rc = sqlite3_open_v2(GetDbPath().c_str(), &db,
                           SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
  if (rc != SQLITE_OK) {
    if (db) sqlite3_close(db);
    return results;
  }

  std::string sql =
      "SELECT id, timestamp, team1_name, team2_name, score1, score2, match_time_ms,"
      "possession1_pct, possession2_pct, shots1, shots2,"
      "shots_on_target1, shots_on_target2, passes1, passes2,"
      "passes_completed1, passes_completed2, fouls1, fouls2 "
      "FROM match_history ORDER BY id DESC LIMIT " +
      std::to_string(limit) + ";";

  char** table = nullptr;
  int rows = 0, cols = 0;
  char* errMsg = nullptr;
  int ret = sqlite3_get_table(db, sql.c_str(), &table, &rows, &cols, &errMsg);
  if (errMsg) sqlite3_free(errMsg);

  if (ret == SQLITE_OK && rows > 0) {
    for (int r = 1; r <= rows; r++) {
      MatchHistoryEntry e;
      int base = r * cols;
      e.id              = table[base + 0]  ? std::stoi(table[base + 0])  : 0;
      e.timestamp       = table[base + 1]  ? table[base + 1]             : "";
      e.team1_name      = table[base + 2]  ? table[base + 2]             : "";
      e.team2_name      = table[base + 3]  ? table[base + 3]             : "";
      e.score1          = table[base + 4]  ? std::stoi(table[base + 4])  : 0;
      e.score2          = table[base + 5]  ? std::stoi(table[base + 5])  : 0;
      e.match_time_ms   = table[base + 6]  ? std::stoi(table[base + 6])  : 0;
      e.possession1_pct = table[base + 7]  ? std::stof(table[base + 7])  : 0;
      e.possession2_pct = table[base + 8]  ? std::stof(table[base + 8])  : 0;
      e.shots1          = table[base + 9]  ? std::stoi(table[base + 9])  : 0;
      e.shots2          = table[base + 10] ? std::stoi(table[base + 10]) : 0;
      e.shots_on_target1= table[base + 11] ? std::stoi(table[base + 11]) : 0;
      e.shots_on_target2= table[base + 12] ? std::stoi(table[base + 12]) : 0;
      e.passes1         = table[base + 13] ? std::stoi(table[base + 13]) : 0;
      e.passes2         = table[base + 14] ? std::stoi(table[base + 14]) : 0;
      e.passes_completed1= table[base + 15] ? std::stoi(table[base + 15]) : 0;
      e.passes_completed2= table[base + 16] ? std::stoi(table[base + 16]) : 0;
      e.fouls1          = table[base + 17] ? std::stoi(table[base + 17]) : 0;
      e.fouls2          = table[base + 18] ? std::stoi(table[base + 18]) : 0;
      results.push_back(e);
    }
    sqlite3_free_table(table);
  }

  sqlite3_close(db);
  return results;
}
