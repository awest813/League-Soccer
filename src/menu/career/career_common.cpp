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
     << p.fitness << "|" << p.careerGoals << "|" << p.careerAssists << "|" << p.matchesPlayed;
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
  p.preferredPosition = p.position;
  return p;
}

}  // namespace CareerCommon
}  // namespace blunted
