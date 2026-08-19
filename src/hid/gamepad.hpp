#ifndef _HPP_HIDGAMEPAD
#define _HPP_HIDGAMEPAD

#include "base/math/vector3.hpp"
#include "ihidevice.hpp"

using namespace blunted;

enum e_ControllerType {
  e_ControllerType_Xbox,
  e_ControllerType_PlayStation,
  e_ControllerType_NintendoSwitch,
  e_ControllerType_LogitechDirectInput,
  e_ControllerType_Generic
};

class HIDGamepad : public IHIDevice {
public:
  // deviceIndex: SDL device index at connect time (used only for naming / the
  //   config key, so it stays stable within a connection and across the
  //   slot renumbering that happens on hotplug).
  // gamepadID: the dense slot in UserEventManager used to sample this joystick.
  //   This is kept dense 0..count-1 and renumbered when a device is removed.
  HIDGamepad(int deviceIndex, int gamepadID);
  virtual ~HIDGamepad();

  virtual void LoadConfig();
  virtual void SaveConfig();

  virtual void Process();

  virtual bool GetButton(e_ButtonFunction buttonFunction);
  virtual float GetButtonValue(e_ButtonFunction buttonFunction);  // for analog support
  virtual void SetButton(e_ButtonFunction buttonFunction, bool state);
  virtual bool GetPreviousButtonState(e_ButtonFunction buttonFunction);
  virtual Vector3 GetDirection();

  e_ControllerType GetControllerType() const { return controllerType; }
  std::string GetControllerTypeName() const;
  static e_ControllerType DetectControllerType(const std::string& name);
  std::string GetButtonGlyphPath(e_ButtonFunction buttonFunction) const;

  e_ControllerButton GetFunctionMapping(e_ButtonFunction buttonFunction) {
    std::unique_lock<std::mutex> blah(mutex);
    return functionMapping[buttonFunction];
  }
  void SetFunctionMapping(e_ButtonFunction buttonFunction, e_ControllerButton controllerButton) {
    std::unique_lock<std::mutex> blah(mutex);
    functionMapping[buttonFunction] = controllerButton;
  }
  signed int GetControllerMapping(e_ControllerButton controllerButton) {
    std::unique_lock<std::mutex> blah(mutex);
    return controllerMapping[controllerButton];
  }
  void SetControllerMapping(e_ControllerButton controllerButton, signed int id) {
    std::unique_lock<std::mutex> blah(mutex);
    controllerMapping[controllerButton] = id;
  }

  int GetGamepadID() { return gamepadID; }
  void SetGamepadID(int id) { gamepadID = id; }

protected:
  int deviceIndex;
  int gamepadID;
  e_ControllerType controllerType;
  float controllerButtonState[e_ControllerButton_Size];
  float previousControllerButtonState[e_ControllerButton_Size];

  e_ControllerButton functionMapping[e_ButtonFunction_Size];
  signed int controllerMapping[e_ControllerButton_Size];
};

#endif
