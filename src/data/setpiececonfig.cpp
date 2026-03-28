#include "setpiececonfig.hpp"

#include <filesystem>

#include "sqlite3.h"

std::string SetPieceConfig::GetDbPath() {
  return "data/saves/set_piece_config.sqlite";
}

void SetPieceConfig::EnsureTable(int /*teamDatabaseID*/) {
  std::filesystem::create_directories("data/saves");

  sqlite3* db = nullptr;
  int rc = sqlite3_open_v2(GetDbPath().c_str(), &db,
                           SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
  if (rc != SQLITE_OK) {
    if (db) sqlite3_close(db);
    return;
  }

  const char* sql =
      "CREATE TABLE IF NOT EXISTS set_piece_config ("
      "  team_id INTEGER,"
      "  set_piece_type INTEGER,"
      "  formation_depth REAL,"
      "  formation_width REAL,"
      "  PRIMARY KEY (team_id, set_piece_type)"
      ");";

  char* errMsg = nullptr;
  sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
  if (errMsg) sqlite3_free(errMsg);

  sqlite3_close(db);
}

SetPieceParams SetPieceConfig::Load(int teamDatabaseID, e_SetPiece setPiece) {
  SetPieceParams defaults;
  defaults.formation_depth = 0.45f;
  defaults.formation_width = 0.95f;

  sqlite3* db = nullptr;
  int rc = sqlite3_open_v2(GetDbPath().c_str(), &db,
                           SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
  if (rc != SQLITE_OK) {
    if (db) sqlite3_close(db);
    return defaults;
  }

  const char* sql =
      "SELECT formation_depth, formation_width FROM set_piece_config "
      "WHERE team_id=? AND set_piece_type=?;";

  sqlite3_stmt* stmt = nullptr;
  SetPieceParams result = defaults;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, teamDatabaseID);
    sqlite3_bind_int(stmt, 2, static_cast<int>(setPiece));
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      result.formation_depth = static_cast<float>(sqlite3_column_double(stmt, 0));
      result.formation_width = static_cast<float>(sqlite3_column_double(stmt, 1));
    }
    sqlite3_finalize(stmt);
  }

  sqlite3_close(db);
  return result;
}

void SetPieceConfig::Save(int teamDatabaseID, e_SetPiece setPiece,
                          const SetPieceParams& params) {
  sqlite3* db = nullptr;
  int rc = sqlite3_open_v2(GetDbPath().c_str(), &db,
                           SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
  if (rc != SQLITE_OK) {
    if (db) sqlite3_close(db);
    return;
  }

  const char* saveSql =
      "INSERT OR REPLACE INTO set_piece_config "
      "(team_id, set_piece_type, formation_depth, formation_width) VALUES (?,?,?,?);";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, saveSql, -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, teamDatabaseID);
    sqlite3_bind_int(stmt, 2, static_cast<int>(setPiece));
    sqlite3_bind_double(stmt, 3, params.formation_depth);
    sqlite3_bind_double(stmt, 4, params.formation_width);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
  }

  sqlite3_close(db);
}
