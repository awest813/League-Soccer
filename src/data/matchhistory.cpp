#include "matchhistory.hpp"

#include <filesystem>

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

  const char* sql =
      "INSERT INTO match_history ("
      "timestamp, team1_name, team2_name, score1, score2, match_time_ms,"
      "possession1_pct, possession2_pct, shots1, shots2,"
      "shots_on_target1, shots_on_target2, passes1, passes2,"
      "passes_completed1, passes_completed2, fouls1, fouls2"
      ") VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, e.timestamp.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, e.team1_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, e.team2_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, e.score1);
    sqlite3_bind_int(stmt, 5, e.score2);
    sqlite3_bind_int(stmt, 6, e.match_time_ms);
    sqlite3_bind_double(stmt, 7, e.possession1_pct);
    sqlite3_bind_double(stmt, 8, e.possession2_pct);
    sqlite3_bind_int(stmt, 9, e.shots1);
    sqlite3_bind_int(stmt, 10, e.shots2);
    sqlite3_bind_int(stmt, 11, e.shots_on_target1);
    sqlite3_bind_int(stmt, 12, e.shots_on_target2);
    sqlite3_bind_int(stmt, 13, e.passes1);
    sqlite3_bind_int(stmt, 14, e.passes2);
    sqlite3_bind_int(stmt, 15, e.passes_completed1);
    sqlite3_bind_int(stmt, 16, e.passes_completed2);
    sqlite3_bind_int(stmt, 17, e.fouls1);
    sqlite3_bind_int(stmt, 18, e.fouls2);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
  }

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

  const char* sql =
      "SELECT id, timestamp, team1_name, team2_name, score1, score2, match_time_ms,"
      "possession1_pct, possession2_pct, shots1, shots2,"
      "shots_on_target1, shots_on_target2, passes1, passes2,"
      "passes_completed1, passes_completed2, fouls1, fouls2 "
      "FROM match_history ORDER BY id DESC LIMIT ?;";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, limit);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      MatchHistoryEntry e;
      e.id               = sqlite3_column_int(stmt, 0);
      const char* ts     = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
      e.timestamp        = ts ? ts : "";
      const char* t1     = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
      e.team1_name       = t1 ? t1 : "";
      const char* t2     = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
      e.team2_name       = t2 ? t2 : "";
      e.score1           = sqlite3_column_int(stmt, 4);
      e.score2           = sqlite3_column_int(stmt, 5);
      e.match_time_ms    = sqlite3_column_int(stmt, 6);
      e.possession1_pct  = static_cast<float>(sqlite3_column_double(stmt, 7));
      e.possession2_pct  = static_cast<float>(sqlite3_column_double(stmt, 8));
      e.shots1           = sqlite3_column_int(stmt, 9);
      e.shots2           = sqlite3_column_int(stmt, 10);
      e.shots_on_target1 = sqlite3_column_int(stmt, 11);
      e.shots_on_target2 = sqlite3_column_int(stmt, 12);
      e.passes1          = sqlite3_column_int(stmt, 13);
      e.passes2          = sqlite3_column_int(stmt, 14);
      e.passes_completed1= sqlite3_column_int(stmt, 15);
      e.passes_completed2= sqlite3_column_int(stmt, 16);
      e.fouls1           = sqlite3_column_int(stmt, 17);
      e.fouls2           = sqlite3_column_int(stmt, 18);
      results.push_back(e);
    }
    sqlite3_finalize(stmt);
  }

  sqlite3_close(db);
  return results;
}
