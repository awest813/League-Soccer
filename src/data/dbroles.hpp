// Football Manager role notation, as stored in players.role ("D C",
// "AM RLC, F C", "D/WB/AM L"), mapped to the game's ten position codes. Pure
// string handling, kept free of the database so it can be unit tested.

#ifndef _HPP_DATA_DBROLES
#define _HPP_DATA_DBROLES

#include <sstream>
#include <string>
#include <vector>

namespace dbroles {

// One position group ("AM RLC") to a code: GK, CB, LB, RB, DM, CM, LM, RM, AM
// or CF. Short codes pass through; unknown notation becomes CM.
inline std::string PositionFromGroup(const std::string& group) {
  std::istringstream tokens(group);
  std::string positions;
  std::string sides;
  tokens >> positions >> sides;
  if (positions.empty())
    return "CM";

  // "D/WB/AM" -> "D": the first listed position is the natural one.
  const std::string position = positions.substr(0, positions.find('/'));
  const bool central = sides.empty() || sides.find('C') != std::string::npos;
  const char flank = sides.empty() ? 'C' : sides[0];
  const std::string flankSuffix = (flank == 'L') ? "L" : "R";

  if (position == "GK")
    return "GK";
  if (position == "SW")
    return "CB";
  if (position == "D")
    return central ? "CB" : flankSuffix + "B";
  if (position == "WB")
    return flankSuffix + "B";
  if (position == "DM")
    return "DM";
  if (position == "M")
    return central ? "CM" : flankSuffix + "M";
  if (position == "AM")
    return central ? "AM" : flankSuffix + "M";
  if (position == "F" || position == "ST")
    return "CF";
  static const char* const kShortCodes[] = {"GK", "CB", "LB", "RB", "DM",
                                            "CM", "LM", "RM", "AM", "CF"};
  for (const char* code : kShortCodes) {
    if (position == code)
      return position;
  }
  return "CM";
}

// The natural position: the first comma-separated group.
inline std::string PositionFromDbRole(const std::string& dbRole) {
  return PositionFromGroup(dbRole.substr(0, dbRole.find(',')));
}

// Every position the notation lists, natural one first, without repeats
// ("SW, D C, DM" -> CB, DM).
inline std::vector<std::string> PositionsFromDbRole(const std::string& dbRole) {
  std::vector<std::string> codes;
  std::string::size_type start = 0;
  while (start <= dbRole.size()) {
    const std::string::size_type comma = dbRole.find(',', start);
    const std::string group =
        dbRole.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
    if (group.find_first_not_of(' ') != std::string::npos) {
      const std::string code = PositionFromGroup(group);
      bool seen = false;
      for (const auto& existing : codes)
        seen = seen || existing == code;
      if (!seen)
        codes.push_back(code);
    }
    if (comma == std::string::npos)
      break;
    start = comma + 1;
  }
  if (codes.empty())
    codes.push_back("CM");
  return codes;
}

}  // namespace dbroles

#endif  // _HPP_DATA_DBROLES
