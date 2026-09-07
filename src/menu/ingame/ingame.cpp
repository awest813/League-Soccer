#include "ingame.hpp"

#include <cstdio>

#include "../controllerselect.hpp"
#include "../gameplan.hpp"
#include "../pagefactory.hpp"
#include "../settings.hpp"
#include "../../league/leaguecode.hpp"
#include "main.hpp"
#include "replaymenu.hpp"
#include "utils/localization.hpp"
#include "onthepitch/match.hpp"

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

  Gui2Frame* frame = new Gui2Frame(windowManager, "frame_ingame", 10, 8, 80, 84, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_ingame_title", 2, 2, 76, 3,
                                       Localization::GetInstance().Translate("ingame_pause"));
  title->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  frame->AddView(title);
  title->Show();

  Gui2Caption* scoreLine =
      new Gui2Caption(windowManager, "caption_ingame_score", 2, 6, 76, 3, scoreBuf);
  scoreLine->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright1));
  frame->AddView(scoreLine);
  scoreLine->Show();
  
  // STATS GRID (Right side)
  MatchData* md = match->GetMatchData();
  Gui2Grid* statsGrid = new Gui2Grid(windowManager, "stats_grid", 40, 12, 38, 35);
  frame->AddView(statsGrid);
  Gui2Caption* h0 = new Gui2Caption(windowManager, "s_t1", 0, 0, 12, 3, md->GetTeamData(0)->GetShortName());
  Gui2Caption* h1 = new Gui2Caption(windowManager, "s_l1", 0, 0, 14, 3,
                                     Localization::GetInstance().Translate("ingame_match_stats"));
  Gui2Caption* h2 = new Gui2Caption(windowManager, "s_t2", 0, 0, 12, 3, md->GetTeamData(1)->GetShortName());
  h0->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  h1->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  h2->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  statsGrid->AddView(h0, 0, 0);
  statsGrid->AddView(h1, 0, 1);
  statsGrid->AddView(h2, 0, 2);
  statsGrid->AddView(new Gui2Caption(windowManager, "s_v1", 0, 0, 12, 3, int_to_str(md->GetGoalCount(0))), 1, 0);
  statsGrid->AddView(new Gui2Caption(windowManager, "s_l2", 0, 0, 14, 3,
                                     Localization::GetInstance().Translate("ingame_score")), 1, 1);
  statsGrid->AddView(new Gui2Caption(windowManager, "s_v2", 0, 0, 12, 3, int_to_str(md->GetGoalCount(1))), 1, 2);
  unsigned long p0 = md->GetPossessionTime_ms(0);
  unsigned long p1 = md->GetPossessionTime_ms(1);
  int p0_pct = (p0 + p1 > 0) ? (p0 * 100) / (p0 + p1) : 50;
  statsGrid->AddView(new Gui2Caption(windowManager, "s_p1", 0, 0, 12, 3, int_to_str(p0_pct) + "%"), 2, 0);
  statsGrid->AddView(new Gui2Caption(windowManager, "s_l3", 0, 0, 14, 3,
                                     Localization::GetInstance().Translate("ingame_possession")), 2, 1);
  statsGrid->AddView(new Gui2Caption(windowManager, "s_p2", 0, 0, 12, 3, int_to_str(100 - p0_pct) + "%"), 2, 2);
  statsGrid->AddView(new Gui2Caption(windowManager, "s_s1", 0, 0, 12, 3, int_to_str(md->GetShots(0))), 3, 0);
  statsGrid->AddView(new Gui2Caption(windowManager, "s_l4", 0, 0, 14, 3,
                                     Localization::GetInstance().Translate("ingame_shots")), 3, 1);
  statsGrid->AddView(new Gui2Caption(windowManager, "s_s2", 0, 0, 12, 3, int_to_str(md->GetShots(1))), 3, 2);
  statsGrid->UpdateLayout(0.5);
  statsGrid->Show();

  // Navigation Grid (Left side)
  Gui2Grid* grid = new Gui2Grid(windowManager, "grid_ingame", 2, 12, 36, 80);
  int row = 0;

  Gui2Caption* tacticsLabel =
      new Gui2Caption(windowManager, "caption_ingame_section_tactics", 0, 0, 36, 2,
                      Localization::GetInstance().Translate("ingame_section_tactics"));
  tacticsLabel->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  grid->AddView(tacticsLabel, row++, 0);

  Gui2Button* buttonGamePlan =
      new Gui2Button(windowManager, "button_gameplan", 0, 0, 36, 3,
                     Localization::GetInstance().Translate("ingame_game_plan"));
  Gui2Button* buttonSetPieces =
      new Gui2Button(windowManager, "button_setpieces", 0, 0, 36, 3,
                     Localization::GetInstance().Translate("ingame_set_pieces"));
  buttonGamePlan->sig_OnClick.connect([this](...) { GoGamePlan(); });
  buttonSetPieces->sig_OnClick.connect([this](...) { GoSetPieceEditor(); });
  grid->AddView(buttonGamePlan, row++, 0);
  grid->AddView(buttonSetPieces, row++, 0);

  Gui2Caption* settingsLabel =
      new Gui2Caption(windowManager, "caption_ingame_section_settings", 0, 0, 36, 2,
                      Localization::GetInstance().Translate("ingame_section_settings"));
  settingsLabel->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  grid->AddView(settingsLabel, row++, 0);

  Gui2Button* buttonControllerSelect =
      new Gui2Button(windowManager, "button_controllerselect", 0, 0, 36, 3,
                     Localization::GetInstance().Translate("ingame_controller_select"));
  Gui2Button* buttonCameraSettings =
      new Gui2Button(windowManager, "button_camerasettings", 0, 0, 36, 3,
                     Localization::GetInstance().Translate("ingame_camera_settings"));
  Gui2Button* buttonVisualOptions =
      new Gui2Button(windowManager, "button_visualoptions", 0, 0, 36, 3,
                     Localization::GetInstance().Translate("ingame_visual_options"));
  Gui2Button* buttonSystemSettings =
      new Gui2Button(windowManager, "button_systemsettings", 0, 0, 36, 3,
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
      new Gui2Caption(windowManager, "caption_ingame_section_media", 0, 0, 36, 2,
                      Localization::GetInstance().Translate("ingame_section_media"));
  grid->AddView(mediaLabel, row++, 0);

  Gui2Button* buttonReplay = new Gui2Button(windowManager, "button_replay", 0, 0, 36, 3,
                                            Localization::GetInstance().Translate("ingame_replay"));
  buttonReplay->sig_OnClick.connect([this](...) { GoReplay(); });
  grid->AddView(buttonReplay, row++, 0);

  Gui2Caption* exitLabel =
      new Gui2Caption(windowManager, "caption_ingame_section_exit", 0, 0, 36, 2,
                      Localization::GetInstance().Translate("ingame_section_match"));
  grid->AddView(exitLabel, row++, 0);

  Gui2Button* buttonResume =
      new Gui2Button(windowManager, "button_resume", 0, 0, 36, 3,
                     Localization::GetInstance().Translate("ingame_resume_match"));
  Gui2Button* buttonPreQuit =
      new Gui2Button(windowManager, "button_quit", 0, 0, 36, 3,
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

  Gui2Caption* hintCaption = new Gui2Caption(windowManager, "caption_ingame_hint", 2, 79, 36, 2,
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
  // Forfeiting a league match must not leave a stale fixture armed for the
  // next 3D game's result flow-back.
  LeagueClearPendingFixture();
  this->Exit();
  GetMenuTask()->SetMenuAction(e_MenuAction_Menu);
  delete this;
}
