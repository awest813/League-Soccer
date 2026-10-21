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
  EXPECT_TRUE(AITactics::ShouldStartZonePressure(0.5f, 0.2f, 8.0f));
  EXPECT_TRUE(AITactics::ShouldStartZonePressure(1.0f, -0.3f, 12.0f));
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
  EXPECT_NEAR(AITactics::GetDribbleForwardDrive(0.5f, 0.5f), 0.75f, 0.001f);
}

TEST(AITacticsTest, SupportDistancePreservesNeutralSpacingAndOffersSubtleRange) {
  EXPECT_FLOAT_EQ(AITactics::GetSupportWebScale(0.5f), 0.78f);
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

// ---------------------------------------------------------------------------
// Stoppage / injury time simulation
// ---------------------------------------------------------------------------

TEST(MatchIntegrationTest, StoppageTimeExtendsActiveHalf) {
  MatchClock clock;
  clock.startMatch();

  // Set 3 minutes of injury time (180,000 ms)
  clock.setStoppageTime(3UL * 60UL * 1000UL);

  // At 45:00 exact, the half should NOT have ended yet because of stoppage time
  advanceClock(clock, kHalfDuration_ms);
  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_1stHalf);
  EXPECT_EQ(clock.halfTime_ms, kHalfDuration_ms);

  // Goal scored in 1st minute of added time
  clock.addGoal(0);
  EXPECT_EQ(clock.goals[0], 1);

  // Play remaining 3 minutes of stoppage time
  advanceClock(clock, 3UL * 60UL * 1000UL);
  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_HalfTime);
  EXPECT_EQ(clock.stoppageTime_ms, 0UL);  // Resets after period transition
}

// ---------------------------------------------------------------------------
// Extra time and penalty shootout simulation
// ---------------------------------------------------------------------------

TEST(MatchIntegrationTest, ExtraTimeTransitionsAndTotalDuration) {
  MatchClock clock;
  clock.allowExtraTime = true;
  clock.startMatch();

  // 1-1 at 90 minutes
  clock.addGoal(0);
  advanceClock(clock, kHalfDuration_ms);
  clock.startSecondHalf();
  clock.addGoal(1);
  advanceClock(clock, kHalfDuration_ms);

  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_FullTime);
  EXPECT_TRUE(clock.isTied());
  EXPECT_EQ(clock.totalElapsed_ms(), 2UL * kHalfDuration_ms);

  // Kick off extra time
  clock.startExtraTime();
  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_1stExtraTime);

  // 1st extra time (15 mins)
  advanceClock(clock, kExtraTimeHalfDuration_ms);
  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_ExtraTimeBreak);
  EXPECT_EQ(clock.totalElapsed_ms(), 2UL * kHalfDuration_ms + kExtraTimeHalfDuration_ms);

  // 2nd extra time (15 mins)
  clock.startSecondExtraTime();
  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_2ndExtraTime);

  // Goal in 2nd extra time for team 0
  clock.addGoal(0);
  advanceClock(clock, kExtraTimeHalfDuration_ms);

  // Not tied, so concludes at Final
  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_Final);
  EXPECT_FALSE(clock.isTied());
  EXPECT_EQ(clock.goals[0], 2);
  EXPECT_EQ(clock.goals[1], 1);
  EXPECT_EQ(clock.totalElapsed_ms(), 2UL * kHalfDuration_ms + 2UL * kExtraTimeHalfDuration_ms);
}

TEST(MatchIntegrationTest, ExtraTimeTieTransitionsToPenaltyShootout) {
  MatchClock clock;
  clock.allowExtraTime = true;
  clock.allowPenalties = true;
  clock.startMatch();

  // 0-0 through 90 minutes
  advanceClock(clock, kHalfDuration_ms);
  clock.startSecondHalf();
  advanceClock(clock, kHalfDuration_ms);
  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_FullTime);

  // Extra time 1st & 2nd periods remain 0-0
  clock.startExtraTime();
  advanceClock(clock, kExtraTimeHalfDuration_ms);
  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_ExtraTimeBreak);

  clock.startSecondExtraTime();
  advanceClock(clock, kExtraTimeHalfDuration_ms);

  // Since still tied and allowPenalties is true, transitions to Penalties
  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_Penalties);
  EXPECT_TRUE(clock.isTied());

  // Simulate shootout: 5-4
  for (int i = 0; i < 5; ++i)
    clock.addPenaltyGoal(0);
  for (int i = 0; i < 4; ++i)
    clock.addPenaltyGoal(1);

  EXPECT_EQ(clock.penaltyGoals[0], 5);
  EXPECT_EQ(clock.penaltyGoals[1], 4);
  EXPECT_FALSE(clock.isPenaltyTied());

  clock.concludePenalties();
  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_Final);
}

TEST(MatchIntegrationTest, ClockDoesNotAdvanceDuringExtraTimeBreakOrPenalties) {
  MatchClock clock;
  clock.allowExtraTime = true;
  clock.allowPenalties = true;
  clock.startMatch();

  advanceClock(clock, kHalfDuration_ms);
  clock.startSecondHalf();
  advanceClock(clock, kHalfDuration_ms);
  clock.startExtraTime();
  advanceClock(clock, kExtraTimeHalfDuration_ms);

  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_ExtraTimeBreak);
  EXPECT_FALSE(clock.tick(5000));
  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_ExtraTimeBreak);

  clock.startSecondExtraTime();
  advanceClock(clock, kExtraTimeHalfDuration_ms);
  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_Penalties);

  EXPECT_FALSE(clock.tick(5000));
  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_Penalties);
}

TEST(MatchIntegrationTest, NoElapsedTimeBeforeKickoffAndKickoffCannotResetPlay) {
  MatchClock clock;
  EXPECT_EQ(clock.totalElapsed_ms(), 0UL);
  clock.startMatch();
  clock.tick(12000);
  clock.startMatch();
  EXPECT_EQ(clock.totalElapsed_ms(), 12000UL);
}

TEST(MatchIntegrationTest, StoppageTimeAccumulatesAcrossAllPeriods) {
  MatchClock clock;
  clock.allowExtraTime = true;
  clock.startMatch();
  clock.setStoppageTime(180000);
  clock.tick(kHalfDuration_ms + 180000);
  clock.startSecondHalf();
  clock.setStoppageTime(120000);
  EXPECT_FALSE(clock.tick(kHalfDuration_ms + 120000));
  EXPECT_EQ(clock.totalElapsed_ms(), 2UL * kHalfDuration_ms + 300000);
  clock.startExtraTime();
  clock.setStoppageTime(60000);
  clock.tick(kExtraTimeHalfDuration_ms + 60000);
  clock.startSecondExtraTime();
  clock.setStoppageTime(30000);
  EXPECT_TRUE(clock.tick(kExtraTimeHalfDuration_ms + 30000));
  EXPECT_EQ(clock.totalElapsed_ms(),
            2UL * kHalfDuration_ms + 2UL * kExtraTimeHalfDuration_ms + 390000);
}

TEST(MatchIntegrationTest, KnockoutStagesCannotInterruptPlayOrSkipExtraTime) {
  MatchClock clock;
  clock.allowExtraTime = true;
  clock.allowPenalties = true;
  clock.startMatch();
  clock.tick(kHalfDuration_ms);
  clock.startSecondHalf();
  clock.startExtraTime();
  clock.startPenalties();
  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_2ndHalf);
  EXPECT_FALSE(clock.tick(kHalfDuration_ms));
  clock.startPenalties();
  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_FullTime);
  clock.startExtraTime();
  clock.tick(kExtraTimeHalfDuration_ms);
  clock.startSecondExtraTime();
  clock.startPenalties();
  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_2ndExtraTime);
  EXPECT_FALSE(clock.tick(kExtraTimeHalfDuration_ms));
  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_Penalties);
}

TEST(MatchIntegrationTest, RegulationWinnerDoesNotEnterKnockoutStages) {
  MatchClock clock;
  clock.allowExtraTime = true;
  clock.allowPenalties = true;
  clock.startMatch();
  clock.addGoal(0);
  clock.tick(kHalfDuration_ms);
  clock.startSecondHalf();
  EXPECT_TRUE(clock.tick(kHalfDuration_ms));
  clock.startExtraTime();
  clock.startPenalties();
  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_FullTime);
  clock.allowExtraTime = false;
  clock.startPenalties();
  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_FullTime);
}

TEST(MatchIntegrationTest, DirectShootoutKeepsRegulationTimeAndCannotReopenFinal) {
  MatchClock clock;
  clock.allowPenalties = true;
  clock.addPenaltyGoal(0);
  EXPECT_EQ(clock.penaltyGoals[0], 0);
  clock.startMatch();
  clock.tick(kHalfDuration_ms);
  clock.startSecondHalf();
  EXPECT_FALSE(clock.tick(kHalfDuration_ms));
  clock.startPenalties();
  EXPECT_EQ(clock.totalElapsed_ms(), 2UL * kHalfDuration_ms);
  clock.concludePenalties();
  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_Penalties);
  clock.addPenaltyGoal(0);
  clock.concludePenalties();
  clock.startPenalties();
  clock.startMatch();
  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_Final);
  EXPECT_EQ(clock.totalElapsed_ms(), 2UL * kHalfDuration_ms);
  clock.addPenaltyGoal(1);
  EXPECT_EQ(clock.penaltyGoals[1], 0);
}

TEST(MatchIntegrationTest, LargeClockValuesDoNotWrap) {
  MatchClock clock;
  const auto maximum = (std::numeric_limits<unsigned long>::max)();
  clock.startMatch();
  clock.tick(1000);
  clock.setStoppageTime(maximum);
  clock.tick(maximum);
  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_HalfTime);
  EXPECT_EQ(clock.totalElapsed_ms(), maximum);
  clock.startSecondHalf();
  clock.tick(kHalfDuration_ms);
  EXPECT_EQ(clock.totalElapsed_ms(), maximum);
}

TEST(MatchIntegrationTest, GoalsOnlyCountDuringLivePeriods) {
  MatchClock clock;
  clock.addGoal(0);
  EXPECT_EQ(clock.goals[0], 0);
  clock.startMatch();
  clock.addGoal(0);
  clock.addGoal(-1);
  clock.addGoal(2);
  clock.tick(kHalfDuration_ms);
  clock.addGoal(1);
  EXPECT_EQ(clock.goals[1], 0);
  clock.startSecondHalf();
  clock.addGoal(1);
  clock.tick(kHalfDuration_ms);
  clock.addGoal(0);
  EXPECT_EQ(clock.goals[0], 1);
  EXPECT_EQ(clock.goals[1], 1);
}

TEST(MatchIntegrationTest, ReducingStoppageTimeNeverRewindsElapsedPlay) {
  MatchClock clock;
  clock.startMatch();
  clock.setStoppageTime(180000);
  clock.tick(kHalfDuration_ms + 120000);
  clock.setStoppageTime(60000);
  clock.tick(1000);
  EXPECT_EQ(clock.phase, e_MatchPhaseSimple_HalfTime);
  EXPECT_EQ(clock.totalElapsed_ms(), kHalfDuration_ms + 120000);
  clock.startSecondHalf();
  EXPECT_EQ(clock.totalElapsed_ms(), kHalfDuration_ms + 120000);
}

}  // namespace
