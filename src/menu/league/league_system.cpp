#include "league_system.hpp"

#include "../../league/leaguecode.hpp"
#include "../../main.hpp"
#include "../pagefactory.hpp"
#include "base/utils.hpp"
#include "menu_smoke.hpp"
#include "utils/gui2/widgets/editline.hpp"
#include "utils/gui2/widgets/pulldown.hpp"
#include "utils/gui2/widgets/slider.hpp"

LeagueSystemPage::LeagueSystemPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData),
      pageCreatedTime_ms(league_menu_smoke::Now_ms()),
      autoAdvanceTriggered(false) {
  Gui2Frame* frame = new Gui2Frame(windowManager, "frame_league_system", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_league_system", 2, 2, 66, 3, "System");
  frame->AddView(title);
  title->Show();

  Gui2Button* btnSave = new Gui2Button(windowManager, "btn_system_save", 0, 0, 60, 3, "Save");
  Gui2Button* btnSettings =
      new Gui2Button(windowManager, "btn_system_settings", 0, 0, 60, 3, "Settings");
  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_system_back", 0, 0, 60, 3, "Back to Dashboard");

  btnSave->sig_OnClick.connect([this](...) { GoPage(e_PageID_League_System_Save); });
  btnSettings->sig_OnClick.connect([this](...) { GoPage(e_PageID_League_System_Settings); });
  btnBack->sig_OnClick.connect([this](...) { GoPage(e_PageID_League_Forward); });

  Gui2Grid* grid = new Gui2Grid(windowManager, "grid_system", 2, 10, 66, 50);
  grid->AddView(btnSave, 0, 0);
  grid->AddView(btnSettings, 1, 0);
  grid->AddView(btnBack, 2, 0);
  grid->UpdateLayout(0.5);
  frame->AddView(grid);
  grid->Show();

  btnSave->SetFocus();
  this->Show();
}

LeagueSystemPage::~LeagueSystemPage() {}

void LeagueSystemPage::Process() {
  Gui2Page::Process();

  if (!league_menu_smoke::RouteEnabled("system_settings") || autoAdvanceTriggered ||
      league_menu_smoke::Now_ms() < pageCreatedTime_ms + league_menu_smoke::kAdvanceDelay_ms) {
    return;
  }

  autoAdvanceTriggered = true;
  printf("[menu-smoke] System page opening settings\n");
  GoPage(e_PageID_League_System_Settings);
}

void LeagueSystemPage::GoPage(e_PageID pageID) {
  this->Exit();
  Properties properties;
  windowManager->GetPageFactory()->CreatePage(static_cast<int>(pageID), properties, 0);
  delete this;
}

LeagueSystemSavePage::LeagueSystemSavePage(Gui2WindowManager* windowManager,
                                           const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* frame = new Gui2Frame(windowManager, "frame_league_save", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_league_system_save", 2, 2, 66, 3, "Save Game");
  frame->AddView(title);
  title->Show();

  std::string saveDir = GetActiveSaveDirectory();
  Gui2Caption* info = new Gui2Caption(windowManager, "caption_save_info", 2, 10, 66, 6,
                                      "Game saves automatically. Your current save: " + saveDir);
  frame->AddView(info);
  info->Show();

  Gui2Button* btnSave =
      new Gui2Button(windowManager, "btn_save_manual", 30, 40, 40, 3, "Manual Save");
  btnSave->sig_OnClick.connect([this, windowManager, frame](...) {
    SaveAutosaveToDatabase();
    Gui2Caption* feedback =
        new Gui2Caption(windowManager, "caption_save_feedback", 2, 30, 66, 3, "Save successful!");
    frame->AddView(feedback);
    feedback->Show();
  });
  frame->AddView(btnSave);
  btnSave->Show();

  Gui2Button* btnBack = new Gui2Button(windowManager, "btn_save_back", 30, 90, 40, 3, "Back");
  btnBack->sig_OnClick.connect([this, windowManager](...) {
    this->Exit();
    Properties properties;
    windowManager->GetPageFactory()->CreatePage(static_cast<int>(e_PageID_League_System),
                                                properties, 0);
    delete this;
  });
  frame->AddView(btnBack);
  btnBack->Show();
  btnSave->SetFocus();
  this->Show();
}

LeagueSystemSavePage::~LeagueSystemSavePage() {}

LeagueSystemSettingsPage::LeagueSystemSettingsPage(Gui2WindowManager* windowManager,
                                                   const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData),
      editManagerName(nullptr),
      pulldownCurrency(nullptr),
      sliderDifficulty(nullptr),
      editSeasonYear(nullptr),
      frame(nullptr),
      pageCreatedTime_ms(league_menu_smoke::Now_ms()),
      autoAdvanceTriggered(false) {
  frame = new Gui2Frame(windowManager, "frame_league_settings", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_league_system_settings", 2, 2, 66, 3, "Settings");
  frame->AddView(title);
  title->Show();

  auto result = GetDB()->Query(
      "SELECT s.managername, t.name, s.currency, s.difficulty, s.seasonyear "
      "FROM settings s JOIN teams t ON s.team_id = t.id LIMIT 1");

  Gui2Grid* grid = new Gui2Grid(windowManager, "grid_settings", 2, 10, 66, 60);
  int row = 0;

  std::string curManagerName = "Manager";
  std::string curTeamName = "Unknown";
  std::string curCurrency = "euro";
  float curDifficulty = 0.5f;
  std::string curSeasonYear = "2026";

  if (!result->data.empty()) {
    curManagerName = result->data.at(0).at(0);
    curTeamName = result->data.at(0).at(1);
    curCurrency = result->data.at(0).at(2);
    curDifficulty = atof(result->data.at(0).at(3).c_str());
    curSeasonYear = result->data.at(0).at(4);
  }

  Gui2Caption* lblMgr =
      new Gui2Caption(windowManager, "caption_set_mgr", 0, 0, 40, 2.5, "Manager Name:");
  editManagerName =
      new Gui2EditLine(windowManager, "editline_set_mgr", 0, 0, 40, 3, curManagerName);
  editManagerName->SetMaxLength(32);
  grid->AddView(lblMgr, row, 0);
  grid->AddView(editManagerName, row++, 1);

  Gui2Caption* lblTeam = new Gui2Caption(windowManager, "caption_set_team", 0, 0, 40, 2.5, "Team:");
  Gui2Caption* valTeam =
      new Gui2Caption(windowManager, "caption_set_team_val", 0, 0, 40, 2.5, curTeamName);
  grid->AddView(lblTeam, row, 0);
  grid->AddView(valTeam, row++, 1);

  Gui2Caption* lblCur =
      new Gui2Caption(windowManager, "caption_set_currency", 0, 0, 40, 2.5, "Currency:");
  pulldownCurrency = new Gui2Pulldown(windowManager, "pulldown_set_currency", 0, 0, 40, 3);
  pulldownCurrency->AddEntry("Euro", "euro");
  pulldownCurrency->AddEntry("Dollar", "dollar");
  pulldownCurrency->AddEntry("Yen", "yen");
  pulldownCurrency->AddEntry("Pound", "pound");
  pulldownCurrency->AddEntry("Swiss franc", "swissfranc");
  pulldownCurrency->AddEntry("Australian dollar", "ausdollar");
  pulldownCurrency->AddEntry("Canadian dollar", "candollar");
  pulldownCurrency->AddEntry("Swedish krone", "swekrone");
  pulldownCurrency->AddEntry("Hong Kong dollar", "hongkongdollar");
  pulldownCurrency->AddEntry("Norwegian krone", "norkrone");
  pulldownCurrency->SetSelectedByName(curCurrency);
  grid->AddView(lblCur, row, 0);
  grid->AddView(pulldownCurrency, row++, 1);

  Gui2Caption* lblDiff =
      new Gui2Caption(windowManager, "caption_set_diff", 0, 0, 40, 2.5, "Difficulty:");
  sliderDifficulty = new Gui2Slider(windowManager, "slider_set_diff", 0, 0, 40, 6, "Difficulty");
  sliderDifficulty->SetValue(curDifficulty);
  grid->AddView(lblDiff, row, 0);
  grid->AddView(sliderDifficulty, row++, 1);

  Gui2Caption* lblSeason =
      new Gui2Caption(windowManager, "caption_set_season", 0, 0, 40, 2.5, "Season Year:");
  editSeasonYear =
      new Gui2EditLine(windowManager, "editline_set_season", 0, 0, 40, 3, curSeasonYear);
  editSeasonYear->SetAllowedChars("0123456789");
  editSeasonYear->SetMaxLength(4);
  grid->AddView(lblSeason, row, 0);
  grid->AddView(editSeasonYear, row++, 1);

  Gui2Button* btnSave =
      new Gui2Button(windowManager, "btn_settings_save", 0, 0, 60, 3, "Save Settings");
  btnSave->sig_OnClick.connect([this](...) { SaveSettings(); });
  grid->AddView(btnSave, row++, 0);

  grid->UpdateLayout(0.5);
  frame->AddView(grid);
  grid->Show();

  Gui2Button* btnBack = new Gui2Button(windowManager, "btn_settings_back", 30, 90, 40, 3, "Back");
  btnBack->sig_OnClick.connect([this, windowManager](...) {
    this->Exit();
    Properties properties;
    windowManager->GetPageFactory()->CreatePage(static_cast<int>(e_PageID_League_System),
                                                properties, 0);
    delete this;
  });
  frame->AddView(btnBack);
  btnBack->Show();
  editManagerName->SetFocus();
  this->Show();
}

LeagueSystemSettingsPage::~LeagueSystemSettingsPage() {}

void LeagueSystemSettingsPage::SaveSettings() {
  GetDB()->Query("UPDATE settings SET managername = '" + editManagerName->GetText() +
                 "', currency = '" + pulldownCurrency->GetSelected() +
                 "', difficulty = " + real_to_str(sliderDifficulty->GetValue()) +
                 ", seasonyear = " + editSeasonYear->GetText());

  Gui2Caption* feedback =
      new Gui2Caption(windowManager, "caption_settings_saved", 2, 85, 66, 3, "Settings saved!");
  frame->AddView(feedback);
  feedback->Show();
}

void LeagueSystemSettingsPage::Process() {
  Gui2Page::Process();

  if (!league_menu_smoke::RouteEnabled("system_settings") || autoAdvanceTriggered ||
      league_menu_smoke::Now_ms() < pageCreatedTime_ms + league_menu_smoke::kQuitDelay_ms) {
    return;
  }

  autoAdvanceTriggered = true;
  printf("[menu-smoke] League system settings reached successfully\n");
  GetMenuTask()->QuitGame();
}
