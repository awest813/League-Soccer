#include "keyboard.hpp"

#include "../main.hpp"
#include "managers/usereventmanager.hpp"

HIDKeyboard::HIDKeyboard() {
  deviceType = e_HIDeviceType_Keyboard;
  identifier = "keyboard";

  LoadConfig();
}

HIDKeyboard::~HIDKeyboard() {}

SDL_Keycode HIDKeyboard::DefaultKey(int buttonFunction) {
  if (buttonFunction < 0 || buttonFunction >= e_ButtonFunction_Size)
    return SDLK_UNKNOWN;
  const SDL_Keycode key = SDL_GetKeyFromScancode(defaultKeyScancodes[buttonFunction]);
  return key != SDLK_UNKNOWN ? key : defaultKeyIDs[buttonFunction];
}

void HIDKeyboard::LoadConfig() {
  keymapGeneration = UserEventManager::GetInstance().GetKeymapGeneration();
  std::unique_lock<std::mutex> blah(mutex);
  for (int i = 0; i < e_ButtonFunction_Size; i++) {
    functionButtonState[i] = false;
    previousFunctionButtonState[i] = false;

    // Prefer the saved physical key; fall back to a keycode saved by older
    // builds, then to the layout-aware default.
    const std::string scancodeKey = "input_keyboard_sc_" + int_to_str(i);
    const std::string keycodeKey = "input_keyboard_" + int_to_str(i);
    SDL_Keycode key = SDLK_UNKNOWN;
    if (GetConfiguration()->Exists(scancodeKey.c_str())) {
      const int scancode = GetConfiguration()->GetInt(scancodeKey.c_str(), 0);
      if (scancode > SDL_SCANCODE_UNKNOWN && scancode < SDL_NUM_SCANCODES)
        key = SDL_GetKeyFromScancode(static_cast<SDL_Scancode>(scancode));
    }
    if (key == SDLK_UNKNOWN && GetConfiguration()->Exists(keycodeKey.c_str()))
      key = (SDL_Keycode)GetConfiguration()->GetInt(keycodeKey.c_str(), SDLK_UNKNOWN);
    functionMapping[i] = key != SDLK_UNKNOWN ? key : DefaultKey(i);
  }
}

void HIDKeyboard::SaveConfig() {
  std::unique_lock<std::mutex> blah(mutex);
  for (int i = 0; i < e_ButtonFunction_Size; i++) {
    GetConfiguration()->SetInt(("input_keyboard_sc_" + int_to_str(i)).c_str(),
                               SDL_GetScancodeFromKey(functionMapping[i]));
    // Keycode too, for older builds reading the same config.
    GetConfiguration()->SetInt(("input_keyboard_" + int_to_str(i)).c_str(), functionMapping[i]);
  }
  GetConfiguration()->SaveFile(GetConfigFilename());
}

void HIDKeyboard::Process() {
  // Re-resolve physical bindings when the OS keyboard layout changes (SDL
  // also reports this once the window's keymap is first known).
  if (keymapGeneration != UserEventManager::GetInstance().GetKeymapGeneration())
    LoadConfig();

  std::unique_lock<std::mutex> blah(mutex);
  for (int i = 0; i < e_ButtonFunction_Size; i++) {
    previousFunctionButtonState[i] = functionButtonState[i];
    functionButtonState[i] = UserEventManager::GetInstance().GetKeyboardState(functionMapping[i]);
  }
}

bool HIDKeyboard::GetButton(e_ButtonFunction buttonFunction) {
  std::unique_lock<std::mutex> blah(mutex);
  return functionButtonState[buttonFunction];
}

float HIDKeyboard::GetButtonValue(e_ButtonFunction buttonFunction) {
  std::unique_lock<std::mutex> blah(mutex);
  if (functionButtonState[buttonFunction])
    return 1.0;
  else
    return 0.0;
}

void HIDKeyboard::SetButton(e_ButtonFunction buttonFunction, bool state) {
  std::unique_lock<std::mutex> blah(mutex);
  functionButtonState[buttonFunction] = state;
}

bool HIDKeyboard::GetPreviousButtonState(e_ButtonFunction buttonFunction) {
  std::unique_lock<std::mutex> blah(mutex);
  return previousFunctionButtonState[buttonFunction];
}

Vector3 HIDKeyboard::GetDirection() {
  Vector3 inputDirection;
  if (GetButton(e_ButtonFunction_Left))
    inputDirection.coords[0] -= 1;
  if (GetButton(e_ButtonFunction_Right))
    inputDirection.coords[0] += 1;
  if (GetButton(e_ButtonFunction_Up))
    inputDirection.coords[1] += 1;
  if (GetButton(e_ButtonFunction_Down))
    inputDirection.coords[1] -= 1;
  inputDirection.Normalize(0);
  return inputDirection;
}
