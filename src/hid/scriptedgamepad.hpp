// written by bastiaan konings schuiling 2008 - 2015
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not
// be used for anything important. i do not offer support, so don't ask. to be used for inspiration
// :)

// ScriptedGamepad: an autonomous IHIDevice used only by the headless
// "gamepad controls" smoke match (data/menu_smoke_gamepad_match.config). It
// drives a single team through the normal HumanController path with scripted
// input, so CI can exercise the real controller -> human-player pipeline
// (button capture, movement, slot mapping) without a physical gamepad or a
// display interaction loop. It is intentionally excluded from real-gamepad
// handling (distinct device type, IsScriptedInput() == true).

#ifndef _HPP_HID_SCRIPTEDGAMEPAD
#define _HPP_HID_SCRIPTEDGAMEPAD

#include "base/math/vector3.hpp"
#include "ihidevice.hpp"

using namespace blunted;

class ScriptedGamepad : public IHIDevice {
public:
  ScriptedGamepad();
  virtual ~ScriptedGamepad();

  virtual void LoadConfig();
  virtual void SaveConfig();

  virtual void Process();

  virtual bool GetButton(e_ButtonFunction buttonFunction);
  virtual float GetButtonValue(e_ButtonFunction buttonFunction);  // for analog support
  virtual void SetButton(e_ButtonFunction buttonFunction, bool state);
  virtual bool GetPreviousButtonState(e_ButtonFunction buttonFunction);
  virtual Vector3 GetDirection();

  bool IsScriptedInput() const { return true; }

protected:
  unsigned long queryCount = 0;
  int actionTimer = 0;  // input reads remaining in the current action window
  int idleTimer = 0;    // input reads remaining in the idle gap in between
};

#endif
