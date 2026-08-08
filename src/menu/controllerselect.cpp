// written by bastiaan konings schuiling 2008 - 2015
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not
// be used for anything important. i do not offer support, so don't ask. to be used for inspiration
// :)

#include "controllerselect.hpp"

#include "../main.hpp"
#include "hid/gamepad.hpp"
#include "hid/keyboard.hpp"
#include "mainmenu.hpp"
#include "managers/usereventmanager.hpp"
#include "pagefactory.hpp"
#include "startmatch/teamselect.hpp"
#include "utils/localization.hpp"

using namespace blunted;

namespace {

constexpr float kControllerThumbnailAspectRatio = 300.0f / 200.0f;
constexpr float kControllerThumbnailHeight = 10.0f;

constexpr unsigned long kMenuSmokeAdvanceDelay_ms = 250;

bool MenuSmokeQuickMatchEnabled() {
  return GetConfiguration()->GetBool("menu_smoke_test_quick_match", false);
}

bool MenuSmokeFullMatchEnabled() {
  return GetConfiguration()->GetBool("menu_smoke_test_full_match", false);
}

bool MenuSmokeGamepadMatchEnabled() {
  return GetConfiguration()->GetBool("menu_smoke_test_gamepad_match", false);
}

bool MenuSmokeAutoQuickMatchEnabled() {
  return MenuSmokeQuickMatchEnabled() || MenuSmokeFullMatchEnabled();
}

Gui2Caption* AddControllerSelectNotice(Gui2Page* page, Gui2WindowManager* windowManager,
                                       const std::string& name, float yPercent,
                                       const std::string& caption) {
  Gui2Caption* notice = new Gui2Caption(windowManager, name, 10, yPercent, 80, 3, caption);
  page->AddView(notice);
  notice->SetPosition(50 - notice->GetTextWidthPercent() * 0.5, yPercent);
  notice->Show();
  return notice;
}

Gui2Button* AddControllerSelectBackButton(Gui2Page* page, Gui2WindowManager* windowManager,
                                          const std::string& name, float yPercent = 72.0f) {
  Gui2Button* backButton = new Gui2Button(windowManager, name, 38, yPercent, 24, 3,
                                          Localization::GetInstance().Translate("action_back"));
  backButton->sig_OnClick.connect([page](...) { page->GoBack(); });
  page->AddView(backButton);
  backButton->Show();
  return backButton;
}

}  // namespace

ControllerSelectPage::ControllerSelectPage(Gui2WindowManager* windowManager,
                                           const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData),
      pageCreatedTime_ms(EnvironmentManager::GetInstance().GetTime_ms()),
      autoAdvanceTriggered(false) {
  inGame = pageData.properties->GetBool("isInGame");

  // Elegant semi-transparent background frame
  Gui2Frame* ctrlFrame =
      new Gui2Frame(windowManager, "frame_controllerselect", 10, 5, 80, 85, true);
  this->AddView(ctrlFrame);
  ctrlFrame->Show();

  Gui2Caption* t1 =
      new Gui2Caption(windowManager, "caption_controllerselect_t1", 0, 0, 28, 3, "Team 1");
  Gui2Caption* t2 =
      new Gui2Caption(windowManager, "caption_controllerselect_t2", 0, 0, 28, 3, "Team 2");

  t1->SetPosition(25 - t1->GetTextWidthPercent() * 0.5, 10);
  t2->SetPosition(75 - t2->GetTextWidthPercent() * 0.5, 10);

  ctrlFrame->AddView(t1);
  t1->Show();
  ctrlFrame->AddView(t2);
  t2->Show();

  this->SetFocus();

  const std::vector<IHIDevice*>& controllers = GetControllers();
  if (controllers.empty()) {
    AddControllerSelectNotice(this, windowManager, "caption_controllerselect_noinput", 44,
                              "No input devices detected.");
    AddControllerSelectNotice(this, windowManager, "caption_controllerselect_noinput_hint", 49,
                              "Connect a keyboard or gamepad before starting a match.");
    Gui2Button* backButton =
        AddControllerSelectBackButton(this, windowManager, "button_controllerselect_back");
    backButton->SetFocus();
    this->Show();
    return;
  }

  if (inGame) {
    sides = GetMenuTask()->GetControllerSetup();
  }
  for (unsigned int i = 0; i < controllers.size(); i++) {
    SideSelection side;
    side.controllerID = i;
    if (inGame && i < sides.size()) {
      side.side = sides.at(i).side;
    } else {
      // Always start on No Team (side 0): no device should auto-participate.
      // Only the first gamepad is auto-selected as Player 1; the keyboard never
      // is, so it is not forced into every match (think gamepad-only testing).
      side.side = 0;
      if (!autoAssignedPlayerOne && controllers.at(i)->GetDeviceType() == e_HIDeviceType_Gamepad) {
        side.side = -1;
        autoAssignedPlayerOne = true;
      }
    }
    const float controllerImageWidth = windowManager->GetWidthPercentForHeight(
        kControllerThumbnailHeight, kControllerThumbnailAspectRatio);
    side.controllerImage = new Gui2Image(windowManager, "image_controller" + int_to_str(i), 0, 0,
                                         controllerImageWidth, kControllerThumbnailHeight);
    this->AddView(side.controllerImage);
    if (controllers.at(i)->GetDeviceType() == e_HIDeviceType_Gamepad) {
      side.controllerImage->LoadImage("media/menu/controller/controller_small.png");
    } else {
      side.controllerImage->LoadImage("media/menu/controller/keyboard_small.png");
    }
    side.controllerImage->Show();
    if (!inGame || i >= sides.size())
      sides.push_back(side);
    else
      sides.at(i) = side;
    delay.push_back(0);
  }

  if (sides.size() > controllers.size()) {
    sides.resize(controllers.size());
  }

  SetImagePositions();

  Gui2Button* backButton =
      AddControllerSelectBackButton(this, windowManager, "button_controllerselect_back", 88.0f);
  if (!MenuSmokeAutoQuickMatchEnabled()) {
    // Keep focus on the page for side selection; Back remains clickable.
    (void)backButton;
  }

  this->Show();
}

ControllerSelectPage::~ControllerSelectPage() {}

void ControllerSelectPage::ConfirmSelection() {
  if (!inGame) {
    if (sides.empty()) {
      printf("[menu-smoke] Controller select has no available input devices\n");
      return;
    }

    if (MenuSmokeFullMatchEnabled()) {
      if (MenuSmokeGamepadMatchEnabled()) {
        // "gamepad controls" smoke: the scripted controller is Player 1, the
        // other team is CPU. This drives the real HumanController path from a
        // scripted HID device for the whole match.
        const std::vector<IHIDevice*>& controllers = GetControllers();
        for (auto& side : sides) {
          bool scripted = side.controllerID >= 0 &&
                          side.controllerID < static_cast<int>(controllers.size()) &&
                          controllers.at(side.controllerID)->IsScriptedInput();
          side.side = scripted ? -1 : 0;
        }
        SetImagePositions();
        printf(
            "[menu-smoke] Controller select: scripted gamepad drives Player 1, other team CPU\n");
      } else {
        for (auto& side : sides) {
          side.side = 0;
        }
        SetImagePositions();
        printf(
            "[menu-smoke] Controller select switched to CPU vs CPU for full-match verification\n");
      }
    }

    GetMenuTask()->SetControllerSetup(sides);
    printf("[menu-smoke] Controller select confirmed, opening team selection\n");
    CreatePage(e_PageID_TeamSelect);
  }
}

void ControllerSelectPage::SetImagePositions() {
  for (unsigned int i = 0; i < sides.size(); i++) {
    const float controllerImageWidth = windowManager->GetWidthPercentForHeight(
        kControllerThumbnailHeight, kControllerThumbnailAspectRatio);
    const float centerX = 50.0f + sides.at(i).side * 25.0f;
    sides.at(i).controllerImage->SetPosition(centerX - controllerImageWidth * 0.5f, 20 + i * 15);
  }
}

void ControllerSelectPage::Process() {
  Gui2Page::Process();

  if (!autoAdvanceTriggered && MenuSmokeAutoQuickMatchEnabled() && !inGame &&
      EnvironmentManager::GetInstance().GetTime_ms() >=
          pageCreatedTime_ms + kMenuSmokeAdvanceDelay_ms) {
    autoAdvanceTriggered = true;
    ConfirmSelection();
    // ConfirmSelection() transitions to the next page via CreatePage(), which
    // does `delete this` (see Gui2Page::CreatePage). We must not touch any
    // member of this page after that point, so stop here instead of falling
    // through to the input-polling loop below — doing so was a use-after-free
    // that deterministically segfaulted the headless smoke tests on Linux.
    return;
  }

  // Move the side selection by polling each device's HID state directly here in
  // Process(). This is deliberate: input must not depend on GUI focus (the
  // keyboard otherwise stays stuck on its default side when focus is held by
  // another widget), and it must not depend on SDL event delivery (a still
  // gamepad in a deadzone generates no joystick events). Polling every device
  // each frame puts the keyboard and every gamepad on equal, focus-independent
  // footing.
  const std::vector<IHIDevice*>& controllers = GetControllers();
  if (controllers.empty() || sides.empty() || delay.empty()) {
    return;
  }
  unsigned long now_ms = EnvironmentManager::GetInstance().GetTime_ms();
  for (unsigned int i = 0; i < controllers.size() && i < sides.size() && i < delay.size(); i++) {
    if (delay.at(i) >= now_ms - 250) {
      continue;
    }

    IHIDevice* controller = controllers.at(i);
    bool moved = false;

    if (controller->GetDeviceType() == e_HIDeviceType_Keyboard) {
      HIDKeyboard* keyboard = static_cast<HIDKeyboard*>(controller);
      if (keyboard->GetButtonValue(e_ButtonFunction_Left) > 0.5) {
        sides.at(i).side -= 1;
        moved = true;
      }
      if (keyboard->GetButtonValue(e_ButtonFunction_Right) > 0.5) {
        sides.at(i).side += 1;
        moved = true;
      }
      // One-keystroke hotkeys: jump straight to a team instead of tapping arrows.
      // "1" = Team 1, "2" = Team 2. Most useful for keyboard-only players who now
      // start on No Team by default.
      if (UserEventManager::GetInstance().GetKeyboardState(SDLK_1)) {
        sides.at(i).side = -1;
        moved = true;
      } else if (UserEventManager::GetInstance().GetKeyboardState(SDLK_2)) {
        sides.at(i).side = 1;
        moved = true;
      }
    } else if (controller->GetDeviceType() == e_HIDeviceType_Gamepad) {
      HIDGamepad* gamepad = static_cast<HIDGamepad*>(controller);
      if (gamepad->GetButtonValue(e_ButtonFunction_Left) > 0.5) {
        sides.at(i).side -= 1;
        moved = true;
      }
      if (gamepad->GetButtonValue(e_ButtonFunction_Right) > 0.5) {
        sides.at(i).side += 1;
        moved = true;
      }
    }

    if (moved) {
      sides.at(i).side = clamp(sides.at(i).side, -1, 1);
      delay.at(i) = now_ms;
    }
  }

  SetImagePositions();
}

void ControllerSelectPage::ProcessKeyboardEvent(KeyboardEvent* event) {
  // Keyboard side selection is handled by polling the HID state in Process() so
  // that it works regardless of GUI focus. Nothing to do with the event itself.
  (void)event;
}

void ControllerSelectPage::ProcessJoystickEvent(JoystickEvent* event) {
  // Gamepad side selection is also handled by polling in Process(), exactly
  // like the keyboard, so it does not rely on joystick events being delivered.
  (void)event;
}

void ControllerSelectPage::ProcessWindowingEvent(WindowingEvent* event) {
  if (event->IsActivate()) {
    if (!inGame) {
      if (sides.empty()) {
        event->Ignore();
        return;
      }
      GetMenuTask()->SetControllerSetup(sides);

      CreatePage(e_PageID_TeamSelect);
      return;
    }
    Gui2Page::ProcessWindowingEvent(event);
  }
  if (event->IsEscape()) {
    if (inGame) {
      GetMenuTask()->SetControllerSetup(sides);
      GetGameTask()->GetMatch()->UpdateControllerSetup();
    }
    Gui2Page::ProcessWindowingEvent(event);
  }
}
