#ifndef _HPP_HIDKEYBOARD
#define _HPP_HIDKEYBOARD

#include <SDL2/SDL.h>

#include "base/math/vector3.hpp"
#include "ihidevice.hpp"

using namespace blunted;

class HIDKeyboard : public IHIDevice {
public:
  HIDKeyboard();
  virtual ~HIDKeyboard();

  virtual void LoadConfig();
  virtual void SaveConfig();

  virtual void Process();

  virtual bool GetButton(e_ButtonFunction buttonFunction);
  virtual float GetButtonValue(e_ButtonFunction buttonFunction);  // for analog support
  virtual void SetButton(e_ButtonFunction buttonFunction, bool state);
  virtual bool GetPreviousButtonState(e_ButtonFunction buttonFunction);
  virtual Vector3 GetDirection();

  void SetFunctionMapping(int index, SDL_Keycode key) {
    std::unique_lock<std::mutex> blah(mutex);
    functionMapping[index] = key;
  }

  SDL_Keycode GetFunctionMapping(e_ButtonFunction buttonFunction) {
    std::unique_lock<std::mutex> blah(mutex);
    return functionMapping[buttonFunction];
  }

protected:
  bool functionButtonState[e_ButtonFunction_Size];
  bool previousFunctionButtonState[e_ButtonFunction_Size];

  SDL_Keycode functionMapping[e_ButtonFunction_Size];
};

#endif
