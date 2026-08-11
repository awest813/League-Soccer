#include <gtest/gtest.h>

#include "onthepitch/aitactics.hpp"
#include "onthepitch/gameplaytuning.hpp"
#include "onthepitch/matchclock.hpp"
#include "onthepitch/matchduration.hpp"

// Integration tests that simulate a complete 90-minute match using the
// headless MatchClock, verifying phase transitions and score tracking.

namespace {

TEST(AITacticsTest, CounterAttackMakesRunsEarlierAndLastLonger) {
  EXPECT_GT(AITactics::GetAttackingRunThreshold(0.0f), AITactics::GetAttackingRunThreshold(1.0f));
  EXPECT_LT(AITactics::GetAttackingRunDuration_ms(0.0f),
            AITactics::GetAttackingRunDuration_ms(1.0f));
}

TEST(AITacticsTest, ZonePressureIsSelectiveBySettingTerritoryAndDistance) {
  EXPECT_FALSE(AITactics::ShouldStartZonePressure(0.0f, 1.0f, 1.0f));
  EXPECT_FALSE(AITactics::ShouldStartZonePressure(0.5f, -0.5f, 5.0f));
  EXPECT_FALSE(AITactics::ShouldStartZonePressure(0.5f, 0.2f, 20.0f));
  EXPECT_TRUE(AITactics::ShouldStartZonePressure(0.5f, 0.2f, 10.0f));
  EXPECT_TRUE(AITactics::ShouldStartZonePressure(1.0f, -0.3f, 15.0f));
}

TEST(AITacticsTest, TerritoryUsesTheTeamsAttackingDirection) {
  EXPECT_FLOAT_EQ(AITactics::GetAttackingTerritory(-52.5f, 1, 52.5f), 1.0f);
  EXPECT_FLOAT_EQ(AITactics::GetAttackingTerritory(52.5f, -1, 52.5f), 1.0f);
  EXPECT_FLOAT_EQ(AITactics::GetAttackingTerritory(0.0f, 1, 52.5f), 0.0f);
}

TEST(AITacticsTest, SupportPassRequiresPressureAndClearerSpace) {
  EXPECT_FALSE(AITactics::ShouldConsiderSupportPass(0.5f, 0.8f, 0.45f, 0.95f, 0.1f));
  EXPECT_FALSE(AITactics::ShouldConsiderSupportPass(0.5f, 0.3f, 0.2f, 0.8f, 0.8f));
  EXPECT_TRUE(AITactics::ShouldConsiderSupportPass(0.5f, 0.3f, 0.4f, 0.6f, 0.2f));
  EXPECT_GT(AITactics::GetSupportPassBonus(0.3f, 0.7f, 0.2f), 0.0f);
}

TEST(AITacticsTest, DribbleDirectnessHasMeaningfulTacticalRange) {
  EXPECT_LT(AITactics::GetDribbleForwardDrive(0.0f, 0.5f),
            AITactics::GetDribbleForwardDrive(1.0f, 0.5f));
  EXPECT_NEAR(AITactics::GetDribbleForwardDrive(0.5f, 0.5f), 0.78f, 0.001f);
}

TEST(AITacticsTest, SupportDistancePreservesNeutralSpacingAndOffersSubtleRange) {
  EXPECT_FLOAT_EQ(AITactics::GetSupportWebScale(0.5f), 0.75f);
  EXPECT_LT(AITactics::GetSupportWebScale(0.0f), AITactics::GetSupportWebScale(0.5f));
  EXPECT_GT(AITactics::GetSupportWebScale(1.0f), AITactics::GetSupportWebScale(0.5f));
}

TEST(AITacticsTest, CentreBacksRemainMoreDisciplinedThanFullBacks) {
  EXPECT_LT(AITactics::GetDefenderSupportScale(0.0f), AITactics::GetDefenderSupportScale(0.25f));
  EXPECT_NEAR(AITactics::GetDefenderSupportScale(0.0f), 0.72f, 0.001f);
  EXPECT_FLOAT_EQ(AITactics::GetDefenderSupportScale(1.0f), 1.0f);
}

TEST(AITacticsTest, SecondaryPressureFavoursAdvancedRolesAtSimilarDistance) {
  EXPECT_GT(AITactics::GetSecondaryPressureRolePenalty(0.0f),
            AITactics::GetSecondaryPressureRolePenalty(0.5f));
  EXPECT_GT(AITactics::GetSecondaryPressureRolePenalty(0.5f),
            AITactics::GetSecondaryPressureRolePenalty(1.0f));
  EXPECT_FLOAT_EQ(AITactics::GetSecondaryPressureRolePenalty(1.0f), 0.0f);
}

TEST(GameplayTuningTest, FirstTouchPenaltyRespondsToPressureAndBlindSidePace) {
  const float composedFrontTouch =
      GameplayTuning::GetFirstTouchContextPenalty(2.0f, 0.9f, 0.9f, 4.0f, 1.0f);
  const float pressuredBlindTouch =
      GameplayTuning::GetFirstTouchContextPenalty(0.5f, 0.4f, 0.4f, 12.0f, -1.0f);
  EXPECT_FLOAT_EQ(composedFrontTouch, 0.0f);
  EXPECT_GT(pressuredBlindTouch, composedFrontTouch);
  EXPECT_LE(pressuredBlindTouch, 0.14f);
}

TEST(GameplayTuningTest, RepeatedSprintingCostsMoreThanMeasuredJogging) {
  EXPECT_LT(GameplayTuning::GetFatigueWorkloadFactor(4.0f, 8.0f, false), 1.0f);
  EXPECT_GT(GameplayTuning::GetFatigueWorkloadFactor(8.0f, 8.0f, false), 1.0f);
  EXPECT_GT(GameplayTuning::GetFatigueWorkloadFactor(8.0f, 8.0f, true),
            GameplayTuning::GetFatigueWorkloadFactor(8.0f, 8.0f, false));
}

TEST(GameplayTuningTest, KeeperThreatDetectionRejectsBallsOverTheBar) {
  EXPECT_TRUE(GameplayTuning::IsGoalMouthThreat(2.0f, 1.5f, 3.7f, 2.5f, 1.0f));
  EXPECT_FALSE(GameplayTuning::IsGoalMouthThreat(2.0f, 3.0f, 3.7f, 2.5f, 1.0f));
  EXPECT_FALSE(GameplayTuning::IsGoalMouthThreat(4.0f, 1.5f, 3.7f, 2.5f, 1.0f));
  EXPECT_TRUE(GameplayTuning::IsGoalMouthThreat(3.8f, 2.55f, 3.7f, 2.5f, 1.1f));
}

TEST(MatchDurationTest, SliderUsesFiveMinuteStepsFromFiveToNinety) {
  EXPECT_FLOAT_EQ(MatchDurationMinutesFromSlider(0.0f), 5.0f);
  EXPECT_FLOAT_EQ(MatchDurationMinutesFromSlider(4.0f / 17.0f), 25.0f);
  EXPECT_FLOAT_EQ(MatchDurationMinutesFromSlider(1.0f), 90.0f);
}

TEST(MatchDurationTest, DurationFactorMatchesRealActivePlayTime) {
  EXPECT_FLOAT_EQ(MatchDurationFactorFromMinutes(25.0f), 25.0f / 90.0f);
  EXPECT_FLOAT_EQ(MatchDurationFactorFromMinutes(90.0f), 1.0f);

  const float realSecondsPerHalfAt25Minutes =
      (45.0f * 60.0f) * MatchDurationFactorFromMinutes(25.0f);
  EXPECT_FLOAT_EQ(realSecondsPerHalfAt25Minutes, 12.5f * 60.0f);
}

TEST(MatchDurationTest, FractionalTickDurationsDoNotDrift) {
  for (int minutes = 5; minutes <= 90; minutes += 5) {
    const int tickCount = minutes * 60 * 100;
    double gameTime_ms = 0.0;
    for (int tick = 0; tick < tickCount; ++tick) {
      gameTime_ms += MatchDurationGameTimeFromRealMilliseconds(10.0, static_cast<float>(minutes));
    }
    EXPECT_NEAR(gameTime_ms, 2.0 * kHalfDuration_ms, 0.01) << minutes << " minute setting";
  }
}

TEST(MatchDurationTest, MigratesLegacySliderUsingAdvertisedRange) {
  EXPECT_FLOAT_EQ(MatchDurationMinutesFromLegacySlider(0.0f), 5.0f);
  EXPECT_FLOAT_EQ(MatchDurationMinutesFromLegacySlider(0.5f), 15.0f);
  EXPECT_FLOAT_EQ(MatchDurationMinutesFromLegacySlider(1.0f), 25.0f);
}

// ---------------------------------------------------------------------------
// Helper: run the clock forward by 'total_ms' in one or more ticks
// ---------------------------------------------------------------------------
static void advanceClock(MatchClock& clock, unsigned long total_ms,
                         unsigned long tickSize_ms = 10000UL) {
  unsigned long remaining = total_ms;
  while (remaining > 0) {
    unsigned long step = (remaining < tickSize_ms) ? remaining : tickSize_ms;
    clock.tick(step);
    remaining -= step;
  }
}

// ---------------------------------------------------------------------------
// Basic phase transitions
// ---------------------------------------------------------------------------

TEST(MatchIntegrationTest, StartsInPreMatch) {
  MatchClock clock;
  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_PreMatch);
}

TEST(MatchIntegrationTest, TransitionsToHalfTimeAfter45Minutes) {
  MatchClock clock;
  clock.startMatch();
  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_1stHalf);

  advanceClock(clock, kHalfDuration_ms);
  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_HalfTime);
}

TEST(MatchIntegrationTest, TransitionsToFullTimeAfter90Minutes) {
  MatchClock clock;
  clock.startMatch();
  advanceClock(clock, kHalfDuration_ms);

  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_HalfTime);

  clock.startSecondHalf();
  advanceClock(clock, kHalfDuration_ms);

  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_FullTime);
}

TEST(MatchIntegrationTest, TotalElapsedTimeIs90MinutesAtFullTime) {
  MatchClock clock;
  clock.startMatch();
  advanceClock(clock, kHalfDuration_ms);
  clock.startSecondHalf();
  advanceClock(clock, kHalfDuration_ms);

  EXPECT_EQ(clock.totalElapsed_ms(), 2UL * kHalfDuration_ms);
}

// ---------------------------------------------------------------------------
// Score tracking
// ---------------------------------------------------------------------------

TEST(MatchIntegrationTest, InitialScoreIsZeroZero) {
  MatchClock clock;
  EXPECT_EQ(clock.goals[0], 0);
  EXPECT_EQ(clock.goals[1], 0);
}

TEST(MatchIntegrationTest, GoalsScoredDuringMatchAreRecorded) {
  MatchClock clock;
  clock.startMatch();

  // Goal at ~20 minutes for team 0
  advanceClock(clock, 20UL * 60UL * 1000UL);
  clock.addGoal(0);

  EXPECT_EQ(clock.goals[0], 1);
  EXPECT_EQ(clock.goals[1], 0);

  // Rest of first half
  advanceClock(clock, kHalfDuration_ms - 20UL * 60UL * 1000UL);
  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_HalfTime);

  clock.startSecondHalf();

  // Goal at ~70 minutes (i.e. 25 min into 2nd half) for team 1
  advanceClock(clock, 25UL * 60UL * 1000UL);
  clock.addGoal(1);
  EXPECT_EQ(clock.goals[1], 1);

  // Complete 2nd half
  advanceClock(clock, kHalfDuration_ms - 25UL * 60UL * 1000UL);
  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_FullTime);

  // Final score: 1-1
  EXPECT_EQ(clock.goals[0], 1);
  EXPECT_EQ(clock.goals[1], 1);
}

TEST(MatchIntegrationTest, MultipleGoalsAccumulate) {
  MatchClock clock;
  clock.startMatch();

  clock.addGoal(0);
  clock.addGoal(0);
  clock.addGoal(1);

  EXPECT_EQ(clock.goals[0], 2);
  EXPECT_EQ(clock.goals[1], 1);
}

// ---------------------------------------------------------------------------
// Clock does not advance when not in a live half
// ---------------------------------------------------------------------------

TEST(MatchIntegrationTest, TickDoesNothingInPreMatch) {
  MatchClock clock;
  clock.tick(10000);
  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_PreMatch);
  EXPECT_EQ(clock.halfTime_ms, 0UL);
}

TEST(MatchIntegrationTest, TickDoesNothingAtHalfTime) {
  MatchClock clock;
  clock.startMatch();
  advanceClock(clock, kHalfDuration_ms);
  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_HalfTime);

  unsigned long savedTime = clock.halfTime_ms;
  clock.tick(10000);
  EXPECT_EQ(clock.halfTime_ms, savedTime);
}

// ---------------------------------------------------------------------------
// Single-tick simulation covering the full match
// ---------------------------------------------------------------------------

TEST(MatchIntegrationTest, FullMatchSimulationSingleTick) {
  MatchClock clock;
  clock.startMatch();

  // Single tick covering both halves: transitions to HalfTime (1st overflow)
  bool ended = clock.tick(2UL * kHalfDuration_ms);
  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_HalfTime);
  EXPECT_FALSE(ended);

  clock.startSecondHalf();
  ended = clock.tick(kHalfDuration_ms);
  EXPECT_TRUE(ended);
  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_FullTime);
}

}  // namespace
