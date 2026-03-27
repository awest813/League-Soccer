// Minimal match-clock and score-board simulation.
// This module has no SDL / OpenGL dependency and can be used in headless
// unit / integration tests.

#ifndef _HPP_FOOTBALL_ONTHEPITCH_MATCHCLOCK
#define _HPP_FOOTBALL_ONTHEPITCH_MATCHCLOCK

// Match phases (mirrors e_MatchPhase in gamedefines.hpp)
enum e_MatchPhaseSimple {
  e_MatchPhaseSimple_PreMatch,
  e_MatchPhaseSimple_1stHalf,
  e_MatchPhaseSimple_HalfTime,
  e_MatchPhaseSimple_2ndHalf,
  e_MatchPhaseSimple_FullTime,
};

// Duration of each half in milliseconds (45 minutes)
inline constexpr unsigned long kHalfDuration_ms = 45UL * 60UL * 1000UL;

// A minimal match clock that tracks elapsed game time and phase transitions.
struct MatchClock {
  e_MatchPhaseSimple phase = e_MatchPhaseSimple_PreMatch;

  // Elapsed game time within the current half (ms)
  unsigned long halfTime_ms = 0;

  // Total goals per team: [0] = home, [1] = away
  int goals[2] = {0, 0};

  // Start the first half.
  void startMatch() { phase = e_MatchPhaseSimple_1stHalf; }

  // Kick off the second half after half-time.
  void startSecondHalf() {
    if (phase == e_MatchPhaseSimple_HalfTime) {
      phase = e_MatchPhaseSimple_2ndHalf;
      halfTime_ms = 0;
    }
  }

  // Record a goal for the given team (0 or 1).
  void addGoal(int teamID) {
    if (teamID == 0 || teamID == 1) {
      goals[teamID]++;
    }
  }

  // Advance the clock by delta_ms.  Phase transitions happen automatically.
  // Returns true if the match has just reached FullTime.
  bool tick(unsigned long delta_ms) {
    if (phase != e_MatchPhaseSimple_1stHalf && phase != e_MatchPhaseSimple_2ndHalf)
      return false;

    halfTime_ms += delta_ms;

    if (halfTime_ms >= kHalfDuration_ms) {
      halfTime_ms = kHalfDuration_ms;
      if (phase == e_MatchPhaseSimple_1stHalf) {
        phase = e_MatchPhaseSimple_HalfTime;
      } else if (phase == e_MatchPhaseSimple_2ndHalf) {
        phase = e_MatchPhaseSimple_FullTime;
        return true;
      }
    }
    return false;
  }

  // Convenience: total elapsed match time (both halves combined, ms)
  unsigned long totalElapsed_ms() const {
    if (phase == e_MatchPhaseSimple_1stHalf || phase == e_MatchPhaseSimple_HalfTime)
      return halfTime_ms;
    return kHalfDuration_ms + halfTime_ms;
  }
};

#endif  // _HPP_FOOTBALL_ONTHEPITCH_MATCHCLOCK
