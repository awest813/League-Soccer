#ifndef _HPP_DATABASE
#define _HPP_DATABASE

#include <memory>

#include "defines.hpp"
#include "sqltext.hpp"

struct sqlite3;

namespace blunted {

class DatabaseResult;

class Database {
public:
  Database();
  virtual ~Database();

  bool Load(const std::string& filename);
  // Runs one or more statements. On an SQLite error the error is logged and
  // kept in GetLastError(), and the result has no rows.
  std::unique_ptr<DatabaseResult> Query(const std::string& query);
  // The error from the last Query, or empty if it succeeded.
  const std::string& GetLastError() const { return lastError; }

protected:
  sqlite3* db;
  std::string lastError;
};

class DatabaseResult {
public:
  std::vector<std::string> header;
  std::vector<std::vector<std::string>> data;
};

}  // namespace blunted

#endif
