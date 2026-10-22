// League Soccer — Gameplay Audit Test Suite
// Covers: AI tactics, first-touch, fatigue workload, goal-mouth detection,
// passing/dribble AI heuristics, zone pressure, offside trapping, and
// counter-attack strategy parameters.

#include <gtest/gtest.h>

#include "onthepitch/aitactics.hpp"
#include "onthepitch/gameplaytuning.hpp"
#include "onthepitch/matchclock.hpp"
#include "onthepitch/pendingcautions.hpp"
#include "onthepitch/setpiecerules.hpp"
#include "onthepitch/shotaim.hpp"
#include "onthepitch/sprinttap.hpp"

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
  EXPECT_GT(penalty, 0.15f);  // must be meaningfully large
  EXPECT_LE(penalty, 0.25f);  // capped
}

TEST(FirstTouchTest, PoorConditionAddsExtraPenalty) {
  float good =
      GameplayTuning::GetFirstTouchContextPenalty(2.0f, 0.5f, 0.5f, 8.0f, 0.0f, /*condition=*/5);
  float poor =
      GameplayTuning::GetFirstTouchContextPenalty(2.0f, 0.5f, 0.5f, 8.0f, 0.0f, /*condition=*/1);
  EXPECT_GT(poor, good);
}

TEST(FirstTouchTest, PenaltyIsAlwaysNonNegative) {
  // Even best-case condition should never produce a negative penalty
  float penalty =
      GameplayTuning::GetFirstTouchContextPenalty(10.0f, 1.0f, 1.0f, 1.0f, 1.0f, /*condition=*/5);
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
  EXPECT_FALSE(GameplayTuning::IsGoalMouthThreat(4.0f, 0.5f, 3.66f, 2.44f, /*anticipation=*/1.0f));
  EXPECT_TRUE(GameplayTuning::IsGoalMouthThreat(4.0f, 0.5f, 3.66f, 2.44f, /*anticipation=*/1.15f));
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

TEST(TerritoryTest, BallAtOwnGoalReturnsPlusOneForTeamSideOne) {
  // Ball at the defending team's goal from our attacking frame = +1
  float territory = AITactics::GetAttackingTerritory(
      /*ballX=*/-50.0f, /*teamSide=*/1, /*pitchHalfLength=*/50.0f);
  EXPECT_NEAR(territory, 1.0f, 0.01f);
}

TEST(TerritoryTest, BallAtOpponentGoalReturnsNegativeOneForTeamSideOne) {
  float territory = AITactics::GetAttackingTerritory(50.0f, 1, 50.0f);
  EXPECT_NEAR(territory, -1.0f, 0.01f);
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
  EXPECT_FALSE(AITactics::ShouldConsiderSupportPass(0.6f, 0.5f, 0.6f, 0.5f, 0.3f));
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

TEST(SprintTapTest, FirstPressAtKickoffDoesNotKnockOn) {
  SprintTap tap;
  tap.Update(0, true, true);
  EXPECT_FALSE(tap.Consume());
  tap.Update(100, true, true);
  EXPECT_TRUE(tap.Consume());
  EXPECT_FALSE(tap.Consume());
}

TEST(SprintTapTest, SlowSecondPressDoesNotKnockOn) {
  SprintTap tap;
  tap.Update(100, true, true);
  tap.Update(380, true, true);
  EXPECT_FALSE(tap.Consume());
}

TEST(SprintTapTest, UnconsumedKnockOnExpires) {
  SprintTap tap;
  tap.Update(100, true, true);
  tap.Update(200, true, true);
  tap.Update(480, false, true);
  EXPECT_FALSE(tap.Consume());
}

TEST(SprintTapTest, SuperCancelOrStoppageDiscardsPendingTouch) {
  SprintTap tap;
  tap.Update(100, true, true);
  tap.Update(200, true, true);
  tap.Update(210, false, false);
  EXPECT_FALSE(tap.Consume());
  tap.Update(220, true, true);
  EXPECT_FALSE(tap.Consume());
}

TEST(SprintTapTest, PlayerSwitchResetsDoubleTapHistory) {
  SprintTap tap;
  tap.Update(100, true, true);
  tap.Reset();
  tap.Update(200, true, true);
  EXPECT_FALSE(tap.Consume());
}

TEST(SprintTapTest, HeldButtonDoesNotCreateDoubleTap) {
  SprintTap tap;
  tap.Update(100, true, true);
  tap.Update(110, false, true);
  tap.Update(120, false, true);
  EXPECT_FALSE(tap.Consume());
}

TEST(AdvantageTest, EarlyMatchFoulKeepsItsGracePeriod) {
  using namespace GameplayTuning;
  EXPECT_EQ(EvaluateAdvantage(100, 100, true), AdvantageDecision::Continue);
  EXPECT_EQ(EvaluateAdvantage(700, 100, true), AdvantageDecision::Continue);
  EXPECT_EQ(EvaluateAdvantage(701, 100, true), AdvantageDecision::RecallFoul);
}

TEST(AdvantageTest, PossessionLossWithinWindowRecallsFoul) {
  using namespace GameplayTuning;
  EXPECT_EQ(EvaluateAdvantage(1500, 100, false), AdvantageDecision::Continue);
  EXPECT_EQ(EvaluateAdvantage(1500, 100, true), AdvantageDecision::RecallFoul);
  EXPECT_EQ(EvaluateAdvantage(3600, 100, true), AdvantageDecision::RecallFoul);
  EXPECT_EQ(EvaluateAdvantage(3601, 100, true), AdvantageDecision::PlayedOut);
}

TEST(AdvantageTest, WindowIsRelativeToFoulTime) {
  using namespace GameplayTuning;
  EXPECT_EQ(EvaluateAdvantage(500100, 500000, true), AdvantageDecision::Continue);
  EXPECT_EQ(EvaluateAdvantage(500601, 500000, true), AdvantageDecision::RecallFoul);
  EXPECT_EQ(EvaluateAdvantage(503501, 500000, false), AdvantageDecision::PlayedOut);
}

TEST(PendingCautionsTest, AdvantageKeepsCautionUntilPlayStops) {
  PendingCautions<int> cautions;
  int player = 0;
  cautions.Add(&player);
  EXPECT_TRUE(cautions.TakeAtStoppage(true).empty());
  EXPECT_EQ(cautions.CountFor(&player), 1);
  const auto issued = cautions.TakeAtStoppage(false);
  ASSERT_EQ(issued.size(), 1u);
  EXPECT_EQ(issued[0], &player);
  EXPECT_TRUE(cautions.TakeAtStoppage(false).empty());
  EXPECT_EQ(cautions.CountFor(&player), 0);
}

TEST(PendingCautionsTest, LaterOffenceDoesNotEraseEarlierOffender) {
  PendingCautions<int> cautions;
  int first = 0, second = 0;
  cautions.Add(&first);
  cautions.Add(&second);
  const auto issued = cautions.TakeAtStoppage(false);
  ASSERT_EQ(issued.size(), 2u);
  EXPECT_EQ(issued[0], &first);
  EXPECT_EQ(issued[1], &second);
}

TEST(PendingCautionsTest, DistinctOffencesBySamePlayerAreRetained) {
  PendingCautions<int> cautions;
  int player = 0;
  cautions.Add(&player);
  cautions.Add(&player);
  EXPECT_EQ(cautions.CountFor(&player), 2);
  EXPECT_EQ(cautions.TakeAtStoppage(false).size(), 2u);
}

TEST(PendingCautionsTest, MissingOffenderIsIgnored) {
  PendingCautions<int> cautions;
  cautions.Add(nullptr);
  EXPECT_TRUE(cautions.TakeAtStoppage(false).empty());
}

TEST(SetPieceRulesTest, DirectGoalKickCornerAndThrowInAreOffsideExempt) {
  EXPECT_TRUE(IsOffsideExemptRestart(e_SetPiece_GoalKick));
  EXPECT_TRUE(IsOffsideExemptRestart(e_SetPiece_Corner));
  EXPECT_TRUE(IsOffsideExemptRestart(e_SetPiece_ThrowIn));
}

TEST(SetPieceRulesTest, OpenPlayAndOtherRestartsStillCheckOffside) {
  EXPECT_FALSE(IsOffsideExemptRestart(e_SetPiece_None));
  EXPECT_FALSE(IsOffsideExemptRestart(e_SetPiece_FreeKick));
  EXPECT_FALSE(IsOffsideExemptRestart(e_SetPiece_KickOff));
  EXPECT_FALSE(IsOffsideExemptRestart(e_SetPiece_Penalty));
}
TEST(ShotAimTest, SmallLateCorrectionReachesRequestedDirection) {
  const blunted::Vector3 committed(1, 0, 0);
  const auto requested = committed.GetRotated2D(0.1f);
  const auto result = RefineShotDirection(committed, requested, 0.3f);
  EXPECT_NEAR(result.GetDistance(requested), 0.0f, 0.0001f);
}

TEST(ShotAimTest, LargeCorrectionsStayWithinCommitmentInBothDirections) {
  const blunted::Vector3 committed(1, 0, 0);
  for (float direction : {-1.0f, 1.0f}) {
    const auto requested = committed.GetRotated2D(direction * 1.0f);
    const auto result = RefineShotDirection(committed, requested, 0.3f);
    EXPECT_NEAR(result.GetAngle2D(committed), direction * 0.3f, 0.0001f);
    EXPECT_NEAR(result.GetLength(), 1.0f, 0.0001f);
  }
}

TEST(ShotAimTest, CorrectionAcrossAngleWrapUsesShortPath) {
  const auto committed = blunted::Vector3(1, 0, 0).GetRotated2D(3.1f);
  const auto requested = blunted::Vector3(1, 0, 0).GetRotated2D(-3.1f);
  const auto result = RefineShotDirection(committed, requested, 0.3f);
  EXPECT_NEAR(result.GetDistance(requested), 0.0f, 0.0001f);
}

TEST(ShotAimTest, ZeroAllowanceKeepsCommittedDirection) {
  const blunted::Vector3 committed(1, 0, 0);
  const blunted::Vector3 requested(0, 1, 0);
  EXPECT_NEAR(RefineShotDirection(committed, requested, 0).GetDistance(committed), 0, 0.0001f);
}

TEST(ShotStyleTest, IndividualModifiersKeepTheirStyle) {
  EXPECT_EQ(ResolveShotStyle(false, false), ShotStyle::Normal);
  EXPECT_EQ(ResolveShotStyle(true, false), ShotStyle::Chip);
  EXPECT_EQ(ResolveShotStyle(false, true), ShotStyle::Finesse);
}

TEST(ShotStyleTest, CombinedModifiersUseChipWithoutFinesseBonuses) {
  EXPECT_EQ(ResolveShotStyle(true, true), ShotStyle::Chip);
}
TEST(TeammateRunsTest, SpaceProbeLooksTowardsOpponentGoalForEitherSide) {
  EXPECT_FLOAT_EQ(AITactics::GetAttackingProbeX(20, 1, 10), 10);
  EXPECT_FLOAT_EQ(AITactics::GetAttackingProbeX(-20, -1, 10), -10);
  EXPECT_FLOAT_EQ(AITactics::GetAttackingProbeX(0, 1, 26), -26);
  EXPECT_FLOAT_EQ(AITactics::GetAttackingProbeX(0, -1, 26), 26);
}

TEST(TeammateRunsTest, EligibleOutfieldTeammateCanRun) {
  EXPECT_TRUE(AITactics::IsAutomaticRunnerEligible(true, false, false, false));
}

TEST(TeammateRunsTest, KeeperCarrierHumanAndInactivePlayersAreExcluded) {
  EXPECT_FALSE(AITactics::IsAutomaticRunnerEligible(true, false, true, false));
  EXPECT_FALSE(AITactics::IsAutomaticRunnerEligible(true, false, false, true));
  EXPECT_FALSE(AITactics::IsAutomaticRunnerEligible(true, true, false, false));
  EXPECT_FALSE(AITactics::IsAutomaticRunnerEligible(false, false, false, false));
}

TEST(TeammatePressureTest, SmallDistanceChangesDoNotSwapThePresser) {
  EXPECT_TRUE(AITactics::ShouldKeepPressurePlayer(7.0f, 6.9f));
  EXPECT_TRUE(AITactics::ShouldKeepPressurePlayer(7.0f, 5.5f));
  EXPECT_TRUE(AITactics::ShouldKeepPressurePlayer(7.0f, 7.0f));
}

TEST(TeammatePressureTest, ClearlyBetterDefenderTakesOver) {
  EXPECT_FALSE(AITactics::ShouldKeepPressurePlayer(7.0f, 5.4f));
  EXPECT_FALSE(AITactics::ShouldKeepPressurePlayer(20.0f, 5.0f));
}
}  // namespace

namespace {

// ============================================================
// Match clock — the minute shown for goals and in the pause menu
// ============================================================

TEST(MatchClockTest, MinuteCountsFromOne) {
  EXPECT_EQ(MatchMinuteText(0, 45), "1'");
  EXPECT_EQ(MatchMinuteText(22 * 60000UL + 5000, 45), "23'");
  EXPECT_EQ(MatchMinuteText(44 * 60000UL, 45), "45'");
}

TEST(MatchClockTest, StoppageTimeIsShownAfterThePeriod) {
  EXPECT_EQ(MatchMinuteText(45 * 60000UL, 45), "45+1'");
  EXPECT_EQ(MatchMinuteText(92 * 60000UL, 90), "90+3'");
  EXPECT_EQ(MatchMinuteText(100 * 60000UL, 105), "101'");
}

}  // namespace

#include "onthepitch/kitclash.hpp"

namespace {

// ============================================================
// Kit clash — the away side changes kit when the shirts look alike
// ============================================================

TEST(KitClashTest, KeepsTheAwayKitWhenItStandsOut) {
  const kitclash::Colour home{16, 78, 195}, away1{13, 58, 155}, away2{204, 212, 202};
  EXPECT_EQ(kitclash::ChooseAwayKit(home, &away1, &away2, 2), 2);
}

TEST(KitClashTest, SwitchesWhenTheAwayKitClashes) {
  // Two dark blues: the white first kit is the one to wear.
  const kitclash::Colour home{16, 78, 195}, away1{204, 212, 202}, away2{20, 70, 180};
  EXPECT_EQ(kitclash::ChooseAwayKit(home, &away1, &away2, 2), 1);
}

TEST(KitClashTest, StaysWhenTheOtherKitIsNoBetter) {
  const kitclash::Colour home{35, 54, 44}, away1{45, 25, 45}, away2{60, 55, 51};
  EXPECT_EQ(kitclash::ChooseAwayKit(home, &away1, &away2, 2), 2);
}

TEST(KitClashTest, NeverSwitchesToAMissingKit) {
  const kitclash::Colour home{16, 78, 195}, away2{20, 70, 180};
  EXPECT_EQ(kitclash::ChooseAwayKit(home, nullptr, &away2, 2), 2);
}

}  // namespace

namespace {

// ============================================================
// Stoppage time — the minutes added at the end of each period
// ============================================================

TEST(StoppageTimeTest, QuietFirstHalfGetsAMinute) {
  EXPECT_EQ(StoppageMinutes(0, 0, 45), 1);
}

TEST(StoppageTimeTest, GoalsAndCardsAddTime) {
  EXPECT_EQ(StoppageMinutes(2, 3, 45), 5);
  EXPECT_EQ(StoppageMinutes(1, 0, 90), 3);
}

TEST(StoppageTimeTest, IsCappedPerPeriod) {
  EXPECT_EQ(StoppageMinutes(6, 6, 90), 5);
  EXPECT_EQ(StoppageMinutes(3, 2, 105), 2);
  EXPECT_EQ(StoppageMinutes(0, 0, 120), 1);
}

}  // namespace
