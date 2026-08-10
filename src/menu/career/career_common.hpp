#ifndef CAREER_COMMON_HPP
#define CAREER_COMMON_HPP

#include <random>
#include <string>
#include <vector>

#include "../../data/careerdata.hpp"

namespace blunted {
namespace CareerCommon {

// Seeded RNG used by deterministic career simulations. Seeding applies to the
// process-wide career sequence and is harmless in normal play.
std::mt19937& Rng();
void SeedRng(unsigned int seed);
int RandomInt(int minValue, int maxValue);
int ClampInt(int value, int minValue, int maxValue);

// Exception-safe numeric parsing. A corrupt or hand-edited save must never
// crash the game on load; bad fields fall back to a default.
int SafeStoi(const std::string& s, int fallback = 0);
long long SafeStoll(const std::string& s, long long fallback = 0);
float SafeStof(const std::string& s, float fallback = 0.0f);

// Splits a '|'-delimited record (empty fields preserved) and strips the field
// separator / newlines from free-text so it cannot corrupt the text format.
std::vector<std::string> SplitPipes(const std::string& s);
std::string Sanitize(const std::string& s);

// Serializes / parses a player to the pipe-delimited record used for the
// roster, free agents, and youth prospects.
std::string PlayerToRecord(const PlayerCareerState& p);
PlayerCareerState PlayerFromRecord(const std::string& val);

// Abstract sink for the cross-cutting career events / board-confidence
// mutations so extracted modules can raise events without knowing about the
// concrete CareerDatabase (which owns persistence-on-major-events).
class CareerEvents {
public:
  virtual ~CareerEvents() = default;
  virtual void AddEvent(const std::string& eventType, const std::string& description,
                        int reputationDelta, bool isMajor) = 0;
  virtual void ModifyBoardConfidence(int delta) = 0;
};

}  // namespace CareerCommon
}  // namespace blunted

#endif  // CAREER_COMMON_HPP
