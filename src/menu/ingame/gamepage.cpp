// written by bastiaan konings schuiling 2008 - 2015
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not
// be used for anything important. i do not offer support, so don't ask. to be used for inspiration
// :)

#include "gamepage.hpp"

#include "../../onthepitch/match.hpp"
#include "../pagefactory.hpp"
#include "gameover.hpp"
#include "main.hpp"
#include "phasemenu.hpp"
#include "replaymenu.hpp"

using namespace blunted;

namespace {

constexpr unsigned long kMenuSmokeQuitDelay_ms = 2000;

bool MenuSmokeQuickMatchEnabled() {
  return GetConfiguration()->GetBool("menu_smoke_test_quick_match", false);
}

bool MenuSmokeFullMatchEnabled() {
  return GetConfiguration()->GetBool("menu_smoke_test_full_match", false);
}

}  // namespace

GamePage::GamePage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData), match(0), matchReadyTime_ms(0), autoQuitTriggered(false) {
  Gui2Caption* betaSign =
      new Gui2Caption(windowManager, "caption_betasign", 0, 0, 0, 2, "League-Soccer v0.4.0");
  betaSign->SetColor(Vector3(180, 180, 180));
  betaSign->SetTransparency(0.3f);
  this->AddView(betaSign);
  float w = betaSign->GetTextWidthPercent();
  betaSign->SetPosition(50 - w * 0.5f, 97.0f);
  betaSign->Show();

  this->Show();

  this->SetFocus();
}

GamePage::~GamePage() {
  // todonow: only when connected in the first place?
  // problem is, this function may be called outside of gametask's or match's lifetime.

  if (match) {
    if (Verbose())
      printf("disconnecting signals\n");

    conn_MatchPhaseChange.disconnect();
    conn_ShortReplayMoment.disconnect();
    conn_ExtendedReplayMoment.disconnect();
    conn_GameOver.disconnect();
  }
}

void GamePage::Process() {
  Gui2Page::Process();

  if (!match) {
    GetGameTask()->matchLifetimeMutex.lock();
    if (GetGameTask()->GetMatch() != 0) {
      match = GetGameTask()->GetMatch();

      if (Verbose())
        printf("connecting signals\n");

      conn_MatchPhaseChange =
          match->sig_OnMatchPhaseChange.connect([this](...) { GoMatchPhasePage(); });
      conn_ShortReplayMoment =
          match->sig_OnShortReplayMoment.connect([this](...) { GoShortReplayPage(); });
      conn_ExtendedReplayMoment =
          match->sig_OnExtendedReplayMoment.connect([this](...) { GoExtendedReplayPage(); });
      conn_GameOver = match->sig_OnGameOver.connect([this](...) { GoGameOverPage(); });

      if (MenuSmokeQuickMatchEnabled() || MenuSmokeFullMatchEnabled()) {
        matchReadyTime_ms = EnvironmentManager::GetInstance().GetTime_ms();
        printf("[menu-smoke] Gameplay page reached and live match is active\n");
      }
    }
    GetGameTask()->matchLifetimeMutex.unlock();
  }

  if (match && !autoQuitTriggered && MenuSmokeQuickMatchEnabled() && !MenuSmokeFullMatchEnabled() &&
      matchReadyTime_ms != 0 &&
      EnvironmentManager::GetInstance().GetTime_ms() >=
          matchReadyTime_ms + kMenuSmokeQuitDelay_ms) {
    autoQuitTriggered = true;
    printf("[menu-smoke] Quick Match verification succeeded, quitting test run\n");
    EnvironmentManager::GetInstance().SignalQuit();
  }
}

void GamePage::GoShortReplayPage() {
  CreatePage((int)e_PageID_Replay);
}

void GamePage::GoExtendedReplayPage() {
  this->Exit();

  Properties properties;
  ReplayPage* replayPage = static_cast<ReplayPage*>(
      windowManager->GetPageFactory()->CreatePage((int)e_PageID_Replay, properties, 0));

  // todo: use properties instead?
  int replayHistoryOffset_ms = match->GetReplaySize_ms();
  bool stayInReplay = true;
  replayPage->Autorun(replayHistoryOffset_ms, stayInReplay);

  delete this;
}

void GamePage::GoMatchPhasePage() {
  e_MatchPhase nextPhase = match->GetMatchPhase();

  Properties properties;
  properties.Set("nextphase", (int)nextPhase);
  CreatePage((int)e_PageID_MatchPhase, properties);
}

void GamePage::GoGameOverPage() {
  CreatePage((int)e_PageID_GameOver);
}

void GamePage::OnCreatedMatch() {}

void GamePage::ProcessWindowingEvent(WindowingEvent* event) {
  if (Verbose())
    if (event->IsEscape())
      printf("escape!\n");
  event->Ignore();
}

void GamePage::ProcessKeyboardEvent(KeyboardEvent* event) {
  if (event->GetKeyOnce(SDLK_TAB)) {
    if (match)
      match->ToggleStatsOverlay();
    return;
  }

  if (event->GetKeyOnce(SDLK_ESCAPE)) {
    // check which team the keyboard belongs to
    int controllerID = 0;
    const std::vector<IHIDevice*>& controllers = GetControllers();
    for (unsigned int c = 0; c < controllers.size(); c++) {
      if (controllers.at(c)->GetDeviceType() == e_HIDeviceType_Keyboard) {
        controllerID = c;
        break;
      }
    }

    if (Verbose())
      printf("controller index %i (keyboard) pressed start\n", controllerID);

    int teamID = 0;
    const std::vector<SideSelection> sides = GetMenuTask()->GetControllerSetup();
    for (unsigned int s = 0; s < sides.size(); s++) {
      if (sides.at(s).controllerID == (signed int)controllerID) {
        teamID = int(round(sides.at(s).side * 0.5 + 0.5));
        break;
      }
    }

    if (Verbose())
      printf("team belonging to this controller seems to be %i\n", teamID);

    Properties properties;
    properties.Set("teamID", teamID);
    CreatePage((int)e_PageID_Ingame, properties);
    return;
  }

  event->Ignore();
}

void GamePage::ProcessJoystickEvent(JoystickEvent* event) {
  // oof, the problem is that we are using a GUI joystick event to find out what ingame HID
  // controller pressed <start>. I think we need a new system in which the game doesn't use its own
  // input system, but uses the GUI one. for now, this hax will have to do

  const std::vector<IHIDevice*>& controllers = GetControllers();
  for (unsigned int c = 0; c < controllers.size(); c++) {
    if (controllers.at(c)->GetDeviceType() == e_HIDeviceType_Gamepad) {
      HIDGamepad* gamepad = static_cast<HIDGamepad*>(controllers.at(c));
      int joyID =
          gamepad->GetGamepadID();  // these should be the same IDs the GUI system uses as joyID

      if (event->GetButton(joyID, gamepad->GetControllerMapping(
                                      gamepad->GetFunctionMapping(e_ButtonFunction_Start)))) {
        if (Verbose())
          printf("controller index %i, gamepad/joy ID %i pressed start\n", c, joyID);

        // check which team this controller belongs to
        int teamID = 0;
        const std::vector<SideSelection> sides = GetMenuTask()->GetControllerSetup();
        for (unsigned int s = 0; s < sides.size(); s++) {
          if (sides.at(s).controllerID == (signed int)c) {
            teamID = int(round(sides.at(s).side * 0.5 + 0.5));
            break;
          }
        }

        if (Verbose())
          printf("team belonging to this controller seems to be %i\n", teamID);

        Properties properties;
        properties.Set("teamID", teamID);
        CreatePage((int)e_PageID_Ingame, properties);
        return;
      }
    }
  }

  event->Ignore();
}
