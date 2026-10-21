#include "career_common.hpp"

#include <algorithm>
#include <ctime>
#include <random>
#include <sstream>

namespace blunted {
namespace CareerCommon {

namespace {

std::mt19937& CareerRng() {
  static std::mt19937 rng(static_cast<unsigned int>(std::time(nullptr)));
  return rng;
}

}  // namespace

void SeedRng(unsigned int seed) {
  CareerRng().seed(seed);
}

std::mt19937& Rng() {
  return CareerRng();
}

int RandomInt(int minValue, int maxValue) {
  std::uniform_int_distribution<int> dist(minValue, maxValue);
  return dist(CareerRng());
}

int ClampInt(int value, int minValue, int maxValue) {
  return std::max(minValue, std::min(maxValue, value));
}

int SafeStoi(const std::string& s, int fallback) {
  try {
    return std::stoi(s);
  } catch (const std::exception&) {
    return fallback;
  }
}

long long SafeStoll(const std::string& s, long long fallback) {
  try {
    return std::stoll(s);
  } catch (const std::exception&) {
    return fallback;
  }
}

float SafeStof(const std::string& s, float fallback) {
  try {
    return std::stof(s);
  } catch (const std::exception&) {
    return fallback;
  }
}

std::vector<std::string> SplitPipes(const std::string& s) {
  std::vector<std::string> tokens;
  size_t start = 0;
  while (true) {
    size_t bar = s.find('|', start);
    if (bar == std::string::npos) {
      tokens.push_back(s.substr(start));
      break;
    }
    tokens.push_back(s.substr(start, bar - start));
    start = bar + 1;
  }
  return tokens;
}

std::string Sanitize(const std::string& s) {
  std::string out = s;
  for (char& c : out) {
    if (c == '|' || c == '\n' || c == '\r')
      c = ' ';
  }
  return out;
}

std::string PlayerToRecord(const PlayerCareerState& p) {
  std::ostringstream os;
  os << Sanitize(p.name) << "|" << Sanitize(p.position) << "|" << p.age << "|" << p.ovr << "|"
     << p.pot << "|" << p.value << "|" << p.wage << "|" << p.morale << "|" << p.matchForm << "|"
     << p.fitness << "|" << p.careerGoals << "|" << p.careerAssists << "|" << p.matchesPlayed << "|"
     << p.developmentPoints << "|" << static_cast<int>(p.injury)
     << "|" << p.databaseID << "|" << p.playerID << "|" << p.teamID
     << "|" << p.contract.yearsRemaining << "|" << p.contract.wage
     << "|" << p.contract.releaseClause << "|" << p.contract.loanListed
     << "|" << p.contract.transferListed << "|" << static_cast<int>(p.role)
     << "|" << p.isYouth << "|" << p.isPromotedFromAcademy
     << "|" << Sanitize(p.preferredPosition) << "|" << static_cast<int>(p.transferStatus);
  return os.str();
}

PlayerCareerState PlayerFromRecord(const std::string& val) {
  std::vector<std::string> t = SplitPipes(val);
  PlayerCareerState p;
  if (t.size() > 0)
    p.name = t[0];
  if (t.size() > 1)
    p.position = t[1];
  if (t.size() > 2)
    p.age = SafeStoi(t[2]);
  if (t.size() > 3)
    p.ovr = static_cast<int>(SafeStof(t[3]));
  if (t.size() > 4)
    p.pot = static_cast<int>(SafeStof(t[4]));
  if (t.size() > 5)
    p.value = SafeStoll(t[5]);
  if (t.size() > 6)
    p.wage = SafeStoll(t[6]);
  if (t.size() > 7)
    p.morale = SafeStoi(t[7], p.morale);
  if (t.size() > 8)
    p.matchForm = SafeStoi(t[8], p.matchForm);
  if (t.size() > 9)
    p.fitness = SafeStoi(t[9], p.fitness);
  if (t.size() > 10)
    p.careerGoals = SafeStoi(t[10]);
  if (t.size() > 11)
    p.careerAssists = SafeStoi(t[11]);
  if (t.size() > 12)
    p.matchesPlayed = SafeStoi(t[12]);
  if (t.size() > 13)
    p.developmentPoints = ClampInt(SafeStoi(t[13]), 0, 99);
  if (t.size() > 14) {
    int injury = SafeStoi(t[14]);
    if (injury >= 0 && injury <= 3)
      p.injury = static_cast<InjuryStatus>(injury);
  }
  if (t.size() > 15) p.databaseID = SafeStoi(t[15]);
  if (t.size() > 16) p.playerID = SafeStoi(t[16]);
  if (t.size() > 17) p.teamID = SafeStoi(t[17]);
  if (t.size() > 18) p.contract.yearsRemaining = SafeStoi(t[18]);
  if (t.size() > 19) p.contract.wage = SafeStoll(t[19]);
  if (t.size() > 20) p.contract.releaseClause = SafeStoll(t[20]);
  if (t.size() > 21) p.contract.loanListed = SafeStoi(t[21]) != 0;
  if (t.size() > 22) p.contract.transferListed = SafeStoi(t[22]) != 0;
  if (t.size() > 23) p.role = static_cast<ClubRole>(ClampInt(SafeStoi(t[23]), 0, 5));
  if (t.size() > 24) p.isYouth = SafeStoi(t[24]) != 0;
  if (t.size() > 25) p.isPromotedFromAcademy = SafeStoi(t[25]) != 0;
  p.preferredPosition = t.size() > 26 && !t[26].empty() ? t[26] : p.position;
  if (t.size() > 27)
    p.transferStatus = static_cast<TransferStatus>(ClampInt(SafeStoi(t[27]), 0, 4));
  return p;
}

std::string FormatCareerMoney(long long amount) {
  const bool negative = amount < 0;
  unsigned long long value =
      negative ? static_cast<unsigned long long>(-amount) : static_cast<unsigned long long>(amount);
  std::string digits = std::to_string(value);
  std::string grouped;
  int count = 0;
  for (int i = static_cast<int>(digits.size()) - 1; i >= 0; --i) {
    if (count > 0 && count % 3 == 0)
      grouped.push_back(',');
    grouped.push_back(digits[static_cast<size_t>(i)]);
    ++count;
  }
  std::reverse(grouped.begin(), grouped.end());
  return std::string("EUR ") + (negative ? "-" : "") + grouped;
}

std::string PositionFromDbRole(const std::string& dbRole) {
  // "AM RLC, F C" -> "AM RLC"; positions and sides are separated by a space.
  const std::string primary = dbRole.substr(0, dbRole.find(','));
  std::istringstream tokens(primary);
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
  // Already a short code (e.g. "CB", "CF") or unknown notation.
  static const char* const kShortCodes[] = {"GK", "CB", "LB", "RB", "DM", "CM",
                                            "LM", "RM", "AM", "CF"};
  for (const char* code : kShortCodes) {
    if (position == code)
      return position;
  }
  return "CM";
}

}  // namespace CareerCommon
}  // namespace blunted
