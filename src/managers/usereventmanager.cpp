// Copyright 2019 Google LLC & Bastiaan Konings
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// written by bastiaan konings schuiling 2008 - 2014
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not
// be used for anything important. i do not offer support, so don't ask. to be used for inspiration
// :)

#include "usereventmanager.hpp"

#include "environmentmanager.hpp"

extern void AddGamepad(int deviceIndex, int gamepadID);
extern void RemoveGamepad(int gamepadID);

namespace blunted {

template <>
UserEventManager* Singleton<UserEventManager>::singleton = 0;

UserEventManager::UserEventManager() {
  lastKeyTime_ms = 0;

  // SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_INTERVAL);

  // yes, SDL starts mousebuttons at 1...
  for (int i = 1; i < 8; i++) {
    mousePressed[i] = false;
  }

  for (int j = 0; j < _JOYSTICK_MAX; j++) {
    joystick[j] = nullptr;
    joystickInstanceNow[j] = -1;
    for (int i = 0; i < _JOYSTICK_MAXBUTTONS; i++) {
      joyButtonPressed[j][i] = false;
    }
  }

  for (int j = 0; j < _JOYSTICK_MAX; j++) {
    for (int i = 0; i < _JOYSTICK_MAXAXES; i++) {
      joyAxis[j][i] = 0.0;
      joyAxisCalibration[j][i][0] = -32768.0;
      joyAxisCalibration[j][i][1] = 32767.0;
      joyAxisCalibration[j][i][2] = 0.0;
    }
  }

  // init the joy!

  SDL_Init(SDL_INIT_JOYSTICK);
  for (int i = 0; i < SDL_NumJoysticks(); i++) {
    joystick[i] = SDL_JoystickOpen(i);
    if (joystick[i])
      joystickInstanceNow[i] = SDL_JoystickInstanceID(joystick[i]);
  }
  // SDL_JoystickEventState(SDL_IGNORE); // doesn't seem to work? bug?
  SDL_JoystickEventState(SDL_ENABLE);
  // printf("JOYSTICK EVENT STATE: %i (%i = ignore, %i = enable)\n",
  // SDL_JoystickEventState(SDL_QUERY), SDL_IGNORE, SDL_ENABLE);
}

UserEventManager::~UserEventManager() {
  for (int i = 0; i < SDL_NumJoysticks(); i++) {
    SDL_JoystickClose(joystick[i]);
  }
}

void UserEventManager::Exit() {}

void UserEventManager::InputSDLEvent(const SDL_Event& event) {
  switch (event.type) {
    case SDL_JOYDEVICEADDED: {
      // SDL_JOYDEVICEADDED reports the *device index* for the new joystick,
      // suitable for SDL_JoystickOpen(). Allocate the next free dense slot and
      // remember the joystick's stable instance ID there.
      printf("[userevent] Gamepad connected: %d\n", event.jdevice.which);
      SDL_Joystick* joy = SDL_JoystickOpen(event.jdevice.which);
      if (!joy)
        break;
      std::unique_lock<std::mutex> lock(joyButtonPressedMutex);
      int slot = FindFreeJoystickSlot();
      if (slot < 0) {
        SDL_JoystickClose(joy);
        break;
      }
      joystick[slot] = joy;
      joystickInstanceNow[slot] = SDL_JoystickInstanceID(joy);
      for (int i = 0; i < _JOYSTICK_MAXBUTTONS; i++)
        joyButtonPressed[slot][i] = false;
      for (int i = 0; i < _JOYSTICK_MAXAXES; i++) {
        joyAxis[slot][i] = 0.0;
        joyAxisCalibration[slot][i][0] = -32768.0;
        joyAxisCalibration[slot][i][1] = 32767.0;
        joyAxisCalibration[slot][i][2] = 0.0;
      }
      lock.unlock();
      // Constructing the HIDGamepad reloads its config, which re-enters the
      // UserEventManager (SetJoystickAxisCalibration), so add it unlocked.
      AddGamepad(event.jdevice.which, slot);
      break;
    }
    case SDL_JOYDEVICEREMOVED: {
      // SDL_JOYDEVICEREMOVED reports the *instance ID* (not a device index).
      // Resolve it back to our dense slot, close the handle, drop it from the
      // controller list, then pack the remaining joysticks down so the slots
      // stay densely 0..count-1 (this is what avoids the index renumbering bug
      // that previously left stale device indices pointing at the wrong data).
      printf("[userevent] Gamepad disconnected: %d\n", event.jdevice.which);
      std::unique_lock<std::mutex> lock(joyButtonPressedMutex);
      int slot = FindJoystickSlot(event.jdevice.which);
      if (slot < 0)
        break;
      if (joystick[slot]) {
        SDL_JoystickClose(joystick[slot]);
        joystick[slot] = nullptr;
      }
      joystickInstanceNow[slot] = -1;
      lock.unlock();
      RemoveGamepad(slot);
      lock.lock();
      CompactJoystickSlots(slot);
      break;
    }
    case SDL_KEYDOWN:
      keyPressedMutex.lock();
      keyPressed[event.key.keysym.sym].pressTime_ms =
          EnvironmentManager::GetInstance().GetTime_ms();
      lastKeyTime_ms = keyPressed[event.key.keysym.sym].pressTime_ms;
      keyPressedMutex.unlock();
      break;
    case SDL_KEYUP:
      keyPressedMutex.lock();
      keyPressed.erase(event.key.keysym.sym);
      keyPressedMutex.unlock();
      break;
    case SDL_MOUSEBUTTONDOWN:
      mousePressedMutex.lock();
      mousePressed[event.button.button] = true;
      mousePressedMutex.unlock();
      break;
    case SDL_MOUSEBUTTONUP:
      mousePressedMutex.lock();
      mousePressed[event.button.button] = false;
      mousePressedMutex.unlock();
      break;
    case SDL_JOYAXISMOTION: {
      std::unique_lock<std::mutex> lock(joyButtonPressedMutex);
      int joyID = FindJoystickSlot(event.jaxis.which);
      if (joyID >= 0)
        joyAxis[joyID][event.jaxis.axis] = event.jaxis.value;
      break;
    }
    case SDL_JOYBUTTONDOWN: {
      std::unique_lock<std::mutex> lock(joyButtonPressedMutex);
      int joyID = FindJoystickSlot(event.jbutton.which);
      if (joyID >= 0)
        joyButtonPressed[joyID][event.jbutton.button] = true;
      break;
    }
    case SDL_JOYBUTTONUP: {
      std::unique_lock<std::mutex> lock(joyButtonPressedMutex);
      int joyID = FindJoystickSlot(event.jbutton.which);
      if (joyID >= 0)
        joyButtonPressed[joyID][event.jbutton.button] = false;
      break;
    }
  }
}

int UserEventManager::FindJoystickSlot(SDL_JoystickID instance) const {
  for (int s = 0; s < _JOYSTICK_MAX; s++) {
    if (joystickInstanceNow[s] == instance)
      return s;
  }
  return -1;
}

int UserEventManager::FindFreeJoystickSlot() const {
  for (int s = 0; s < _JOYSTICK_MAX; s++) {
    if (joystickInstanceNow[s] == -1)
      return s;
  }
  return -1;
}

void UserEventManager::CompactJoystickSlots(int removedSlot) {
  for (int s = removedSlot; s + 1 < _JOYSTICK_MAX; s++) {
    joystick[s] = joystick[s + 1];
    joystickInstanceNow[s] = joystickInstanceNow[s + 1];
    for (int i = 0; i < _JOYSTICK_MAXBUTTONS; i++)
      joyButtonPressed[s][i] = joyButtonPressed[s + 1][i];
    for (int i = 0; i < _JOYSTICK_MAXAXES; i++) {
      joyAxis[s][i] = joyAxis[s + 1][i];
      joyAxisCalibration[s][i][0] = joyAxisCalibration[s + 1][i][0];
      joyAxisCalibration[s][i][1] = joyAxisCalibration[s + 1][i][1];
      joyAxisCalibration[s][i][2] = joyAxisCalibration[s + 1][i][2];
    }
  }
  int top = _JOYSTICK_MAX - 1;
  joystick[top] = nullptr;
  joystickInstanceNow[top] = -1;
  for (int i = 0; i < _JOYSTICK_MAXBUTTONS; i++)
    joyButtonPressed[top][i] = false;
  for (int i = 0; i < _JOYSTICK_MAXAXES; i++) {
    joyAxis[top][i] = 0.0;
    joyAxisCalibration[top][i][0] = -32768.0;
    joyAxisCalibration[top][i][1] = 32767.0;
    joyAxisCalibration[top][i][2] = 0.0;
  }
}

bool UserEventManager::GetKeyboardState(SDL_Keycode code) const {
  std::unique_lock<std::mutex> lock(keyPressedMutex);
  return keyPressed.count(code) > 0;
}

std::map<SDL_Keycode, TimedKeyPress> UserEventManager::GetKeyboardState() const {
  std::unique_lock<std::mutex> lock(keyPressedMutex);
  return keyPressed;
}

void UserEventManager::SetKeyboardState(SDL_Keycode key, bool newState) {
  std::unique_lock<std::mutex> lock(keyPressedMutex);
  if (!newState) {
    keyPressed.erase(key);
  } else {
    keyPressed[key].pressTime_ms = EnvironmentManager::GetInstance().GetTime_ms();
  }
}

unsigned long UserEventManager::GetLastKeyPressDiff_ms() {
  std::unique_lock<std::mutex> lock(keyPressedMutex);
  return EnvironmentManager::GetInstance().GetTime_ms() - lastKeyTime_ms;
}

unsigned long UserEventManager::GetLastKeyPressDiff_ms(SDL_Keycode key) {
  std::unique_lock<std::mutex> lock(keyPressedMutex);
  return EnvironmentManager::GetInstance().GetTime_ms() - keyPressed[key].pressTime_ms;
}

bool UserEventManager::GetMouseButtonState(int sdlButtonID) const {
  std::unique_lock<std::mutex> lock(mousePressedMutex);
  return mousePressed[sdlButtonID];
}

Vector3 UserEventManager::GetMouseRelativePos() const {
  Vector3 mousePos;
  mousePos.coords[2] = 0;
  int x, y;
  SDL_GetRelativeMouseState(&x, &y);
  mousePos.coords[0] = x;
  mousePos.coords[1] = y;
  return mousePos;
}

bool UserEventManager::GetJoyButtonState(int joyID, int sdlJoyButtonID) const {
  std::unique_lock<std::mutex> lock(joyButtonPressedMutex);
  return joyButtonPressed[joyID][sdlJoyButtonID];
}

void UserEventManager::SetJoyButtonState(int joyID, int sdlJoyButtonID, bool newState) {
  std::unique_lock<std::mutex> lock(joyButtonPressedMutex);
  joyButtonPressed[joyID][sdlJoyButtonID] = newState;
}

float UserEventManager::GetJoystickAxis(int joyID, int axisID, bool deadzone) const {
  std::unique_lock<std::mutex> lock(joyButtonPressedMutex);

  float min = joyAxisCalibration[joyID][axisID][0];
  float max = joyAxisCalibration[joyID][axisID][1];
  float rest = joyAxisCalibration[joyID][axisID][2];

  float value = joyAxis[joyID][axisID];

  if (value < min)
    value = min;
  if (value > max)
    value = max;
  float scale = max - min;
  if (scale == 0.0)
    scale = 0.01;  // avoid division by zero, axis would be defunct though if scale evaluates to 0

  // bring value in range 0 .. 1
  value -= min;
  value /= scale;

  // bring rest in range 0 .. 1
  rest -= min;
  rest /= scale;

  // deadzone
  if (deadzone)
    if (fabs(rest - value) < 0.1)
      value = rest;

  if (value < rest) {
    // bring value in range 0 .. -1
    value /= rest;
    value -= 1.0;
  } else if (value > rest) {
    // bring value in range 0 .. 1
    scale = 1.0 - rest;
    value -= rest;
    value /= scale;
  } else {  // value == rest
    value = 0.0;
  }

  return value;
}

float UserEventManager::GetJoystickAxisRaw(int joyID, int axisID) const {
  std::unique_lock<std::mutex> lock(joyButtonPressedMutex);
  return joyAxis[joyID][axisID];
}

float UserEventManager::GetJoystickAxisCalibrationMin(int joyID, int axisID) {
  std::unique_lock<std::mutex> lock(joyButtonPressedMutex);
  return joyAxisCalibration[joyID][axisID][0];
}

float UserEventManager::GetJoystickAxisCalibrationMax(int joyID, int axisID) {
  std::unique_lock<std::mutex> lock(joyButtonPressedMutex);
  return joyAxisCalibration[joyID][axisID][1];
}

float UserEventManager::GetJoystickAxisCalibrationRest(int joyID, int axisID) {
  std::unique_lock<std::mutex> lock(joyButtonPressedMutex);
  return joyAxisCalibration[joyID][axisID][2];
}

void UserEventManager::SetJoystickAxisCalibration(int joyID, int axisID, float min, float max,
                                                  float rest) {
  std::unique_lock<std::mutex> lock(joyButtonPressedMutex);
  joyAxisCalibration[joyID][axisID][0] = min;
  joyAxisCalibration[joyID][axisID][1] = max;
  joyAxisCalibration[joyID][axisID][2] = rest;

  // rest has to be within min/max range
  if (joyAxisCalibration[joyID][axisID][2] < joyAxisCalibration[joyID][axisID][0])
    joyAxisCalibration[joyID][axisID][2] = joyAxisCalibration[joyID][axisID][0];
  if (joyAxisCalibration[joyID][axisID][2] > joyAxisCalibration[joyID][axisID][1])
    joyAxisCalibration[joyID][axisID][2] = joyAxisCalibration[joyID][axisID][1];
  joyAxis[joyID][axisID] = rest;
}

}  // namespace blunted
