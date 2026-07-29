// written by bastiaan konings schuiling 2008 - 2015
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not
// be used for anything important. i do not offer support, so don't ask. to be used for inspiration
// :)

#include "controllerselect.hpp"

#include "../main.hpp"
#include "mainmenu.hpp"
#include "pagefactory.hpp"
#include "startmatch/teamselect.hpp"

using namespace blunted;

namespace {

constexpr unsigned long kMenuSmokeAdvanceDelay_ms = 250;

bool MenuSmokeQuickMatchEnabled() {
  return GetConfiguration()->GetBool("menu_smoke_test_quick_match", false);
}

bool MenuSmokeFullMatchEnabled() {
  return GetConfiguration()->GetBool("menu_smoke_test_full_match", false);
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
  Gui2Button* backButton = new Gui2Button(windowManager, name, 38, yPercent, 24, 3, "Back");
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
      side.side = 0;
      if (i == 0 && controllers.size() < 2)
        side.side = -1;  // autoselect 1st player == team 0 (side -1)
      else if (i == 1)
        side.side = -1;  // if more than 1 controller, we're likely to have a gamepad on id > 0, so
                         // pick this one as auto p1 instead
    }
    side.controllerImage =
        new Gui2Image(windowManager, "image_controller" + int_to_str(i), 0, 0, 14, 10);
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
      for (auto& side : sides) {
        side.side = 0;
      }
      SetImagePositions();
      printf("[menu-smoke] Controller select switched to CPU vs CPU for full-match verification\n");
    }

    GetMenuTask()->SetControllerSetup(sides);
    printf("[menu-smoke] Controller select confirmed, opening team selection\n");
    CreatePage(e_PageID_TeamSelect);
  }
}

void ControllerSelectPage::SetImagePositions() {
  for (unsigned int i = 0; i < sides.size(); i++) {
    int x = 43 + sides.at(i).side * 25;
    sides.at(i).controllerImage->SetPosition(x, 20 + i * 15);
  }
}

void ControllerSelectPage::Process() {
  Gui2Page::Process();

  if (!autoAdvanceTriggered && MenuSmokeAutoQuickMatchEnabled() && !inGame &&
      EnvironmentManager::GetInstance().GetTime_ms() >=
          pageCreatedTime_ms + kMenuSmokeAdvanceDelay_ms) {
    autoAdvanceTriggered = true;
    ConfirmSelection();
  }
}

void ControllerSelectPage::ProcessKeyboardEvent(KeyboardEvent* event) {
  const std::vector<IHIDevice*>& controllers = GetControllers();
  if (controllers.empty() || sides.empty()) {
    return;
  }
  IHIDevice* controller = controllers.at(0);
  if (controller->GetDeviceType() != e_HIDeviceType_Keyboard) {
    return;
  }
  HIDKeyboard* keyboard = static_cast<HIDKeyboard*>(controller);
  if (event->GetKeyOnce(keyboard->GetFunctionMapping(e_ButtonFunction_Left))) {
    sides.at(0).side -= 1;
  }
  if (event->GetKeyOnce(keyboard->GetFunctionMapping(e_ButtonFunction_Right))) {
    sides.at(0).side += 1;
  }
  sides.at(0).side = clamp(sides.at(0).side, -1, 1);

  SetImagePositions();
}

void ControllerSelectPage::ProcessJoystickEvent(JoystickEvent*) {
  const std::vector<IHIDevice*>& controllers = GetControllers();
  for (unsigned int i = 1; i < controllers.size() && i < sides.size() && i < delay.size(); i++) {
    if (controllers.at(i)->GetDeviceType() != e_HIDeviceType_Gamepad) {
      continue;
    }
    if (delay.at(i) < EnvironmentManager::GetInstance().GetTime_ms() - 250) {
      HIDGamepad* gamepad = static_cast<HIDGamepad*>(controllers.at(i));
      if (gamepad->GetButtonValue(e_ButtonFunction_Left) > 0.5) {
        sides.at(i).side -= 1;
        delay.at(i) = EnvironmentManager::GetInstance().GetTime_ms();
      }
      if (gamepad->GetButtonValue(e_ButtonFunction_Right) > 0.5) {
        sides.at(i).side += 1;
        delay.at(i) = EnvironmentManager::GetInstance().GetTime_ms();
      }
      sides.at(i).side = clamp(sides.at(i).side, -1, 1);
    }
  }

  SetImagePositions();
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
