// Quoting for text values pasted into SQL. Every SQLite error is fatal in
// Database::Query, so an unescaped apostrophe in a name ("O'Neill") would
// quit the game.

#ifndef _HPP_UTILS_SQLTEXT
#define _HPP_UTILS_SQLTEXT

#include <string>

namespace blunted {

// value as a single-quoted SQL string literal, with embedded quotes doubled.
inline std::string SqlString(const std::string& value) {
  std::string quoted = "'";
  for (char c : value) {
    if (c == '\'')
      quoted += '\'';
    quoted += c;
  }
  quoted += '\'';
  return quoted;
}

}  // namespace blunted

#endif  // _HPP_UTILS_SQLTEXT
