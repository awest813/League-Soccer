#include "database.hpp"

#include "base/log.hpp"
#include "sqlite3.h"

namespace blunted {

Database::Database() : db(0) {}

Database::~Database() {
  if (db)
    sqlite3_close(db);
}

bool Database::Load(const std::string& filename) {
  // close previously opened db
  if (db)
    sqlite3_close(db);
  db = nullptr;

  if (sqlite3_open_v2(filename.c_str(), &db, SQLITE_OPEN_READWRITE, 0) != SQLITE_OK) {
    // SQLite allocates a handle even when opening fails; queries must not use it.
    sqlite3_close(db);
    db = nullptr;
    return false;
  }
  return true;
}

std::unique_ptr<DatabaseResult> Database::Query(const std::string& query) {
  auto dbresult = std::make_unique<DatabaseResult>();
  if (!db) {
    Log(e_Error, "Database", "Query", "No database is open for query: '" + query + "'");
    return dbresult;
  }

  int rows = 0;
  int columns = 0;
  char** result = nullptr;
  char* errorMsg = nullptr;

  const int returnValue = sqlite3_get_table(db, query.c_str(), &result, &rows, &columns, &errorMsg);
  if (returnValue != SQLITE_OK || errorMsg) {
    const std::string errorMsgStr = errorMsg ? errorMsg : sqlite3_errstr(returnValue);
    if (errorMsg)
      sqlite3_free(errorMsg);
    sqlite3_free_table(result);
    Log(e_FatalError, "Database", "Query",
        "SQLite error message: '" + errorMsgStr + "' for query: '" + query + "'");
    return dbresult;
  }

  // header
  for (int i = 0; i < columns; i++) {
    // printf("1st row heading: %s\n", result[i]);
    dbresult->header.push_back(result[i]);
  }

  // data
  for (int r = 0; r < rows; r++) {
    std::vector<std::string> tmpRow;
    for (int c = 0; c < columns; c++) {
      if (result[(r + 1) * columns + c] == nullptr) {
        tmpRow.push_back("");
      } else {
        // printf("data: %s\n", result[(r + 1) * columns + c]);
        tmpRow.push_back(result[(r + 1) * columns + c]);
      }
    }
    dbresult->data.push_back(tmpRow);
  }

  sqlite3_free_table(result);

  return dbresult;
}

}  // namespace blunted
