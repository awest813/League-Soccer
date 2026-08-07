// written by bastiaan konings schuiling 2008 - 2015
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not
// be used for anything important. i do not offer support, so don't ask. to be used for inspiration
// :)

#include "scriptedgamepad.hpp"

namespace {

// Scheduling (in units of controller input reads; robust to match time scale).
constexpr int kActionHoldReads = 6;
constexpr int kIdleGapReads = 55;

// The match clock only advances while the ball is in play and no set piece is
// being taken (see Match::Process). A controlled player that constantly shoots
// or blasts the ball out of bounds forces throw-ins / goal-kicks / corners,
// which stall the clock and make a full simulated match unreasonably long
// (and flaky) to run under CI. So this headless driver deliberately plays the
// ball forward with short passes only and keeps its movement from leaving the
// action, so the match flows at roughly CPU-vs-CPU speed.

}  // namespace

ScriptedGamepad::ScriptedGamepad() {
  deviceType = e_HIDeviceType_Scripted;
  identifier = "scripted-gamepad";

  LoadConfig();
}

ScriptedGamepad::~ScriptedGamepad() {}

void ScriptedGamepad::LoadConfig() {}

void ScriptedGamepad::SaveConfig() {}

void ScriptedGamepad::Process() {}

Vector3 ScriptedGamepad::GetDirection() {
  queryCount++;
  if (queryCount == 1) {
    printf(
        "[menu-smoke] scripted gamepad: input sampled, driving a human player in a live match\n");
  }

  // Coarse action scheduler: hold a pass for a few reads, then rest. Only a
  // short pass is ever issued; shots are deliberately avoided because a human
  // player shooting at every opportunity blasts the ball out of play (goal
  // kicks, corners, throw-ins), which freezes the match clock and makes the
  // full simulated match far too slow to complete reliably under CI.
  if (actionTimer > 0) {
    actionTimer--;
  } else if (idleTimer > 0) {
    idleTimer--;
  } else {
    actionTimer = kActionHoldReads;
    idleTimer = kIdleGapReads;
  }

  // Keep moving upfield (never idle) so the human-selected player is actively
  // controlled for the duration of the simulated match.
  return Vector3(0.5, 1.0, 0.0);
}

bool ScriptedGamepad::GetButton(e_ButtonFunction buttonFunction) {
  return GetButtonValue(buttonFunction) > 0.5f;
}

float ScriptedGamepad::GetButtonValue(e_ButtonFunction buttonFunction) {
  // Short passes only (see GetDirection). Shot stays 0 so the driver never
  // whacks the ball toward the stands, which would stall the match clock.
  if (buttonFunction == e_ButtonFunction_ShortPass)
    return (actionTimer > 0) ? 1.0f : 0.0f;
  return 0.0f;
}

void ScriptedGamepad::SetButton(e_ButtonFunction, bool) {}

bool ScriptedGamepad::GetPreviousButtonState(e_ButtonFunction) {
  return false;
}
