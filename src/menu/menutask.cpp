// written by bastiaan konings schuiling 2008 - 2015
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not
// be used for anything important. i do not offer support, so don't ask. to be used for inspiration
// :)

#include "menutask.hpp"

#include "../onthepitch/match.hpp"
#include "career/career_database.hpp"
#include "data/careerdata.hpp"
#include "framework/scheduler.hpp"
#include "gametask.hpp"
#include "ingame/gameover.hpp"
#include "ingame/ingame.hpp"
#include "ingame/phasemenu.hpp"
#include "ingame/replaymenu.hpp"
#include "main.hpp"
#include "mainmenu.hpp"
#include "managers/resourcemanagerpool.hpp"
#include "pagefactory.hpp"
#include "visualoptions.hpp"

using namespace blunted;

void SetActiveController(int side, bool keyboard) {
  bool keyboardActive = true;
  const std::vector<SideSelection> sides = GetMenuTask()->GetControllerSetup();
  const std::vector<IHIDevice*>& controllers = GetControllers();
  int menuControllerID = -1;
  for (unsigned int i = 0; i < sides.size(); i++) {
    if (sides.at(i).side == side) {
      int controllerID = sides.at(i).controllerID;
      if (controllerID >= 0 && controllerID < static_cast<int>(controllers.size()) &&
          controllers.at(controllerID)->GetDeviceType() == e_HIDeviceType_Gamepad) {
        menuControllerID = static_cast<HIDGamepad*>(controllers.at(controllerID))->GetGamepadID();
        keyboardActive = false;
      }
      break;
    }
    if (i == sides.size() - 1)
      menuControllerID = 0;  // AI opponent, so allow choosing their team with controller
  }

  GetMenuTask()->SetActiveJoystickID(menuControllerID);
  if (keyboard) {
    if (keyboardActive) {
      GetMenuTask()->EnableKeyboard();
    } else {
      GetMenuTask()->DisableKeyboard();
    }
  } else {
    GetMenuTask()->EnableKeyboard();
  }
}

MenuTask::MenuTask(float aspectRatio, float margin, TTF_Font* defaultFont,
                   TTF_Font* defaultOutlineFont)
    : Gui2Task(GetScene2D(), aspectRatio, margin), menuBackground(nullptr) {
  Gui2Style* style = windowManager->GetStyle();

  style->SetFont(e_TextType_Default, defaultFont);
  style->SetFont(e_TextType_DefaultOutline, defaultOutlineFont);
  style->SetFont(e_TextType_Caption, defaultFont);
  style->SetFont(e_TextType_Title, defaultFont);
  style->SetFont(e_TextType_ToolTip, defaultFont);

  // Ultra-modern minimalist dark theme with vibrant neon accents
  style->SetColor(e_DecorationType_Dark1, Vector3(10, 10, 12));  // almost pure black (backgrounds)
  style->SetColor(e_DecorationType_Dark2, Vector3(28, 28, 32));  // charcoal gray (borders/inactive)
  style->SetColor(e_DecorationType_Bright1, Vector3(250, 250, 255));  // crisp cool white (text)
  style->SetColor(e_DecorationType_Bright2, Vector3(0, 220, 255));    // electric cyan (hover/focus)
  style->SetColor(e_DecorationType_Toggled,
                  Vector3(255, 0, 100));  // vivid magenta (active/toggled)

  windowManager->SetTimeStep_ms(10);

  Gui2Root* root = windowManager->GetRoot();
  root->Show();

  menuBackground =
      new Gui2Image(windowManager, "image_menu_background", 0, 0, 100, 100);
  menuBackground->LoadImage("media/menu/backgrounds/stadium01.png");
  root->AddView(menuBackground);
  menuBackground->Show();

  PageFactory* pageFactory = new PageFactory();
  windowManager->SetPageFactory(pageFactory);

  if (!QuickStart()) {
    queuedFixture->team1KitNum = 1;
    queuedFixture->team2KitNum = 2;

    menuAction = e_MenuAction_Menu;

  } else {
    int size = GetControllers().size();
    for (int i = 0; i < size; i++) {
      SideSelection side;
      side.controllerID = i;
      if ((size > 1 && i == 1) || (size == 1 && i == 0)) {
        side.side = -1;
      } else {
        side.side = 0;
      }
      queuedFixture->sides.push_back(side);
    }

    // 1 == ajax
    // 2 == arsenal
    // 3 == barcelona
    // 4 == bayern
    // 5 == borussia
    // 6 == man utd
    // 7 == psv
    // 8 == real madrid
    queuedFixture->teamID1 = "3";
    queuedFixture->teamID2 = "8";
    queuedFixture->team1KitNum = 2;
    queuedFixture->team2KitNum = 2;

    menuAction = e_MenuAction_Menu;
  }
}

MenuTask::~MenuTask() {
  if (Verbose())
    printf("exiting menutask.. ");

  delete windowManager->GetPageFactory();

  if (Verbose())
    printf("done\n");
}

void MenuTask::ProcessPhase() {
  Gui2Task::ProcessPhase();

  if (menuAction == e_MenuAction_Menu) {
    windowManager->GetPagePath()->Clear();

    GetGameTask()->Action(e_GameTaskMessage_StopMatch);
    GetGameTask()->Action(e_GameTaskMessage_StopMenuScene);

    menuBackground->Show();
    Properties properties;
    if (GetConfiguration()->GetBool("career_resume_hub", false)) {
      GetConfiguration()->SetBool("career_resume_hub", false);
      CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
      const int hubPage = (save && save->mode == CareerMode::OWNER) ? (int)e_PageID_OwnerHub
                                                                    : (int)e_PageID_CareerHub;
      windowManager->GetPageFactory()->CreatePage(hubPage, properties, 0);
    } else if (!QuickStart()) {
      if (!IsReleaseVersion()) {
        windowManager->GetPageFactory()->CreatePage((int)e_PageID_MainMenu, properties, 0);
      } else {
        windowManager->GetPageFactory()->CreatePage((int)e_PageID_Intro, properties, 0);
      }
    } else {
      windowManager->GetPageFactory()->CreatePage((int)e_PageID_LoadingMatch, properties, 0);
    }

  } else if (menuAction == e_MenuAction_Game) {
    menuBackground->Hide();
    GetGameTask()->Action(e_GameTaskMessage_StopMenuScene);
    GetGameTask()->Action(e_GameTaskMessage_StartMatch);
  }

  menuAction = e_MenuAction_None;
}

bool MenuTask::QuickStart() {
  // Keep the normal main-menu flow available in debug builds unless quick-start is explicitly
  // enabled in the config for local iteration.
  return !IsReleaseVersion() && GetConfiguration()->GetBool("quick_start", false) &&
         EnvironmentManager::GetInstance().GetTime_ms() <
             10000;  // after 10 seconds, quickstart disabled
}

void MenuTask::QuitGame() {
  EnvironmentManager::GetInstance().SignalQuit();
}

void MenuTask::ReleaseAllButtons() {
  // when going back to game, depress all buttons, so we don't go around doing passes we don't want
  for (int joyID = 0; joyID < UserEventManager::GetInstance().GetJoystickCount(); joyID++) {
    for (unsigned int buttonID = 0; buttonID < blunted::_JOYSTICK_MAXBUTTONS; buttonID++) {
      UserEventManager::GetInstance().SetJoyButtonState(joyID, buttonID, false);
    }
  }
  UserEventManager::GetInstance().SetKeyboardState(SDLK_ESCAPE, false);
}
