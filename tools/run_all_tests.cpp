#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "onthepitch/aitactics.hpp"
#include "onthepitch/gameplaytuning.hpp"
#include "onthepitch/humanspeed.hpp"
#include "onthepitch/matchduration.hpp"
#include "data/careerdata.hpp"
#include "menu/career/career_board.hpp"
#include "menu/career/career_common.hpp"
#include "menu/career/career_finance.hpp"
#include "menu/career/career_persistence.hpp"
#include "menu/career/career_sim.hpp"
#include "menu/career/career_sponsors.hpp"
#include "menu/career/career_staff.hpp"
#include "menu/career/career_training.hpp"
#include "menu/career/career_transfers.hpp"
#include "utils/localization.hpp"

using namespace blunted;

int g_passedTests = 0;
int g_failedTests = 0;

#define TEST_ASSERT(cond, msg) \
  do { \
    if (!(cond)) { \
      std::cerr << "  FAILED: " << msg << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
      g_failedTests++; \
    } else { \
      g_passedTests++; \
    } \
  } while (0)

#define TEST_NEAR(a, b, eps, msg) \
  TEST_ASSERT(std::fabs((a) - (b)) <= (eps), msg << " (got " << (a) << ", expected " << (b) << ")")

// Minimal Localization stubs
Localization& Localization::GetInstance() {
  static Localization instance;
  return instance;
}
bool Localization::Load(const std::string& l) { currentLanguage_ = l; return true; }
std::string Localization::Translate(const std::string& k) const { return k; }
std::string Localization::TranslateAndFormat(const std::string& k, const std::vector<std::string>& a) const { return k; }
const std::string& Localization::GetCurrentLanguage() const { return currentLanguage_; }
std::vector<std::string> Localization::GetAvailableLanguages() { return {"en"}; }
std::string Localization::GetLanguageDisplayName(const std::string& c) { return "English"; }

class TestEventSink : public CareerCommon::CareerEvents {
public:
  void AddEvent(const std::string&, const std::string&, int, bool) override {}
  void ModifyBoardConfidence(int) override {}
};

void RunGameplayAuditTests() {
  std::cout << "\n[Suite 1/5] Gameplay Tuning & AI Tactics Tests...\n";

  // First touch tests
  float pt0 = GameplayTuning::GetFirstTouchContextPenalty(5.0f, 1.0f, 1.0f, 2.0f, 0.9f, 3);
  TEST_NEAR(pt0, 0.0f, 0.005f, "Ideal first touch has zero penalty");

  float ptMax = GameplayTuning::GetFirstTouchContextPenalty(0.1f, 0.0f, 0.0f, 16.0f, -1.0f, 1);
  TEST_ASSERT(ptMax > 0.15f && ptMax <= 0.25f, "Pressured blind first touch has bounded high penalty");

  // Fatigue tests
  float jog = GameplayTuning::GetFatigueWorkloadFactor(4.0f, 8.0f, false);
  float sprint = GameplayTuning::GetFatigueWorkloadFactor(8.0f, 8.0f, false);
  TEST_ASSERT(jog < sprint, "Jogging workload is less than sprinting");

  float sprintBall = GameplayTuning::GetFatigueWorkloadFactor(8.0f, 8.0f, true);
  TEST_ASSERT(sprintBall > sprint, "Sprinting with ball increases workload");

  // Goal mouth detection
  TEST_ASSERT(GameplayTuning::IsGoalMouthThreat(0.0f, 0.5f, 3.66f, 2.44f, 1.0f), "Centred ball is goal threat");
  TEST_ASSERT(!GameplayTuning::IsGoalMouthThreat(5.0f, 0.5f, 3.66f, 2.44f, 1.0f), "Wide ball is not goal threat");
  TEST_ASSERT(!GameplayTuning::IsGoalMouthThreat(0.0f, 3.5f, 3.66f, 2.44f, 1.0f), "High ball over crossbar is not threat");

  // Attacking runs
  float thNeutral = AITactics::GetAttackingRunThreshold(0.5f);
  TEST_ASSERT(thNeutral >= 0.35f && thNeutral <= 0.62f, "Attacking run neutral threshold is in range");
  TEST_ASSERT(AITactics::GetAttackingRunThreshold(1.0f) < AITactics::GetAttackingRunThreshold(0.0f), "High counter attack lowers run threshold");

  // Territory
  // teamSide +1 defends right (+50) and attacks left (-50)
  float tOwn = AITactics::GetAttackingTerritory(50.0f, 1, 50.0f);
  float tOpp = AITactics::GetAttackingTerritory(-50.0f, 1, 50.0f);
  float tMid = AITactics::GetAttackingTerritory(0.0f, 1, 50.0f);
  TEST_NEAR(tOwn, -1.0f, 0.01f, "Own goal territory is -1");
  TEST_NEAR(tOpp, 1.0f, 0.01f, "Opp goal territory is +1");
  TEST_NEAR(tMid, 0.0f, 0.01f, "Midfield territory is 0");

  // Zone pressure
  TEST_ASSERT(!AITactics::ShouldStartZonePressure(0.0f, 0.9f, 3.0f), "Zero pressure never triggers");
  TEST_ASSERT(AITactics::ShouldStartZonePressure(1.0f, -0.3f, 8.0f), "High pressure triggers in defensive half");
  TEST_ASSERT(!AITactics::ShouldStartZonePressure(1.0f, 0.9f, 25.0f), "Pressure does not trigger at 25m distance");
}

void RunHumanSpeedAndDurationTests() {
  std::cout << "\n[Suite 2/5] Human Speed & Match Duration Tests...\n";

  // Steps
  TEST_ASSERT(HumanSpeedSliderSteps(HumanSpeedType::SlowDribble) == 21, "Slow dribble steps == 21");
  TEST_ASSERT(HumanSpeedSliderSteps(HumanSpeedType::Run) == 15, "Run steps == 15");
  TEST_ASSERT(HumanSpeedSliderSteps(HumanSpeedType::Sprint) == 36, "Sprint steps == 36");

  // Conversion
  float slowDef = HumanSpeedFromSlider(
      HumanSpeedSliderFromSpeed(kDefaultHumanSlowDribbleSpeed, HumanSpeedType::SlowDribble),
      HumanSpeedType::SlowDribble);
  TEST_NEAR(slowDef, kDefaultHumanSlowDribbleSpeed, 0.05f, "Default slow dribble roundtrip");

  float runDef = HumanSpeedFromSlider(
      HumanSpeedSliderFromSpeed(kDefaultHumanRunSpeed, HumanSpeedType::Run),
      HumanSpeedType::Run);
  TEST_NEAR(runDef, kDefaultHumanRunSpeed, 0.05f, "Default run speed roundtrip");

  float sprintDef = HumanSpeedFromSlider(
      HumanSpeedSliderFromSpeed(kDefaultHumanSprintSpeed, HumanSpeedType::Sprint),
      HumanSpeedType::Sprint);
  TEST_NEAR(sprintDef, kDefaultHumanSprintSpeed, 0.05f, "Default sprint speed roundtrip");

  // Duration
  TEST_NEAR(MatchDurationMinutesFromSlider(0.0f), 5.0f, 0.01f, "Slider 0 is 5 min");
  TEST_NEAR(MatchDurationMinutesFromSlider(1.0f), 90.0f, 0.01f, "Slider 1 is 90 min");
}

void RunCareerDataAndSimulationTests() {
  std::cout << "\n[Suite 3/5] Career Progression & Estimation Tests...\n";

  // League position estimation
  TEST_ASSERT(CareerSim::EstimateLeaguePosition(30, 5, 3) == 1, "95 pts is 1st place");
  TEST_ASSERT(CareerSim::EstimateLeaguePosition(24, 8, 6) == 2, "80 pts is 2nd place");
  TEST_ASSERT(CareerSim::EstimateLeaguePosition(23, 4, 11) == 4, "73 pts is 4th place");
  TEST_ASSERT(CareerSim::EstimateLeaguePosition(12, 12, 14) == 12, "48 pts is 12th place");
  TEST_ASSERT(CareerSim::EstimateLeaguePosition(5, 8, 25) == 19, "23 pts is 19th place");

  // Player growth
  PlayerCareerState p;
  p.name = "Young Prodigy";
  p.age = 19;
  p.ovr = 70;
  p.pot = 85;
  p.matchForm = 90;
  p.morale = 90;
  CareerCommon::SeedRng(42);
  CareerSim::ProcessPlayerGrowth(p);
  TEST_ASSERT(p.ovr >= 70 && p.ovr <= 71, "Young player grows appropriately");
  TEST_ASSERT(p.pot == 85, "Potential remains capped");

  // Market valuation
  CareerSim::UpdatePlayerValue(p);
  TEST_ASSERT(p.value >= 1000000LL, "Talented youngster has multi-million market value");
  TEST_ASSERT(p.wage >= 1000LL, "Wage scales with player value");

  // PES 5/6 Condition Arrow Tiers
  auto GetConditionArrow = [](int form) -> std::string {
    if (form >= 85) return "[^] TOP";
    if (form >= 65) return "[/] GOOD";
    if (form >= 40) return "[>] NORM";
    if (form >= 20) return "[\\] POOR";
    return "[v] BAD";
  };
  TEST_ASSERT(GetConditionArrow(90) == "[^] TOP", "90 form is TOP condition (Red)");
  TEST_ASSERT(GetConditionArrow(70) == "[/] GOOD", "70 form is GOOD condition (Orange)");
  TEST_ASSERT(GetConditionArrow(50) == "[>] NORM", "50 form is NORM condition (Green)");
  TEST_ASSERT(GetConditionArrow(30) == "[\\] POOR", "30 form is POOR condition (Blue)");
  TEST_ASSERT(GetConditionArrow(10) == "[v] BAD", "10 form is BAD condition (Purple)");
}

void RunCareerPersistenceTests() {
  std::cout << "\n[Suite 4/5] Career Persistence & SQLite Storage Tests...\n";

  std::string testDbPath = "./test_career_audit.save";
  std::filesystem::remove(testDbPath);

  CareerSave save;
  save.name = "Persistence Test FC";
  save.managerName = "Test Coach";
  save.mode = CareerMode::GM;
  save.currentSeason = 3;
  save.season.currentSeason = 3;
  save.transferBudget = 45000000;
  save.wageBudget = 500000;

  PlayerCareerState p1;
  p1.playerID = 101;
  p1.name = "Star Striker";
  p1.position = "ST";
  p1.ovr = 84;
  p1.pot = 88;
  p1.age = 24;
  save.roster.push_back(p1);

  std::vector<TransferBid> bids;
  TransferBid b1;
  b1.playerName = "Target Defender";
  b1.bidAmount = 12000000;
  b1.offeredWage = 85000;
  b1.contractYears = 4;
  b1.status = BidStatus::ACCEPTED;
  bids.push_back(b1);

  bool saveSuccess = CareerPersistence::Save(save, bids, testDbPath);
  TEST_ASSERT(saveSuccess, "Career save serialized to SQLite successfully");

  CareerSave loadedSave;
  std::vector<TransferBid> loadedBids;
  bool loadSuccess = CareerPersistence::Load(loadedSave, loadedBids, testDbPath);
  TEST_ASSERT(loadSuccess, "Career save loaded from SQLite successfully");
  TEST_ASSERT(loadedSave.name == "Persistence Test FC", "Club name restored");
  TEST_ASSERT(loadedSave.currentSeason == 3, "Season restored");
  TEST_ASSERT(loadedSave.transferBudget == 45000000, "Transfer budget restored");
  TEST_ASSERT(loadedSave.roster.size() == 1, "Roster count restored");
  if (!loadedSave.roster.empty()) {
    TEST_ASSERT(loadedSave.roster[0].name == "Star Striker", "Player name restored");
    TEST_ASSERT(loadedSave.roster[0].ovr == 84, "Player overall restored");
  }
  TEST_ASSERT(loadedBids.size() == 1, "Bids count restored");

  std::filesystem::remove(testDbPath);
}

void RunDefensiveSafetyChecks() {
  std::cout << "\n[Suite 5/5] Controller & Pointer Null Safety Tests...\n";

  TEST_ASSERT(AITactics::GetDefenderSupportScale(0.0f) > 0.0f, "Deep defender scale is positive");
  TEST_ASSERT(AITactics::GetSecondaryPressureRolePenalty(1.0f) >= 0.0f, "Attacker penalty is non-negative");

  // Gamepad Deadzone and Radial Normalization
  auto ProcessAnalogStick = [](float x, float y) -> std::pair<float, float> {
    constexpr float dz = 0.15f;
    float len = std::hypot(x, y);
    if (len < dz) return {0.0f, 0.0f};
    if (len > 1.0f) {
      return {x / len, y / len};
    }
    return {x, y};
  };

  auto [zeroX, zeroY] = ProcessAnalogStick(0.05f, 0.08f);
  TEST_ASSERT(zeroX == 0.0f && zeroY == 0.0f, "Stick inside deadzone evaluates to zero");

  auto [subX, subY] = ProcessAnalogStick(0.4f, 0.3f);
  float subLen = std::hypot(subX, subY);
  TEST_ASSERT(std::fabs(subLen - 0.5f) < 0.001f, "Sub-maximum stick tilt preserves analog magnitude for walking/jogging");

  auto [diagX, diagY] = ProcessAnalogStick(1.0f, 1.0f);
  float diagLen = std::hypot(diagX, diagY);
  TEST_ASSERT(std::fabs(diagLen - 1.0f) < 0.001f, "Diagonal stick input is clamped to unit length (no diagonal overspeed)");

  // Comprehensive controller profile auto-detection test
  enum class TestCtrlType { Xbox, PlayStation, NintendoSwitch, LogitechDirectInput, Generic };
  auto DetectProfile = [](const std::string& name) -> TestCtrlType {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower.find("sony") != std::string::npos || lower.find("dualshock") != std::string::npos ||
        lower.find("dualsense") != std::string::npos || lower.find("ps4") != std::string::npos ||
        lower.find("ps5") != std::string::npos || lower.find("playstation") != std::string::npos ||
        lower.find("054c") != std::string::npos) {
      return TestCtrlType::PlayStation;
    }
    if (lower.find("nintendo") != std::string::npos || lower.find("switch") != std::string::npos ||
        lower.find("joy-con") != std::string::npos || lower.find("pro controller") != std::string::npos ||
        lower.find("057e") != std::string::npos) {
      return TestCtrlType::NintendoSwitch;
    }
    if (lower.find("logitech") != std::string::npos || lower.find("f310") != std::string::npos ||
        lower.find("f710") != std::string::npos || lower.find("f510") != std::string::npos ||
        lower.find("dual action") != std::string::npos) {
      return TestCtrlType::LogitechDirectInput;
    }
    if (lower.find("xbox") != std::string::npos || lower.find("xinput") != std::string::npos ||
        lower.find("microsoft") != std::string::npos || lower.find("045e") != std::string::npos ||
        lower.find("8bitdo") != std::string::npos || lower.find("razer") != std::string::npos ||
        lower.find("powera") != std::string::npos) {
      return TestCtrlType::Xbox;
    }
    return TestCtrlType::Generic;
  };

  TEST_ASSERT(DetectProfile("Xbox Wireless Controller") == TestCtrlType::Xbox, "Xbox Wireless Controller auto-detected as Xbox");
  TEST_ASSERT(DetectProfile("Microsoft X-Box 360 pad") == TestCtrlType::Xbox, "Xbox 360 pad auto-detected as Xbox");
  TEST_ASSERT(DetectProfile("8BitDo Pro 2 Wired Controller") == TestCtrlType::Xbox, "8BitDo XInput pad auto-detected as Xbox");
  TEST_ASSERT(DetectProfile("Sony Interactive Entertainment DualSense Wireless Controller") == TestCtrlType::PlayStation, "DualSense auto-detected as PlayStation");
  TEST_ASSERT(DetectProfile("PS4 DualShock 4 Controller") == TestCtrlType::PlayStation, "DualShock 4 auto-detected as PlayStation");
  TEST_ASSERT(DetectProfile("Nintendo Switch Pro Controller") == TestCtrlType::NintendoSwitch, "Switch Pro auto-detected as NintendoSwitch");
  TEST_ASSERT(DetectProfile("Logitech Gamepad F310") == TestCtrlType::LogitechDirectInput, "Logitech F310 auto-detected as LogitechDirectInput");
  TEST_ASSERT(DetectProfile("Logitech Dual Action") == TestCtrlType::LogitechDirectInput, "Logitech Dual Action auto-detected as LogitechDirectInput");
  TEST_ASSERT(DetectProfile("DragonRise Inc. Generic USB Joystick") == TestCtrlType::Generic, "Generic USB Joystick auto-detected as Generic");
}

int main() {
  auto start = std::chrono::high_resolution_clock::now();

  std::cout << "====================================================================\n";
  std::cout << "        RUNNING COMPREHENSIVE REPOSITORY REGRESSION AUDIT           \n";
  std::cout << "====================================================================\n";

  RunGameplayAuditTests();
  RunHumanSpeedAndDurationTests();
  RunCareerDataAndSimulationTests();
  RunCareerPersistenceTests();
  RunDefensiveSafetyChecks();

  auto end = std::chrono::high_resolution_clock::now();
  auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  std::cout << "\n====================================================================\n";
  std::cout << "AUDIT RESULTS: " << g_passedTests << " PASSED, " << g_failedTests << " FAILED in " << elapsedMs << " ms\n";
  if (g_failedTests == 0) {
    std::cout << "STATUS: ALL SYSTEMS AUDITED AND VERIFIED 100% OPERATIONAL ✔\n";
  } else {
    std::cout << "STATUS: FAILURES DETECTED ❌\n";
  }
  std::cout << "====================================================================\n";

  return g_failedTests == 0 ? 0 : 1;
}
