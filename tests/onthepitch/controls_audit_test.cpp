// Regressions for the October 2026 controls audit (docs/gap-list.md).

#include <cmath>
#include <map>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "hid/controllersetup.hpp"
#include "hid/controlpresets.hpp"
#include "hid/deviceidentity.hpp"
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

// The PES preset must stay identical to the long-standing defaults so
// existing players' layouts do not change.
TEST(ControlsAudit, PesPresetMatchesHistoricDefaults) {
  using controlpresets::GamepadPreset;
  using controlpresets::PresetButtonFor;
  const e_ControllerButton expected[e_ButtonFunction_Size] = {
      e_ControllerButton_Up,   e_ControllerButton_Right,  e_ControllerButton_Down,
      e_ControllerButton_Left, e_ControllerButton_Y,      e_ControllerButton_B,
      e_ControllerButton_A,    e_ControllerButton_X,      e_ControllerButton_Y,
      e_ControllerButton_B,    e_ControllerButton_A,      e_ControllerButton_X,
      e_ControllerButton_L1,   e_ControllerButton_L2,     e_ControllerButton_R1,
      e_ControllerButton_R2,   e_ControllerButton_Select, e_ControllerButton_Start};
  for (int i = 0; i < e_ButtonFunction_Size; i++)
    EXPECT_EQ(PresetButtonFor(GamepadPreset::Pes, static_cast<e_ButtonFunction>(i)), expected[i])
        << "function " << i;
}

TEST(ControlsAudit, FifaPresetSwapsShootLobAndSprint) {
  using controlpresets::GamepadPreset;
  using controlpresets::PresetButtonFor;
  EXPECT_EQ(PresetButtonFor(GamepadPreset::Fifa, e_ButtonFunction_ShortPass), e_ControllerButton_A);
  EXPECT_EQ(PresetButtonFor(GamepadPreset::Fifa, e_ButtonFunction_Shot), e_ControllerButton_B);
  EXPECT_EQ(PresetButtonFor(GamepadPreset::Fifa, e_ButtonFunction_HighPass), e_ControllerButton_X);
  EXPECT_EQ(PresetButtonFor(GamepadPreset::Fifa, e_ButtonFunction_LongPass), e_ControllerButton_Y);
  EXPECT_EQ(PresetButtonFor(GamepadPreset::Fifa, e_ButtonFunction_Sprint), e_ControllerButton_R2);
  EXPECT_EQ(PresetButtonFor(GamepadPreset::Fifa, e_ButtonFunction_Dribble), e_ControllerButton_R1);
  // Defending is the same in both layouts.
  for (e_ButtonFunction f : {e_ButtonFunction_Pressure, e_ButtonFunction_Sliding,
                             e_ButtonFunction_TeamPressure, e_ButtonFunction_KeeperRush}) {
    EXPECT_EQ(PresetButtonFor(GamepadPreset::Fifa, f), PresetButtonFor(GamepadPreset::Pes, f));
  }
  // Unknown saved values fall back to PES.
  EXPECT_EQ(controlpresets::GamepadPresetFromInt(7), GamepadPreset::Pes);
  EXPECT_EQ(controlpresets::GamepadPresetFromInt(1), GamepadPreset::Fifa);
}

TEST(ControlsAudit, DeadzoneAndRumbleOptionsCycleAndClamp) {
  using namespace controlpresets;
  EXPECT_FLOAT_EQ(NextStickDeadzone(0.15f), 0.20f);
  EXPECT_FLOAT_EQ(NextStickDeadzone(0.30f), 0.15f);
  EXPECT_FLOAT_EQ(ClampStickDeadzone(0.01f), 0.15f);  // below the controller's own threshold
  EXPECT_FLOAT_EQ(ClampStickDeadzone(0.9f), 0.30f);
  EXPECT_FLOAT_EQ(ClampStickDeadzone(std::nanf("")), kDefaultStickDeadzone);
  EXPECT_FLOAT_EQ(NextRumbleStrength(1.0f), 0.0f);
  EXPECT_FLOAT_EQ(NextRumbleStrength(0.0f), 0.5f);
  EXPECT_FLOAT_EQ(NextRumbleStrength(0.5f), 1.0f);
  EXPECT_FLOAT_EQ(ClampRumbleStrength(3.0f), 1.0f);
}

// Settings were keyed by "<name> #<connection index>", so a pad lost its
// mapping when pads were plugged in a different order.
TEST(ControlsAudit, GamepadIdentityUsesGuidAndMigratesLegacyKeys) {
  using namespace deviceidentity;
  EXPECT_EQ(GamepadIdentifier("030000005e040000", 0), "030000005e040000");
  EXPECT_EQ(GamepadIdentifier("030000005e040000", 1), "030000005e040000-2");
  EXPECT_EQ(GamepadIdentifier("", 0), "gamepad");
  EXPECT_EQ(LegacyGamepadIdentifier("Xbox Controller", 1), "Xbox Controller #1");

  std::map<std::string, std::string> config = {
      {"input_gamepad_Xbox Controller #1_11", "-12"},
      {"input_gamepad_Xbox Controller #1_calibration_2_rest", "-32768"},
      {"input_gamepad_Xbox Controller #10_11", "7"},  // a different pad
      {"input_keyboard_0", "1073741906"}};
  EXPECT_EQ(MigratePrefixedKeys(config, "input_gamepad_Xbox Controller #1_", "input_gamepad_guid_"),
            2);
  EXPECT_EQ(config["input_gamepad_guid_11"], "-12");
  EXPECT_EQ(config["input_gamepad_guid_calibration_2_rest"], "-32768");
  EXPECT_EQ(config.count("input_gamepad_Xbox Controller #1_11"), 1u);  // kept for old builds

  // Never overwrite settings already saved under the new identity.
  config["input_gamepad_Xbox Controller #1_11"] = "3";
  EXPECT_EQ(MigratePrefixedKeys(config, "input_gamepad_Xbox Controller #1_", "input_gamepad_guid_"),
            0);
  EXPECT_EQ(config["input_gamepad_guid_11"], "-12");
}

struct TestSide {
  int controllerID;
  int side;
};

// Unplugging a pad shifts later devices down one index; the controller setup
// must follow, and report whether the lost pad was playing.
TEST(ControlsAudit, RemovingControllerRenumbersSetup) {
  std::vector<TestSide> sides = {{0, 0}, {1, -1}, {2, 1}, {3, 0}};
  EXPECT_EQ(controllersetup::RemoveControllerFromSetup(sides, 1), -1);
  ASSERT_EQ(sides.size(), 3u);
  EXPECT_EQ(sides[0].controllerID, 0);
  EXPECT_EQ(sides[1].controllerID, 1);
  EXPECT_EQ(sides[1].side, 1);
  EXPECT_EQ(sides[2].controllerID, 2);

  EXPECT_EQ(controllersetup::RemoveControllerFromSetup(sides, 2), 0);  // was not playing
  EXPECT_EQ(controllersetup::RemoveControllerFromSetup(sides, 9), 0);  // not in the setup
  EXPECT_EQ(sides.size(), 2u);
}

// LB before or after the action button must behave the same: no teammate run
// and no braking when LB is used as a modifier.
TEST(ControlsAudit, SwitchModifierIsOrderIndependent) {
  // LB first, then shot.
  SwitchModifierTracker lbFirst;
  EXPECT_FALSE(lbFirst.Update(true, false, false, 1000));
  EXPECT_FALSE(lbFirst.ShouldBrake(true, false, 1050));  // not yet held long enough
  EXPECT_FALSE(lbFirst.Update(true, true, true, 1060));  // shot armed while LB held
  EXPECT_FALSE(lbFirst.ShouldBrake(true, true, 1400));
  EXPECT_FALSE(lbFirst.Update(false, true, true, 1500));  // release: no run

  // Shot first, then LB.
  SwitchModifierTracker shotFirst;
  EXPECT_FALSE(shotFirst.Update(true, false, true, 1000));
  EXPECT_FALSE(shotFirst.ShouldBrake(true, true, 1400));
  EXPECT_FALSE(shotFirst.Update(false, true, false, 1500));

  // LB on its own: brakes after a moment, and asks for a run on release.
  SwitchModifierTracker alone;
  EXPECT_FALSE(alone.Update(true, false, false, 1000));
  EXPECT_FALSE(alone.ShouldBrake(true, false, 1100));
  EXPECT_TRUE(alone.ShouldBrake(true, false, 1200));
  EXPECT_TRUE(alone.Update(false, true, false, 1300));
  EXPECT_FALSE(alone.ShouldBrake(false, false, 1400));
}

}  // namespace
