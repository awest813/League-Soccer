#include <gtest/gtest.h>

#include "onthepitch/velocitystate.hpp"

namespace {

// ---------------------------------------------------------------------------
// RangeVelocity – maps a continuous float to the nearest quantised level
// ---------------------------------------------------------------------------

TEST(VelocityStateTest, RangeVelocity_IdleBeforeSwitch) {
  // Values below idleDribbleSwitch (1.8) → idle
  EXPECT_EQ(RangeVelocity(0.0f), idleVelocity);
  EXPECT_EQ(RangeVelocity(1.0f), idleVelocity);
  EXPECT_EQ(RangeVelocity(1.79f), idleVelocity);
}

TEST(VelocityStateTest, RangeVelocity_DribbleBand) {
  // [idleDribbleSwitch, dribbleWalkSwitch) → dribble
  EXPECT_EQ(RangeVelocity(idleDribbleSwitch), dribbleVelocity);  // 1.8 → dribble
  EXPECT_EQ(RangeVelocity(3.0f), dribbleVelocity);
  EXPECT_EQ(RangeVelocity(4.19f), dribbleVelocity);
}

TEST(VelocityStateTest, RangeVelocity_WalkBand) {
  // [dribbleWalkSwitch, walkSprintSwitch) → walk
  EXPECT_EQ(RangeVelocity(dribbleWalkSwitch), walkVelocity);  // 4.2 → walk
  EXPECT_EQ(RangeVelocity(5.0f), walkVelocity);
  EXPECT_EQ(RangeVelocity(5.99f), walkVelocity);
}

TEST(VelocityStateTest, RangeVelocity_SprintAboveSwitch) {
  // ≥ walkSprintSwitch (6.0) → sprint
  EXPECT_EQ(RangeVelocity(walkSprintSwitch), sprintVelocity);  // 6.0 → sprint
  EXPECT_EQ(RangeVelocity(7.0f), sprintVelocity);
  EXPECT_EQ(RangeVelocity(8.0f), sprintVelocity);
  EXPECT_EQ(RangeVelocity(100.0f), sprintVelocity);
}

// ---------------------------------------------------------------------------
// ClampVelocity – keeps velocity in [0, sprintVelocity]
// ---------------------------------------------------------------------------

TEST(VelocityStateTest, ClampVelocity_NegativeBecomesZero) {
  EXPECT_EQ(ClampVelocity(-1.0f), 0.0f);
  EXPECT_EQ(ClampVelocity(-100.0f), 0.0f);
}

TEST(VelocityStateTest, ClampVelocity_InRangeUnchanged) {
  EXPECT_EQ(ClampVelocity(0.0f), 0.0f);
  EXPECT_EQ(ClampVelocity(4.0f), 4.0f);
  EXPECT_EQ(ClampVelocity(sprintVelocity), sprintVelocity);
}

TEST(VelocityStateTest, ClampVelocity_AboveMaxClamped) {
  EXPECT_EQ(ClampVelocity(sprintVelocity + 1.0f), sprintVelocity);
  EXPECT_EQ(ClampVelocity(100.0f), sprintVelocity);
}

// ---------------------------------------------------------------------------
// FloorVelocity – rounds up to the next quantised level (never returns idle)
// ---------------------------------------------------------------------------

TEST(VelocityStateTest, FloorVelocity_ZeroRoundsUpToWalk) {
  // 0 is not in (0, dribbleVelocity) so it falls through to the
  // velocity <= walkVelocity branch and returns walkVelocity.
  EXPECT_EQ(FloorVelocity(0.0f), walkVelocity);
}

TEST(VelocityStateTest, FloorVelocity_SubDribbleRoundedUp) {
  // Any positive value below dribbleVelocity (3.5) → dribbleVelocity
  EXPECT_EQ(FloorVelocity(0.1f), dribbleVelocity);
  EXPECT_EQ(FloorVelocity(3.4f), dribbleVelocity);
}

TEST(VelocityStateTest, FloorVelocity_DribbleToWalkRange) {
  // Values in (dribbleVelocity, walkVelocity] → walkVelocity
  EXPECT_EQ(FloorVelocity(dribbleVelocity), walkVelocity);
  EXPECT_EQ(FloorVelocity(walkVelocity), walkVelocity);
}

TEST(VelocityStateTest, FloorVelocity_AboveWalkIsSprint) {
  EXPECT_EQ(FloorVelocity(walkVelocity + 0.01f), sprintVelocity);
  EXPECT_EQ(FloorVelocity(sprintVelocity), sprintVelocity);
}

// ---------------------------------------------------------------------------
// EnumToFloatVelocity – enum → canonical float
// ---------------------------------------------------------------------------

TEST(VelocityStateTest, EnumToFloat_AllValues) {
  EXPECT_EQ(EnumToFloatVelocity(e_Velocity_Idle), idleVelocity);
  EXPECT_EQ(EnumToFloatVelocity(e_Velocity_Dribble), dribbleVelocity);
  EXPECT_EQ(EnumToFloatVelocity(e_Velocity_Walk), walkVelocity);
  EXPECT_EQ(EnumToFloatVelocity(e_Velocity_Sprint), sprintVelocity);
}

// ---------------------------------------------------------------------------
// FloatToEnumVelocity – float → enum (via RangeVelocity)
// ---------------------------------------------------------------------------

TEST(VelocityStateTest, FloatToEnum_IdleBand) {
  EXPECT_EQ(FloatToEnumVelocity(0.0f), e_Velocity_Idle);
  EXPECT_EQ(FloatToEnumVelocity(1.79f), e_Velocity_Idle);
}

TEST(VelocityStateTest, FloatToEnum_DribbleBand) {
  EXPECT_EQ(FloatToEnumVelocity(idleDribbleSwitch), e_Velocity_Dribble);
  EXPECT_EQ(FloatToEnumVelocity(3.0f), e_Velocity_Dribble);
  EXPECT_EQ(FloatToEnumVelocity(4.19f), e_Velocity_Dribble);
}

TEST(VelocityStateTest, FloatToEnum_WalkBand) {
  EXPECT_EQ(FloatToEnumVelocity(dribbleWalkSwitch), e_Velocity_Walk);
  EXPECT_EQ(FloatToEnumVelocity(5.0f), e_Velocity_Walk);
  EXPECT_EQ(FloatToEnumVelocity(5.99f), e_Velocity_Walk);
}

TEST(VelocityStateTest, FloatToEnum_SprintBand) {
  EXPECT_EQ(FloatToEnumVelocity(walkSprintSwitch), e_Velocity_Sprint);
  EXPECT_EQ(FloatToEnumVelocity(8.0f), e_Velocity_Sprint);
}

// ---------------------------------------------------------------------------
// Round-trip: EnumToFloat → FloatToEnum must be identity
// ---------------------------------------------------------------------------

TEST(VelocityStateTest, RoundTrip_EnumFloatEnum) {
  for (auto v :
       {e_Velocity_Idle, e_Velocity_Dribble, e_Velocity_Walk, e_Velocity_Sprint}) {
    EXPECT_EQ(FloatToEnumVelocity(EnumToFloatVelocity(v)), v);
  }
}

// ---------------------------------------------------------------------------
// Boundary values at the exact switch thresholds
// ---------------------------------------------------------------------------

TEST(VelocityStateTest, Boundaries_AtSwitchPoints) {
  EXPECT_EQ(FloatToEnumVelocity(idleDribbleSwitch), e_Velocity_Dribble);
  EXPECT_EQ(FloatToEnumVelocity(dribbleWalkSwitch), e_Velocity_Walk);
  EXPECT_EQ(FloatToEnumVelocity(walkSprintSwitch), e_Velocity_Sprint);
}

}  // namespace
