#include "setpiececonfig.hpp"

#include <filesystem>
#include <sstream>

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

  std::string sql =
      "SELECT formation_depth, formation_width FROM set_piece_config WHERE team_id=" +
      std::to_string(teamDatabaseID) +
      " AND set_piece_type=" + std::to_string((int)setPiece) + ";";

  char** table = nullptr;
  int rows = 0, cols = 0;
  char* errMsg = nullptr;
  int ret = sqlite3_get_table(db, sql.c_str(), &table, &rows, &cols, &errMsg);
  if (errMsg) sqlite3_free(errMsg);

  SetPieceParams result = defaults;
  if (ret == SQLITE_OK && rows > 0 && table[2] && table[3]) {
    result.formation_depth = std::stof(table[2]);
    result.formation_width = std::stof(table[3]);
    sqlite3_free_table(table);
  } else if (table) {
    sqlite3_free_table(table);
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

  std::ostringstream sql;
  sql << "INSERT OR REPLACE INTO set_piece_config "
         "(team_id, set_piece_type, formation_depth, formation_width) VALUES ("
      << teamDatabaseID << "," << (int)setPiece << ","
      << params.formation_depth << "," << params.formation_width << ");";

  char* errMsg = nullptr;
  sqlite3_exec(db, sql.str().c_str(), nullptr, nullptr, &errMsg);
  if (errMsg) sqlite3_free(errMsg);

  sqlite3_close(db);
}
