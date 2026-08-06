// written by bastiaan konings schuiling 2008 - 2015
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not
// be used for anything important. i do not offer support, so don't ask. to be used for inspiration
// :)

#include "scriptedgamepad.hpp"

namespace {

// Scheduling (in units of controller input reads; robust to match time scale).
constexpr int kActionHoldReads = 6;
constexpr int kIdleGapReads = 55;

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
    printf("[menu-smoke] scripted gamepad: input sampled, driving a human player in a live match\n");
  }

  // Coarse action scheduler: hold a pass/shot for a few reads, then rest.
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
  if (buttonFunction == e_ButtonFunction_ShortPass || buttonFunction == e_ButtonFunction_Shot)
    return (actionTimer > 0) ? 1.0f : 0.0f;
  return 0.0f;
}

void ScriptedGamepad::SetButton(e_ButtonFunction, bool) {}

bool ScriptedGamepad::GetPreviousButtonState(e_ButtonFunction) {
  return false;
}
