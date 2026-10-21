// Regressions for the October 2026 controls audit (docs/gap-list.md).

#include <cmath>

#include <gtest/gtest.h>

#include "hid/stickinput.hpp"
#include "onthepitch/playercontrolsettings.hpp"
#include "onthepitch/shotaim.hpp"
#include "onthepitch/sprinttap.hpp"
#include "utils/gui2/menurepeat.hpp"

namespace {

using blunted::MenuRepeat;
using blunted::Vector3;

// Steady tapping used to knock on with every tap: the press that completed a
// pair also counted as the first press of the next pair.
TEST(ControlsAudit, SprintTapPairsDoNotChain) {
  SprintTap tap;
  tap.Update(0, true, true);
  tap.Update(150, true, true);
  EXPECT_TRUE(tap.Consume());
  tap.Update(300, true, true);
  EXPECT_FALSE(tap.Consume());
  tap.Update(450, true, true);
  EXPECT_TRUE(tap.Consume());
}

// The dribbler keeps control in both switching modes; previously Fully Manual
// switched away on any press, breaking the chip and one-two modifiers.
TEST(ControlsAudit, SwitchPressNeverTakesTheBallCarrier) {
  EXPECT_FALSE(ShouldSwitchOnPress(PlayerSwitchMode::FullyManual, true, true));
  EXPECT_FALSE(ShouldSwitchOnPress(PlayerSwitchMode::Assisted, true, true));
  EXPECT_TRUE(ShouldSwitchOnPress(PlayerSwitchMode::FullyManual, false, false));
  EXPECT_TRUE(ShouldSwitchOnPress(PlayerSwitchMode::Assisted, false, true));
  EXPECT_FALSE(ShouldSwitchOnPress(PlayerSwitchMode::Assisted, false, false));
}

TEST(ControlsAudit, ShotAimWithoutInputKeepsCommittedDirection) {
  const Vector3 committed(1, 0, 0);
  const Vector3 result = RefineShotDirection(committed, Vector3(0), 0.3f);
  EXPECT_NEAR(result.GetDistance(committed), 0.0f, 0.0001f);
}

// The negative axis encoding used by gamepad mappings must round-trip, and the
// Xbox right trigger must be axis 5 positive (the old default -5 was axis 2
// negative, i.e. the left trigger at rest).
TEST(ControlsAudit, AxisBindingsRoundTrip) {
  int axis = -1;
  int sign = 0;
  EXPECT_FALSE(stickinput::DecodeAxisBinding(0, &axis, &sign));
  EXPECT_FALSE(stickinput::DecodeAxisBinding(7, &axis, &sign));
  for (int a = 0; a < 8; ++a) {
    for (int s : {-1, 1}) {
      ASSERT_TRUE(stickinput::DecodeAxisBinding(stickinput::EncodeAxisBinding(a, s), &axis, &sign));
      EXPECT_EQ(axis, a);
      EXPECT_EQ(sign, s);
    }
  }
  // Existing defaults: -1 left, -2 right, -3 up, -4 down on axes 0/1.
  ASSERT_TRUE(stickinput::DecodeAxisBinding(-1, &axis, &sign));
  EXPECT_EQ(axis, 0);
  EXPECT_EQ(sign, -1);
  ASSERT_TRUE(stickinput::DecodeAxisBinding(-4, &axis, &sign));
  EXPECT_EQ(axis, 1);
  EXPECT_EQ(sign, 1);
  EXPECT_EQ(stickinput::EncodeAxisBinding(5, 1), -12);
  ASSERT_TRUE(stickinput::DecodeAxisBinding(-5, &axis, &sign));
  EXPECT_EQ(axis, 2);
  EXPECT_EQ(sign, -1);
}

TEST(ControlsAudit, TriggersRestAtTheirInitialValue) {
  EXPECT_EQ(stickinput::DefaultAxisRest(-32768.0f), -32768.0f);
  EXPECT_EQ(stickinput::DefaultAxisRest(32767.0f), 32767.0f);
  EXPECT_EQ(stickinput::DefaultAxisRest(0.0f), 0.0f);
  EXPECT_EQ(stickinput::DefaultAxisRest(1200.0f), 0.0f);  // stick drift is not a trigger
}

// A per-axis deadzone snapped near-cardinal angles onto the axes; the radial
// deadzone keeps the angle and magnitude.
TEST(ControlsAudit, RadialDeadzoneKeepsStickAngle) {
  EXPECT_EQ(stickinput::ApplyRadialDeadzone(Vector3(0.1f, 0.05f, 0), 0.15f).GetLength(), 0.0f);

  const Vector3 nearCardinal(0.98f, 0.17f, 0);  // ~10 degrees off the x axis
  const Vector3 out = stickinput::ApplyRadialDeadzone(nearCardinal, 0.15f);
  EXPECT_NEAR(out.coords[1], 0.17f, 0.0001f);
  EXPECT_NEAR(std::atan2(out.coords[1], out.coords[0]),
              std::atan2(nearCardinal.coords[1], nearCardinal.coords[0]), 0.0001f);

  const Vector3 partial(0.3f, 0.3f, 0);
  EXPECT_NEAR(stickinput::ApplyRadialDeadzone(partial, 0.15f).GetLength(), partial.GetLength(),
              0.0001f);
  EXPECT_NEAR(stickinput::ApplyRadialDeadzone(Vector3(1, 1, 0), 0.15f).GetLength(), 1.0f, 0.0001f);
}

// Menus used to repeat every GUI tick between 250 ms and the OS repeat delay,
// then stop once OS repeats refreshed the press time.
TEST(ControlsAudit, MenuRepeatHasInitialDelayThenSteadyRate) {
  MenuRepeat repeat;
  int steps = 0;
  for (unsigned long t = 0; t <= 1000; t += 10) {
    if (repeat.Update(true, t))
      steps++;
  }
  // t=0, then 400, 510, 620, 730, 840, 950.
  EXPECT_EQ(steps, 7);

  EXPECT_FALSE(repeat.Update(false, 1010));
  EXPECT_TRUE(repeat.Update(true, 1020));   // fresh press fires immediately
  EXPECT_FALSE(repeat.Update(true, 1030));  // then waits for the initial delay
}

}  // namespace
