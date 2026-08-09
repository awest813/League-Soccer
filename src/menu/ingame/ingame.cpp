// written by bastiaan konings schuiling 2008 - 2015
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not
// be used for anything important. i do not offer support, so don't ask. to be used for inspiration
// :)

#include "ingame.hpp"

#include <cstdio>

#include "../controllerselect.hpp"
#include "../gameplan.hpp"
#include "../pagefactory.hpp"
#include "../settings.hpp"
#include "main.hpp"
#include "replaymenu.hpp"
#include "utils/localization.hpp"

using namespace blunted;

IngamePage::IngamePage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  teamID = pageData.properties->GetInt("teamID", 0);

  GetGameTask()->GetMatch()->Pause(true);

  Match* match = GetGameTask()->GetMatch();
  int score0 = match->GetScore(0);
  int score1 = match->GetScore(1);
  std::string team0Name = match->GetTeam(0)->GetTeamData()->GetName();
  std::string team1Name = match->GetTeam(1)->GetTeamData()->GetName();

  unsigned long matchTime_ms = match->GetMatchTime_ms();
  int matchMinute = static_cast<int>(matchTime_ms / 60000);
  if (matchMinute > 90)
    matchMinute = 90;

  char scoreBuf[256];
  snprintf(scoreBuf, sizeof(scoreBuf), "%s  %d - %d  %s  (%d')", team0Name.c_str(), score0, score1,
           team1Name.c_str(), matchMinute);

  Gui2Frame* frame = new Gui2Frame(windowManager, "frame_ingame", 20, 8, 60, 84, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_ingame_title", 2, 2, 56, 3,
                                       Localization::GetInstance().Translate("ingame_pause"));
  frame->AddView(title);
  title->Show();

  Gui2Caption* scoreLine =
      new Gui2Caption(windowManager, "caption_ingame_score", 2, 6, 56, 3, scoreBuf);
  frame->AddView(scoreLine);
  scoreLine->Show();

  // All selectable buttons live in ONE grid so arrow-key navigation flows
  // continuously across every section (the previous layout used four separate
  // grids, which trapped focus inside a single section).
  Gui2Grid* grid = new Gui2Grid(windowManager, "grid_ingame", 2, 12, 56, 80);
  int row = 0;

  Gui2Caption* tacticsLabel =
      new Gui2Caption(windowManager, "caption_ingame_section_tactics", 0, 0, 56, 2,
                      Localization::GetInstance().Translate("ingame_section_tactics"));
  grid->AddView(tacticsLabel, row++, 0);

  Gui2Button* buttonGamePlan =
      new Gui2Button(windowManager, "button_gameplan", 0, 0, 56, 3,
                     Localization::GetInstance().Translate("ingame_game_plan"));
  Gui2Button* buttonSetPieces =
      new Gui2Button(windowManager, "button_setpieces", 0, 0, 56, 3,
                     Localization::GetInstance().Translate("ingame_set_pieces"));
  buttonGamePlan->sig_OnClick.connect([this](...) { GoGamePlan(); });
  buttonSetPieces->sig_OnClick.connect([this](...) { GoSetPieceEditor(); });
  grid->AddView(buttonGamePlan, row++, 0);
  grid->AddView(buttonSetPieces, row++, 0);

  Gui2Caption* settingsLabel =
      new Gui2Caption(windowManager, "caption_ingame_section_settings", 0, 0, 56, 2,
                      Localization::GetInstance().Translate("ingame_section_settings"));
  grid->AddView(settingsLabel, row++, 0);

  Gui2Button* buttonControllerSelect =
      new Gui2Button(windowManager, "button_controllerselect", 0, 0, 56, 3,
                     Localization::GetInstance().Translate("ingame_controller_select"));
  Gui2Button* buttonCameraSettings =
      new Gui2Button(windowManager, "button_camerasettings", 0, 0, 56, 3,
                     Localization::GetInstance().Translate("ingame_camera_settings"));
  Gui2Button* buttonVisualOptions =
      new Gui2Button(windowManager, "button_visualoptions", 0, 0, 56, 3,
                     Localization::GetInstance().Translate("ingame_visual_options"));
  Gui2Button* buttonSystemSettings =
      new Gui2Button(windowManager, "button_systemsettings", 0, 0, 56, 3,
                     Localization::GetInstance().Translate("ingame_system_settings"));
  buttonControllerSelect->sig_OnClick.connect([this](...) { GoControllerSelect(); });
  buttonCameraSettings->sig_OnClick.connect([this](...) { GoCameraSettings(); });
  buttonVisualOptions->sig_OnClick.connect([this](...) { GoVisualOptions(); });
  buttonSystemSettings->sig_OnClick.connect([this](...) { GoSystemSettings(); });
  grid->AddView(buttonControllerSelect, row++, 0);
  grid->AddView(buttonCameraSettings, row++, 0);
  grid->AddView(buttonVisualOptions, row++, 0);
  grid->AddView(buttonSystemSettings, row++, 0);

  Gui2Caption* mediaLabel =
      new Gui2Caption(windowManager, "caption_ingame_section_media", 0, 0, 56, 2,
                      Localization::GetInstance().Translate("ingame_section_media"));
  grid->AddView(mediaLabel, row++, 0);

  Gui2Button* buttonReplay = new Gui2Button(windowManager, "button_replay", 0, 0, 56, 3,
                                            Localization::GetInstance().Translate("ingame_replay"));
  buttonReplay->sig_OnClick.connect([this](...) { GoReplay(); });
  grid->AddView(buttonReplay, row++, 0);

  Gui2Caption* exitLabel =
      new Gui2Caption(windowManager, "caption_ingame_section_exit", 0, 0, 56, 2,
                      Localization::GetInstance().Translate("ingame_section_match"));
  grid->AddView(exitLabel, row++, 0);

  Gui2Button* buttonResume =
      new Gui2Button(windowManager, "button_resume", 0, 0, 56, 3,
                     Localization::GetInstance().Translate("ingame_resume_match"));
  Gui2Button* buttonPreQuit =
      new Gui2Button(windowManager, "button_quit", 0, 0, 56, 3,
                     Localization::GetInstance().Translate("ingame_forfeit_match"));
  buttonResume->sig_OnClick.connect([this](...) {
    GetMenuTask()->ReleaseAllButtons();
    GetGameTask()->GetMatch()->Pause(false);
    GoBack();  // reconstructs the GamePage, which restores GUI focus
  });
  buttonPreQuit->sig_OnClick.connect([this](...) { GoPreQuit(); });
  grid->AddView(buttonResume, row++, 0);
  grid->AddView(buttonPreQuit, row++, 0);

  grid->UpdateLayout(0.5);
  frame->AddView(grid);
  grid->Show();

  Gui2Caption* hintCaption = new Gui2Caption(windowManager, "caption_ingame_hint", 2, 79, 56, 2,
                                             Localization::GetInstance().Translate("ingame_hint"));
  frame->AddView(hintCaption);
  hintCaption->Show();

  buttonResume->SetFocus();

  this->Show();
}

IngamePage::~IngamePage() {}

void IngamePage::GoControllerRemap() {
  CreatePage(e_PageID_Controller);
}

void IngamePage::GoGamePlan() {
  Properties properties;
  properties.Set("teamID", teamID);
  CreatePage(e_PageID_GamePlan, properties);
}

void IngamePage::GoControllerSelect() {
  Properties properties;
  properties.SetBool("isInGame", true);
  CreatePage(e_PageID_ControllerSelect, properties);
}

void IngamePage::GoCameraSettings() {
  CreatePage(e_PageID_Camera);
}

void IngamePage::GoVisualOptions() {
  CreatePage(e_PageID_VisualOptions);
}

void IngamePage::GoSystemSettings() {
  CreatePage(e_PageID_Settings);
}

void IngamePage::GoReplay() {
  CreatePage(e_PageID_Replay);
}

void IngamePage::GoPreQuit() {
  CreatePage(e_PageID_PreQuit);
}

void IngamePage::GoSetPieceEditor() {
  Properties properties;
  properties.Set("teamDatabaseID",
                 GetGameTask()->GetMatch()->GetTeam(teamID)->GetTeamData()->GetDatabaseID());
  CreatePage((int)e_PageID_SetPieceEditor, properties);
}

void IngamePage::ProcessWindowingEvent(WindowingEvent* event) {
  if (event->IsEscape()) {
    GetMenuTask()->ReleaseAllButtons();
    GetGameTask()->GetMatch()->Pause(false);
  }
  Gui2Page::ProcessWindowingEvent(event);
}

PreQuitPage::PreQuitPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* frame = new Gui2Frame(windowManager, "frame_prequit", 25, 40, 50, 20, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* restartCaption =
      new Gui2Caption(windowManager, "caption_prequit_info", 0, 0, 44, 3,
                      Localization::GetInstance().Translate("ingame_forfeit_confirm"));
  Gui2Button* okButton = new Gui2Button(windowManager, "button_prequit_ok", 0, 0, 44, 3,
                                        Localization::GetInstance().Translate("ingame_forfeit"));
  Gui2Button* cancelButton =
      new Gui2Button(windowManager, "button_prequit_cancel", 0, 0, 44, 3,
                     Localization::GetInstance().Translate("ingame_continue_match"));
  okButton->sig_OnClick.connect([this](...) { GoMenu(); });
  cancelButton->sig_OnClick.connect([this](...) { GoBack(); });

  Gui2Grid* grid = new Gui2Grid(windowManager, "grid_prequit", 2, 2, 46, 16);

  grid->AddView(restartCaption, 0, 0);
  grid->AddView(okButton, 1, 0);
  grid->AddView(cancelButton, 2, 0);

  grid->UpdateLayout(0.5);

  frame->AddView(grid);
  grid->Show();

  cancelButton->SetFocus();

  this->Show();
}

PreQuitPage::~PreQuitPage() {}

void PreQuitPage::GoMenu() {
  this->Exit();
  GetMenuTask()->SetMenuAction(e_MenuAction_Menu);
  delete this;
}
