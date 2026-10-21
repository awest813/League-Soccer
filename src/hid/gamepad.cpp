#include "gamepad.hpp"

#include "../main.hpp"
#include "base/utils.hpp"
#include "deviceidentity.hpp"
#include "managers/usereventmanager.hpp"
#include "stickinput.hpp"

namespace {

// The raw SDL joystick API orders axes per platform: Linux evdev reports LT as
// axis 2, while XInput and SDL's HIDAPI drivers report LT as axis 4. RT is
// axis 5 on all of them. Opening devices through SDL_GameController would
// remove this guesswork (see docs/gap-list.md).
#if defined(__linux__)
const int kXboxLeftTriggerBinding = stickinput::EncodeAxisBinding(2, 1);
#else
const int kXboxLeftTriggerBinding = stickinput::EncodeAxisBinding(4, 1);
#endif

}  // namespace

HIDGamepad::HIDGamepad(int deviceIndex, int gamepadID)
    : deviceIndex(deviceIndex), gamepadID(gamepadID) {
  deviceType = e_HIDeviceType_Gamepad;
  const char* joyName = SDL_JoystickNameForIndex(deviceIndex);
  displayName = joyName ? joyName : "Gamepad";

  // Key saved settings by the device's GUID, not its connection order. Two
  // identical pads get "-2", "-3"... in the order they are connected.
  char guidText[33] = {0};
  SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(deviceIndex), guidText, sizeof(guidText));
  for (int duplicate = 0;; duplicate++) {
    identifier = deviceidentity::GamepadIdentifier(guidText, duplicate);
    bool taken = false;
    for (IHIDevice* device : GetControllers()) {
      if (device->GetDeviceType() == e_HIDeviceType_Gamepad &&
          device->GetIdentifier() == identifier)
        taken = true;
    }
    if (!taken)
      break;
  }

  // Carry settings saved by older builds under "<name> #<index>" over once.
  legacyIdentifier = deviceidentity::LegacyGamepadIdentifier(displayName, deviceIndex);
  deviceidentity::MigratePrefixedKeys(GetConfiguration()->GetMutableProperties(),
                                      "input_gamepad_" + legacyIdentifier + "_", ConfigPrefix());

  LoadConfig();
}

HIDGamepad::~HIDGamepad() {}

void HIDGamepad::LoadConfig() {
  std::unique_lock<std::mutex> blah(mutex);

  for (int i = 0; i < e_ControllerButton_Size; i++) {
    controllerButtonState[i] = false;
    previousControllerButtonState[i] = false;
  }

  for (int i = 0; i < _JOYSTICK_MAXAXES; i++) {
    float min = GetConfiguration()->GetReal(
        ("input_gamepad_" + GetIdentifier() + "_calibration_" + int_to_str(i) + "_min").c_str(),
        -32768);
    float max = GetConfiguration()->GetReal(
        ("input_gamepad_" + GetIdentifier() + "_calibration_" + int_to_str(i) + "_max").c_str(),
        32767);
    // Uncalibrated triggers default to resting at their initial (released)
    // value; a centre rest of 0 would read a released trigger as fully pulled
    // in the opposite direction.
    float initialState = 0.0f;
    float defaultRest = 0.0f;
    if (UserEventManager::GetInstance().GetJoystickAxisInitialState(GetGamepadID(), i,
                                                                    &initialState))
      defaultRest = stickinput::DefaultAxisRest(initialState);
    float rest = GetConfiguration()->GetReal(
        ("input_gamepad_" + GetIdentifier() + "_calibration_" + int_to_str(i) + "_rest").c_str(),
        defaultRest);
    UserEventManager::GetInstance().SetJoystickAxisCalibration(GetGamepadID(), i, min, max, rest);
  }

  // Smart controller profile auto-detection (by name; the identifier is a GUID)
  controllerType = DetectControllerType(GetDisplayName());

  for (int i = 0; i < e_ControllerButton_Size; i++) {
    int defaultButton = 0;
    if (i == 0)
      defaultButton = -3;  // Up
    else if (i == 1)
      defaultButton = -2;  // Right
    else if (i == 2)
      defaultButton = -4;  // Down
    else if (i == 3)
      defaultButton = -1;  // Left
    else if (i == 4) {     // Top Face (Y / Triangle / X on Switch)
      if (controllerType == e_ControllerType_NintendoSwitch)
        defaultButton = 3;  // X on Switch
      else
        defaultButton = 3;  // Y / Triangle
    } else if (i == 5) {    // Right Face (B / Circle / A on Switch)
      if (controllerType == e_ControllerType_NintendoSwitch)
        defaultButton = 1;  // A on Switch (physically right)
      else
        defaultButton = 1;  // B / Circle
    } else if (i == 6) {    // Bottom Face (A / Cross / B on Switch)
      if (controllerType == e_ControllerType_NintendoSwitch)
        defaultButton = 0;  // B on Switch (physically bottom)
      else
        defaultButton = 0;  // A / Cross
    } else if (i == 7) {    // Left Face (X / Square / Y on Switch)
      if (controllerType == e_ControllerType_NintendoSwitch)
        defaultButton = 2;  // Y on Switch (physically left)
      else
        defaultButton = 2;  // X / Square
    } else if (i == 8)
      defaultButton = 4;  // L1 / LB
    else if (i == 9) {    // L2 / LT
      if (controllerType == e_ControllerType_PlayStation)
        defaultButton = 6;
      else if (controllerType == e_ControllerType_NintendoSwitch ||
               controllerType == e_ControllerType_LogitechDirectInput)
        defaultButton = 6;
      else
        defaultButton = kXboxLeftTriggerBinding;
    } else if (i == 10)
      defaultButton = 5;  // R1 / RB
    else if (i == 11) {   // R2 / RT
      if (controllerType == e_ControllerType_PlayStation)
        defaultButton = 7;
      else if (controllerType == e_ControllerType_NintendoSwitch ||
               controllerType == e_ControllerType_LogitechDirectInput)
        defaultButton = 7;
      else
        defaultButton = stickinput::EncodeAxisBinding(5, 1);  // Xbox RT: axis 5, positive
    } else if (i == 12) {                                     // Select / Share / Back / Minus
      if (controllerType == e_ControllerType_PlayStation)
        defaultButton = 8;
      else if (controllerType == e_ControllerType_NintendoSwitch)
        defaultButton = 8;  // Minus
      else if (controllerType == e_ControllerType_LogitechDirectInput)
        defaultButton = 8;
      else
        defaultButton = 6;  // Xbox Back
    } else if (i == 13) {   // Start / Options / Plus
      if (controllerType == e_ControllerType_PlayStation)
        defaultButton = 9;
      else if (controllerType == e_ControllerType_NintendoSwitch)
        defaultButton = 9;  // Plus
      else if (controllerType == e_ControllerType_LogitechDirectInput)
        defaultButton = 9;
      else
        defaultButton = 7;  // Xbox Start
    }

    controllerMapping[i] = GetConfiguration()->GetInt(
        ("input_gamepad_" + GetIdentifier() + "_" + int_to_str(i)).c_str(), defaultButton);
    // A hand-edited or corrupt config must not index past the button/axis arrays.
    int axis = 0;
    int sign = 0;
    const bool isAxis = stickinput::DecodeAxisBinding(controllerMapping[i], &axis, &sign);
    if ((isAxis && axis >= _JOYSTICK_MAXAXES) ||
        (!isAxis && controllerMapping[i] >= _JOYSTICK_MAXBUTTONS))
      controllerMapping[i] = defaultButton;
  }

  preset = controlpresets::GamepadPresetFromInt(
      GetConfiguration()->GetInt((ConfigPrefix() + "preset").c_str(), 0));
  invertVertical = GetConfiguration()->GetBool((ConfigPrefix() + "invert_vertical").c_str(), false);
  stickDeadzone = controlpresets::ClampStickDeadzone(GetConfiguration()->GetReal(
      (ConfigPrefix() + "stick_deadzone").c_str(), controlpresets::kDefaultStickDeadzone));
  rumbleStrength = controlpresets::ClampRumbleStrength(
      GetConfiguration()->GetReal((ConfigPrefix() + "rumble").c_str(), 1.0f));

  for (int i = 0; i < e_ButtonFunction_Size; i++) {
    const int defaultMapping =
        controlpresets::PresetButtonFor(preset, static_cast<e_ButtonFunction>(i));
    int mapping = GetConfiguration()->GetInt(
        ("input_gamepad_" + GetIdentifier() + "_mapping_" + int_to_str(i)).c_str(), defaultMapping);
    if (mapping < 0 || mapping >= e_ControllerButton_Size)
      mapping = defaultMapping;
    functionMapping[i] = (e_ControllerButton)mapping;
  }
}

void HIDGamepad::SaveConfig() {
  std::unique_lock<std::mutex> blah(mutex);
  for (int i = 0; i < e_ControllerButton_Size; i++) {
    GetConfiguration()->Set(("input_gamepad_" + GetIdentifier() + "_" + int_to_str(i)).c_str(),
                            controllerMapping[i]);
  }
  for (int i = 0; i < e_ButtonFunction_Size; i++) {
    GetConfiguration()->Set(
        ("input_gamepad_" + GetIdentifier() + "_mapping_" + int_to_str(i)).c_str(),
        functionMapping[i]);
  }
  SaveOptions();
  GetConfiguration()->SaveFile(GetConfigFilename());
}

void HIDGamepad::ResetToDefaults() {
  // LoadConfig() prefers saved keys over defaults, so they must go first.
  // Legacy keys go too, or they would be migrated back on the next launch.
  GetConfiguration()->EraseWithPrefix(ConfigPrefix());
  GetConfiguration()->EraseWithPrefix("input_gamepad_" + legacyIdentifier + "_");
  LoadConfig();
  SaveConfig();
}

void HIDGamepad::ApplyPreset(controlpresets::GamepadPreset newPreset) {
  {
    std::unique_lock<std::mutex> blah(mutex);
    preset = newPreset;
    for (int i = 0; i < e_ButtonFunction_Size; i++)
      functionMapping[i] =
          controlpresets::PresetButtonFor(preset, static_cast<e_ButtonFunction>(i));
  }
  SaveConfig();
}

void HIDGamepad::SetInvertVertical(bool invert) {
  invertVertical = invert;
  SaveConfig();
}

void HIDGamepad::SetStickDeadzone(float deadzone) {
  stickDeadzone = controlpresets::ClampStickDeadzone(deadzone);
  SaveConfig();
}

void HIDGamepad::SetRumbleStrength(float strength) {
  rumbleStrength = controlpresets::ClampRumbleStrength(strength);
  SaveConfig();
}

void HIDGamepad::SaveOptions() {
  GetConfiguration()->SetInt((ConfigPrefix() + "preset").c_str(), static_cast<int>(preset));
  GetConfiguration()->SetBool((ConfigPrefix() + "invert_vertical").c_str(), invertVertical);
  GetConfiguration()->Set((ConfigPrefix() + "stick_deadzone").c_str(), stickDeadzone);
  GetConfiguration()->Set((ConfigPrefix() + "rumble").c_str(), rumbleStrength);
}

void HIDGamepad::Process() {
  std::unique_lock<std::mutex> blah(mutex);
  for (int i = 0; i < e_ControllerButton_Size; i++) {
    previousControllerButtonState[i] = controllerButtonState[i];
    signed int buttonID = controllerMapping[i];
    if (buttonID >= 0) {  // button
      controllerButtonState[i] =
          UserEventManager::GetInstance().GetJoyButtonState(gamepadID, buttonID) ? 1.0 : 0.0;
    } else {  // axis
      int axisID = 0;
      int sign = 0;
      stickinput::DecodeAxisBinding(buttonID, &axisID, &sign);
      // Movement directions get one radial deadzone in GetDirection(); a
      // per-axis deadzone here would snap near-cardinal stick angles onto the
      // axes. Buttons and triggers keep the per-axis deadzone.
      const bool isDirection = i <= e_ControllerButton_Left;
      float value =
          UserEventManager::GetInstance().GetJoystickAxis(gamepadID, axisID, !isDirection);
      if ((sign < 0 && value < 0) || (sign > 0 && value > 0))
        controllerButtonState[i] = fabs(value);
      else
        controllerButtonState[i] = 0;
    }
  }
}

bool HIDGamepad::GetButton(e_ButtonFunction buttonFunction) {
  std::unique_lock<std::mutex> blah(mutex);
  return controllerButtonState[functionMapping[buttonFunction]] > 0.0f;
}

float HIDGamepad::GetButtonValue(e_ButtonFunction buttonFunction) {
  std::unique_lock<std::mutex> blah(mutex);
  return controllerButtonState[functionMapping[buttonFunction]];
}

void HIDGamepad::SetButton(e_ButtonFunction buttonFunction, bool state) {
  std::unique_lock<std::mutex> blah(mutex);
  controllerButtonState[functionMapping[buttonFunction]] = state;
}

bool HIDGamepad::GetPreviousButtonState(e_ButtonFunction buttonFunction) {
  std::unique_lock<std::mutex> blah(mutex);
  return previousControllerButtonState[functionMapping[buttonFunction]];
}

Vector3 HIDGamepad::GetDirection() {
  Vector3 inputDirection;
  inputDirection.coords[0] -= GetButtonValue(e_ButtonFunction_Left);
  inputDirection.coords[0] += GetButtonValue(e_ButtonFunction_Right);
  inputDirection.coords[1] += GetButtonValue(e_ButtonFunction_Up);
  inputDirection.coords[1] -= GetButtonValue(e_ButtonFunction_Down);
  if (invertVertical)
    inputDirection.coords[1] = -inputDirection.coords[1];
  return stickinput::ApplyRadialDeadzone(inputDirection, stickDeadzone);
}

void HIDGamepad::SetRumble(float low_frequency, float high_frequency, unsigned int duration_ms) {
  if (rumbleStrength <= 0.0f)
    return;
  UserEventManager::GetInstance().SetJoystickRumble(gamepadID, low_frequency * rumbleStrength,
                                                    high_frequency * rumbleStrength, duration_ms);
}

e_ControllerType HIDGamepad::DetectControllerType(const std::string& name) {
  std::string lower = name;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

  // PlayStation check
  if (lower.find("sony") != std::string::npos || lower.find("dualshock") != std::string::npos ||
      lower.find("dualsense") != std::string::npos || lower.find("ps4") != std::string::npos ||
      lower.find("ps5") != std::string::npos || lower.find("ps3") != std::string::npos ||
      lower.find("playstation") != std::string::npos ||
      lower.find("sixaxis") != std::string::npos || lower.find("054c") != std::string::npos) {
    return e_ControllerType_PlayStation;
  }

  // Nintendo Switch check
  if (lower.find("nintendo") != std::string::npos || lower.find("switch") != std::string::npos ||
      lower.find("joy-con") != std::string::npos ||
      lower.find("pro controller") != std::string::npos ||
      lower.find("057e") != std::string::npos) {
    return e_ControllerType_NintendoSwitch;
  }

  // Logitech DirectInput check
  if (lower.find("logitech") != std::string::npos || lower.find("f310") != std::string::npos ||
      lower.find("f710") != std::string::npos || lower.find("f510") != std::string::npos ||
      lower.find("dual action") != std::string::npos ||
      lower.find("wingman") != std::string::npos) {
    return e_ControllerType_LogitechDirectInput;
  }

  // Xbox check
  if (lower.find("xbox") != std::string::npos || lower.find("x-box") != std::string::npos ||
      lower.find("xinput") != std::string::npos || lower.find("microsoft") != std::string::npos ||
      lower.find("045e") != std::string::npos || lower.find("8bitdo") != std::string::npos ||
      lower.find("pdp") != std::string::npos || lower.find("razer") != std::string::npos ||
      lower.find("powera") != std::string::npos) {
    return e_ControllerType_Xbox;
  }

  return e_ControllerType_Generic;
}

std::string HIDGamepad::GetControllerTypeName() const {
  switch (controllerType) {
    case e_ControllerType_Xbox:
      return "Xbox Controller";
    case e_ControllerType_PlayStation:
      return "PlayStation Controller";
    case e_ControllerType_NintendoSwitch:
      return "Nintendo Switch Controller";
    case e_ControllerType_LogitechDirectInput:
      return "Logitech Gamepad";
    case e_ControllerType_Generic:
    default:
      return "Standard Gamepad";
  }
}

std::string HIDGamepad::GetButtonGlyphPath(e_ButtonFunction buttonFunction) const {
  e_ControllerButton mappedBtn;
  {
    // Temporarily cast away const to lock the mutex and access functionMapping
    std::unique_lock<std::mutex> blah(const_cast<std::mutex&>(mutex));
    mappedBtn = functionMapping[buttonFunction];
  }

  if (controllerType == e_ControllerType_PlayStation) {
    switch (mappedBtn) {
      case e_ControllerButton_A:
        return "media/menu/buttons/ps_cross.png";
      case e_ControllerButton_B:
        return "media/menu/buttons/ps_circle.png";
      case e_ControllerButton_X:
        return "media/menu/buttons/ps_square.png";
      case e_ControllerButton_Y:
        return "media/menu/buttons/ps_triangle.png";
      case e_ControllerButton_L1:
        return "media/menu/buttons/ps_l1.png";
      case e_ControllerButton_L2:
        return "media/menu/buttons/ps_l2.png";
      case e_ControllerButton_R1:
        return "media/menu/buttons/ps_r1.png";
      case e_ControllerButton_R2:
        return "media/menu/buttons/ps_r2.png";
      case e_ControllerButton_Start:
        return "media/menu/buttons/ps_options.png";
      case e_ControllerButton_Select:
        return "media/menu/buttons/ps_share.png";
      case e_ControllerButton_Up:
        return "media/menu/buttons/xbox_dpad_up.png";
      case e_ControllerButton_Down:
        return "media/menu/buttons/xbox_dpad_down.png";
      case e_ControllerButton_Left:
        return "media/menu/buttons/xbox_dpad_left.png";
      case e_ControllerButton_Right:
        return "media/menu/buttons/xbox_dpad_right.png";
      default:
        break;
    }
  } else if (controllerType == e_ControllerType_NintendoSwitch) {
    switch (mappedBtn) {
      case e_ControllerButton_A:
        return "media/menu/buttons/switch_b.png";
      case e_ControllerButton_B:
        return "media/menu/buttons/switch_a.png";
      case e_ControllerButton_X:
        return "media/menu/buttons/switch_y.png";
      case e_ControllerButton_Y:
        return "media/menu/buttons/switch_x.png";
      case e_ControllerButton_L1:
        return "media/menu/buttons/xbox_lb.png";
      case e_ControllerButton_L2:
        return "media/menu/buttons/xbox_lt.png";
      case e_ControllerButton_R1:
        return "media/menu/buttons/xbox_rb.png";
      case e_ControllerButton_R2:
        return "media/menu/buttons/xbox_rt.png";
      default:
        break;
    }
  }

  // Default Xbox / Standard layout
  switch (mappedBtn) {
    case e_ControllerButton_A:
      return "media/menu/buttons/xbox_a.png";
    case e_ControllerButton_B:
      return "media/menu/buttons/xbox_b.png";
    case e_ControllerButton_X:
      return "media/menu/buttons/xbox_x.png";
    case e_ControllerButton_Y:
      return "media/menu/buttons/xbox_y.png";
    case e_ControllerButton_L1:
      return "media/menu/buttons/xbox_lb.png";
    case e_ControllerButton_L2:
      return "media/menu/buttons/xbox_lt.png";
    case e_ControllerButton_R1:
      return "media/menu/buttons/xbox_rb.png";
    case e_ControllerButton_R2:
      return "media/menu/buttons/xbox_rt.png";
    case e_ControllerButton_Start:
      return "media/menu/buttons/xbox_start.png";
    case e_ControllerButton_Select:
      return "media/menu/buttons/xbox_back.png";
    case e_ControllerButton_Up:
      return "media/menu/buttons/xbox_dpad_up.png";
    case e_ControllerButton_Down:
      return "media/menu/buttons/xbox_dpad_down.png";
    case e_ControllerButton_Left:
      return "media/menu/buttons/xbox_dpad_left.png";
    case e_ControllerButton_Right:
      return "media/menu/buttons/xbox_dpad_right.png";
    default:
      return "media/menu/buttons/xbox_a.png";
  }
}
