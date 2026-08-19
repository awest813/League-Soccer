#ifndef _HPP_DATABASE
#define _HPP_DATABASE

#include <memory>

#include "defines.hpp"

struct sqlite3;

namespace blunted {

class DatabaseResult;

class Database {
public:
  Database();
  virtual ~Database();

  bool Load(const std::string& filename);
  std::unique_ptr<DatabaseResult> Query(const std::string& query);

protected:
  sqlite3* db;
};

class DatabaseResult {
public:
  std::vector<std::string> header;
  std::vector<std::vector<std::string>> data;
};

}  // namespace blunted

#endif
