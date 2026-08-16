// League Soccer — Gameplay Audit Test Suite
// Covers: AI tactics, first-touch, fatigue workload, goal-mouth detection,
// passing/dribble AI heuristics, zone pressure, offside trapping, and
// counter-attack strategy parameters.

#include <gtest/gtest.h>

#include "onthepitch/aitactics.hpp"
#include "onthepitch/gameplaytuning.hpp"

namespace {

// ============================================================
// GameplayTuning — First Touch Context Penalty
// ============================================================

TEST(FirstTouchTest, NoPenaltyInIdealConditions) {
  // Far opponent, calm player, slow ball straight on = near-zero penalty
  float penalty = GameplayTuning::GetFirstTouchContextPenalty(
      /*opponentDistance=*/5.0f, /*calmness=*/1.0f, /*balance=*/1.0f,
      /*ballSpeed=*/2.0f, /*facingAlignment=*/0.9f, /*condition=*/3);
  EXPECT_NEAR(penalty, 0.0f, 0.005f);
}

TEST(FirstTouchTest, MaxPenaltyUnderHeavyPressure) {
  // Very close opponent, low composure, hard fast ball from blind side
  float penalty = GameplayTuning::GetFirstTouchContextPenalty(
      /*opponentDistance=*/0.1f, /*calmness=*/0.0f, /*balance=*/0.0f,
      /*ballSpeed=*/16.0f, /*facingAlignment=*/-1.0f, /*condition=*/1);
  EXPECT_GT(penalty, 0.15f);   // must be meaningfully large
  EXPECT_LE(penalty, 0.25f);   // capped
}

TEST(FirstTouchTest, PoorConditionAddsExtraPenalty) {
  float good = GameplayTuning::GetFirstTouchContextPenalty(
      2.0f, 0.5f, 0.5f, 8.0f, 0.0f, /*condition=*/5);
  float poor = GameplayTuning::GetFirstTouchContextPenalty(
      2.0f, 0.5f, 0.5f, 8.0f, 0.0f, /*condition=*/1);
  EXPECT_GT(poor, good);
}

TEST(FirstTouchTest, PenaltyIsAlwaysNonNegative) {
  // Even best-case condition should never produce a negative penalty
  float penalty = GameplayTuning::GetFirstTouchContextPenalty(
      10.0f, 1.0f, 1.0f, 1.0f, 1.0f, /*condition=*/5);
  EXPECT_GE(penalty, 0.0f);
}

// ============================================================
// GameplayTuning — Fatigue Workload
// ============================================================

TEST(FatigueTest, JoggingCheaperThanSprinting) {
  const float maxSpeed = 8.0f;
  float jog = GameplayTuning::GetFatigueWorkloadFactor(4.0f, maxSpeed, false);
  float sprint = GameplayTuning::GetFatigueWorkloadFactor(8.0f, maxSpeed, false);
  EXPECT_LT(jog, sprint);
}

TEST(FatigueTest, CarryingBallIncreasesSprintLoad) {
  const float maxSpeed = 8.0f;
  float noBall = GameplayTuning::GetFatigueWorkloadFactor(8.0f, maxSpeed, false);
  float withBall = GameplayTuning::GetFatigueWorkloadFactor(8.0f, maxSpeed, true);
  EXPECT_GT(withBall, noBall);
}

TEST(FatigueTest, IdleSpeedNearBaseWorkload) {
  // Standing still should be close to 0.90 (base workload at zero sprint load)
  float idle = GameplayTuning::GetFatigueWorkloadFactor(0.0f, 8.0f, false);
  EXPECT_NEAR(idle, 0.90f, 0.05f);
}

TEST(FatigueTest, ZeroMaxSpeedReturnsOne) {
  // Guard against division by zero
  float result = GameplayTuning::GetFatigueWorkloadFactor(5.0f, 0.0f, false);
  EXPECT_FLOAT_EQ(result, 1.0f);
}

// ============================================================
// GameplayTuning — Goal Mouth Threat
// ============================================================

TEST(GoalMouthTest, BallCentredInGoalIsThreat) {
  EXPECT_TRUE(GameplayTuning::IsGoalMouthThreat(
      /*lateral=*/0.0f, /*ballHeight=*/0.5f,
      /*goalHalfWidth=*/3.66f, /*goalHeight=*/2.44f, /*anticipation=*/1.0f));
}

TEST(GoalMouthTest, BallWideOfGoalIsNotThreat) {
  EXPECT_FALSE(GameplayTuning::IsGoalMouthThreat(
      /*lateral=*/5.0f, /*ballHeight=*/0.5f,
      /*goalHalfWidth=*/3.66f, /*goalHeight=*/2.44f, /*anticipation=*/1.0f));
}

TEST(GoalMouthTest, BallOverCrossbarIsNotThreat) {
  EXPECT_FALSE(GameplayTuning::IsGoalMouthThreat(
      /*lateral=*/0.0f, /*ballHeight=*/3.0f,
      /*goalHalfWidth=*/3.66f, /*goalHeight=*/2.44f, /*anticipation=*/1.0f));
}

TEST(GoalMouthTest, AnticipationWidensWindow) {
  // Ball 4m wide; not a threat at 1.0 anticipation but is at 1.2
  EXPECT_FALSE(GameplayTuning::IsGoalMouthThreat(
      4.0f, 0.5f, 3.66f, 2.44f, /*anticipation=*/1.0f));
  EXPECT_TRUE(GameplayTuning::IsGoalMouthThreat(
      4.0f, 0.5f, 3.66f, 2.44f, /*anticipation=*/1.15f));
}

// ============================================================
// AITactics — Attacking Run Threshold
// ============================================================

TEST(AttackingRunTest, NeutralThresholdIsInRange) {
  // At counter_attack=0.5, threshold should be near the mid-range value (0.48)
  float threshold = AITactics::GetAttackingRunThreshold(0.5f);
  EXPECT_GE(threshold, 0.35f);
  EXPECT_LE(threshold, 0.62f);
}

TEST(AttackingRunTest, HighCounterAttackLowersThreshold) {
  // Aggressive counter: players run earlier (lower threshold)
  float aggressive = AITactics::GetAttackingRunThreshold(1.0f);
  float conservative = AITactics::GetAttackingRunThreshold(0.0f);
  EXPECT_LT(aggressive, conservative);
}

TEST(AttackingRunTest, DurationIncreasesWithCounterAttack) {
  unsigned int slowDuration = AITactics::GetAttackingRunDuration_ms(0.0f);
  unsigned int fastDuration = AITactics::GetAttackingRunDuration_ms(1.0f);
  EXPECT_GT(fastDuration, slowDuration);
  // Must be positive
  EXPECT_GT(slowDuration, 0u);
}

// ============================================================
// AITactics — Attacking Territory
// ============================================================

TEST(TerritoryTest, BallAtOwnGoalReturnsNegativeOne) {
  // Ball at the defending team's goal = -1
  float territory = AITactics::GetAttackingTerritory(
      /*ballX=*/-50.0f, /*teamSide=*/1, /*pitchHalfLength=*/50.0f);
  EXPECT_NEAR(territory, -1.0f, 0.01f);
}

TEST(TerritoryTest, BallAtOpponentGoalReturnsPlusOne) {
  float territory = AITactics::GetAttackingTerritory(50.0f, 1, 50.0f);
  EXPECT_NEAR(territory, 1.0f, 0.01f);
}

TEST(TerritoryTest, BallAtCentreIsZero) {
  float territory = AITactics::GetAttackingTerritory(0.0f, 1, 50.0f);
  EXPECT_NEAR(territory, 0.0f, 0.01f);
}

TEST(TerritoryTest, TeamSideFlipsMirrorsTerritorySign) {
  // Flipping teamSide (−1 vs +1) should mirror the territory
  float team0 = AITactics::GetAttackingTerritory(20.0f, 1, 50.0f);
  float team1 = AITactics::GetAttackingTerritory(20.0f, -1, 50.0f);
  EXPECT_NEAR(team0, -team1, 0.01f);
}

// ============================================================
// AITactics — Zone Pressure Decision
// ============================================================

TEST(ZonePressureTest, NoPressureSettingNeverTriggers) {
  // pressure=0 means completely passive
  EXPECT_FALSE(AITactics::ShouldStartZonePressure(0.0f, 0.9f, 3.0f));
  EXPECT_FALSE(AITactics::ShouldStartZonePressure(0.0f, 0.5f, 1.0f));
}

TEST(ZonePressureTest, MaxPressureTriggersEvenDeep) {
  // High pressure with close primary defender and opponent in our half
  EXPECT_TRUE(AITactics::ShouldStartZonePressure(1.0f, -0.3f, 8.0f));
}

TEST(ZonePressureTest, PressureDoesNotTriggerWhenDefenderFarAway) {
  // Even at max pressure, don't trigger if our nearest defender is 25m away
  EXPECT_FALSE(AITactics::ShouldStartZonePressure(1.0f, 0.9f, 25.0f));
}

TEST(ZonePressureTest, DurationScalesWithPressureSetting) {
  unsigned int low = AITactics::GetZonePressureDuration_ms(0.0f);
  unsigned int high = AITactics::GetZonePressureDuration_ms(1.0f);
  EXPECT_GT(high, low);
}

// ============================================================
// AITactics — Support Web & Dribble Drive
// ============================================================

TEST(SupportWebTest, HighSettingExpandsWebScale) {
  float compact = AITactics::GetSupportWebScale(0.0f);
  float spread = AITactics::GetSupportWebScale(1.0f);
  EXPECT_GT(spread, compact);
}

TEST(SupportWebTest, NeutralWebScaleInBounds) {
  float neutral = AITactics::GetSupportWebScale(0.5f);
  EXPECT_GE(neutral, 0.65f);
  EXPECT_LE(neutral, 0.85f);
}

TEST(DribbleTest, HighOffensivenessIncreasesForwardDrive) {
  float passive = AITactics::GetDribbleForwardDrive(0.0f, 0.0f);
  float aggressive = AITactics::GetDribbleForwardDrive(1.0f, 1.0f);
  EXPECT_GT(aggressive, passive);
}

// ============================================================
// AITactics — Defender Support Scale
// ============================================================

TEST(DefenderSupportTest, CentreBacksHaveLowerScaleThanFullBacks) {
  // roleMindset near 0 = deep defender (CB), near 1 = attacker
  float cb = AITactics::GetDefenderSupportScale(0.0f);
  float fb = AITactics::GetDefenderSupportScale(0.5f);
  EXPECT_LT(cb, fb);
}

TEST(DefenderSupportTest, ScaleIsAlwaysPositive) {
  EXPECT_GT(AITactics::GetDefenderSupportScale(0.0f), 0.0f);
  EXPECT_GT(AITactics::GetDefenderSupportScale(1.0f), 0.0f);
}

// ============================================================
// AITactics — Support Pass Decision
// ============================================================

TEST(SupportPassTest, ShouldPassWhenTeammateHasMoreSpace) {
  // Current player is under pressure, teammate has good space
  EXPECT_TRUE(AITactics::ShouldConsiderSupportPass(
      /*curTactical=*/0.6f, /*curSpace=*/0.1f,
      /*mateTactical=*/0.55f, /*mateSpace=*/0.5f,
      /*longPossession=*/0.8f));
}

TEST(SupportPassTest, ShouldNotPassWhenBothHaveEqualSpace) {
  EXPECT_FALSE(AITactics::ShouldConsiderSupportPass(
      0.6f, 0.5f, 0.6f, 0.5f, 0.3f));
}

TEST(SupportPassTest, SupportPassBonusScalesWithSpaceGain) {
  float small = AITactics::GetSupportPassBonus(0.5f, 0.6f, 0.4f);
  float large = AITactics::GetSupportPassBonus(0.1f, 0.9f, 0.8f);
  EXPECT_GT(large, small);
}

TEST(SupportPassTest, SupportPassBonusAlwaysNonNegative) {
  EXPECT_GE(AITactics::GetSupportPassBonus(0.5f, 0.5f, 0.5f), 0.0f);
  EXPECT_GE(AITactics::GetSupportPassBonus(0.0f, 0.0f, 0.0f), 0.0f);
}

// ============================================================
// AITactics — Secondary Pressure Role Penalty
// ============================================================

TEST(PressureRolePenaltyTest, DefenderHasHighPenalty) {
  // Deep defender (mindset≈0) should not be the second presser
  float defenderPenalty = AITactics::GetSecondaryPressureRolePenalty(0.0f);
  float attackerPenalty = AITactics::GetSecondaryPressureRolePenalty(1.0f);
  EXPECT_GT(defenderPenalty, attackerPenalty);
}

TEST(PressureRolePenaltyTest, PenaltyIsNonNegative) {
  EXPECT_GE(AITactics::GetSecondaryPressureRolePenalty(0.0f), 0.0f);
  EXPECT_GE(AITactics::GetSecondaryPressureRolePenalty(1.0f), 0.0f);
}

// ============================================================
// Clamp utility (GameplayTuning)
// ============================================================

TEST(ClampUtilTest, Clamp01AlwaysInRange) {
  EXPECT_FLOAT_EQ(GameplayTuning::Clamp01(-5.0f), 0.0f);
  EXPECT_FLOAT_EQ(GameplayTuning::Clamp01(5.0f), 1.0f);
  EXPECT_FLOAT_EQ(GameplayTuning::Clamp01(0.5f), 0.5f);
}

}  // namespace
