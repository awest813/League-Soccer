// Minimal match-clock and score-board simulation.
// This module has no SDL / OpenGL dependency and can be used in headless
// unit / integration tests.

#ifndef _HPP_FOOTBALL_ONTHEPITCH_MATCHCLOCK
#define _HPP_FOOTBALL_ONTHEPITCH_MATCHCLOCK

#include <limits>

// Match phases (mirrors e_MatchPhase in gamedefines.hpp)
enum e_MatchPhaseSimple {
  e_MatchPhaseSimple_PreMatch,
  e_MatchPhaseSimple_1stHalf,
  e_MatchPhaseSimple_HalfTime,
  e_MatchPhaseSimple_2ndHalf,
  e_MatchPhaseSimple_FullTime,
  e_MatchPhaseSimple_1stExtraTime,
  e_MatchPhaseSimple_ExtraTimeBreak,
  e_MatchPhaseSimple_2ndExtraTime,
  e_MatchPhaseSimple_Penalties,
  e_MatchPhaseSimple_Final,
};

// Duration of each half in milliseconds (45 minutes)
inline constexpr unsigned long kHalfDuration_ms = 45UL * 60UL * 1000UL;
// Duration of each extra time period in milliseconds (15 minutes)
inline constexpr unsigned long kExtraTimeHalfDuration_ms = 15UL * 60UL * 1000UL;

// A minimal match clock that tracks elapsed game time and phase transitions.
struct MatchClock {
  e_MatchPhaseSimple phase = e_MatchPhaseSimple_PreMatch;

  // Elapsed game time within the current period (ms)
  unsigned long halfTime_ms = 0;

  // Stoppage / injury time added to the current period (ms)
  unsigned long stoppageTime_ms = 0;

  // Total goals per team in regular / extra play: [0] = home, [1] = away
  int goals[2] = {0, 0};

  // Penalty shootout goals: [0] = home, [1] = away
  int penaltyGoals[2] = {0, 0};

  // Whether extra time and penalty shootouts are permitted
  bool allowExtraTime = false;
  bool allowPenalties = false;

  // Start the first half.
  void startMatch() {
    if (phase == e_MatchPhaseSimple_PreMatch) {
      beginPeriod(e_MatchPhaseSimple_1stHalf);
    }
  }

  // Kick off the second half after half-time.
  void startSecondHalf() {
    if (phase == e_MatchPhaseSimple_HalfTime) {
      beginPeriod(e_MatchPhaseSimple_2ndHalf);
    }
  }

  // Start extra time if tied at full-time.
  void startExtraTime() {
    if (phase == e_MatchPhaseSimple_FullTime && allowExtraTime && isTied()) {
      beginPeriod(e_MatchPhaseSimple_1stExtraTime);
    }
  }

  // Kick off second period of extra time after break.
  void startSecondExtraTime() {
    if (phase == e_MatchPhaseSimple_ExtraTimeBreak) {
      beginPeriod(e_MatchPhaseSimple_2ndExtraTime);
    }
  }

  // Begin penalty shootout.
  void startPenalties() {
    if (phase == e_MatchPhaseSimple_FullTime && allowPenalties && !allowExtraTime && isTied()) {
      phase = e_MatchPhaseSimple_Penalties;
    }
  }

  // Record a regular / extra time goal for the given team (0 or 1).
  void addGoal(int teamID) {
    if (isLivePeriod() && (teamID == 0 || teamID == 1)) {
      goals[teamID]++;
    }
  }

  // Record a shootout penalty goal for the given team (0 or 1).
  void addPenaltyGoal(int teamID) {
    if (phase == e_MatchPhaseSimple_Penalties && (teamID == 0 || teamID == 1)) {
      penaltyGoals[teamID]++;
    }
  }

  // The caller decides when all required kicks have been taken; ties cannot conclude.
  void concludePenalties() {
    if (phase == e_MatchPhaseSimple_Penalties && !isPenaltyTied()) {
      phase = e_MatchPhaseSimple_Final;
    }
  }

  // Query tie state
  bool isTied() const { return goals[0] == goals[1]; }
  bool isPenaltyTied() const { return penaltyGoals[0] == penaltyGoals[1]; }

  // Set stoppage time for the active half/period
  void setStoppageTime(unsigned long stoppage_ms) { stoppageTime_ms = stoppage_ms; }

  // Advance the clock by delta_ms. Phase transitions happen automatically.
  // Returns true only on a transition that completes the match, with no shootout pending.
  bool tick(unsigned long delta_ms) {
    if (!isLivePeriod()) {
      return false;
    }

    const unsigned long periodDuration =
        (phase == e_MatchPhaseSimple_1stExtraTime || phase == e_MatchPhaseSimple_2ndExtraTime)
            ? kExtraTimeHalfDuration_ms
            : kHalfDuration_ms;
    const unsigned long targetDuration = saturatedAdd(periodDuration, stoppageTime_ms);

    // A revised stoppage allowance may already have expired. End the period
    // without erasing time that has actually been played.
    if (halfTime_ms < targetDuration) {
      const unsigned long remaining = targetDuration - halfTime_ms;
      halfTime_ms += delta_ms < remaining ? delta_ms : remaining;
    }

    if (halfTime_ms >= targetDuration) {
      if (phase == e_MatchPhaseSimple_1stHalf) {
        phase = e_MatchPhaseSimple_HalfTime;
        stoppageTime_ms = 0;
      } else if (phase == e_MatchPhaseSimple_2ndHalf) {
        phase = e_MatchPhaseSimple_FullTime;
        stoppageTime_ms = 0;
        return !isTied() || (!allowExtraTime && !allowPenalties);
      } else if (phase == e_MatchPhaseSimple_1stExtraTime) {
        phase = e_MatchPhaseSimple_ExtraTimeBreak;
        stoppageTime_ms = 0;
      } else if (phase == e_MatchPhaseSimple_2ndExtraTime) {
        if (allowPenalties && goals[0] == goals[1]) {
          phase = e_MatchPhaseSimple_Penalties;
        } else {
          phase = e_MatchPhaseSimple_Final;
        }
        stoppageTime_ms = 0;
        return phase == e_MatchPhaseSimple_Final;
      }
    }
    return false;
  }

  // Convenience: total elapsed match time (all played periods combined, ms)
  unsigned long totalElapsed_ms() const { return saturatedAdd(completedTime_ms, halfTime_ms); }

private:
  unsigned long completedTime_ms = 0;

  bool isLivePeriod() const {
    return phase == e_MatchPhaseSimple_1stHalf || phase == e_MatchPhaseSimple_2ndHalf ||
           phase == e_MatchPhaseSimple_1stExtraTime || phase == e_MatchPhaseSimple_2ndExtraTime;
  }

  static unsigned long saturatedAdd(unsigned long left, unsigned long right) {
    const auto maximum = (std::numeric_limits<unsigned long>::max)();
    return right > maximum - left ? maximum : left + right;
  }

  void beginPeriod(e_MatchPhaseSimple nextPhase) {
    completedTime_ms = totalElapsed_ms();
    halfTime_ms = 0;
    stoppageTime_ms = 0;
    phase = nextPhase;
  }
};

#endif  // _HPP_FOOTBALL_ONTHEPITCH_MATCHCLOCK
