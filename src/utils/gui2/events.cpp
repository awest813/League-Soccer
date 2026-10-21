#include "events.hpp"

namespace blunted {

Gui2Event::Gui2Event(e_Gui2EventType eventType) : eventType(eventType), accepted(false) {}

Gui2Event::~Gui2Event() {}

e_Gui2EventType Gui2Event::GetType() const {
  return eventType;
}

// WINDOWING EVENT

WindowingEvent::WindowingEvent() : Gui2Event(e_Gui2EventType_Windowing) {
  activate = false;
  escape = false;
}

WindowingEvent::~WindowingEvent() {}

// KEYBOARD EVENT

KeyboardEvent::KeyboardEvent() : Gui2Event(e_Gui2EventType_Keyboard) {}

KeyboardEvent::~KeyboardEvent() {}

// JOYSTICK EVENT

JoystickEvent::JoystickEvent() : Gui2Event(e_Gui2EventType_Joystick) {
  // Clear every slot, not just connected pads: handlers (e.g. button capture)
  // read axes for a specific pad from events raised by another one.
  for (int j = 0; j < _JOYSTICK_MAX; j++) {
    for (int i = 0; i < _JOYSTICK_MAXBUTTONS; i++)
      button[j][i] = false;
    for (int i = 0; i < _JOYSTICK_MAXAXES; i++)
      axes[j][i] = 0.0f;
  }
}

JoystickEvent::~JoystickEvent() {}

}  // namespace blunted
