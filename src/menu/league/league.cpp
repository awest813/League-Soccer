// written by bastiaan konings schuiling 2008 - 2015
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not
// be used for anything important. i do not offer support, so don't ask. to be used for inspiration
// :)

#include "league.hpp"

#include <cctype>
#include <chrono>
#include <ctime>

#include "../../league/leaguecode.hpp"
#include "../../main.hpp"
#include "../pagefactory.hpp"
#include "base/utils.hpp"
#include "menu_smoke.hpp"
#include "utils/database.hpp"
#include "utils/gui2/widgets/caption.hpp"
#include "utils/gui2/widgets/frame.hpp"
#include "utils/gui2/widgets/root.hpp"
#include "utils/gui2/widgets/text.hpp"
#include "utils/localization.hpp"

namespace {

std::string MakeMenuSmokeLeagueSaveName() {
  const auto now = std::chrono::system_clock::now().time_since_epoch();
  const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
  std::string route = league_menu_smoke::GetRoute();
  if (route.empty()) {
    route = "bootstrap";
  }
  for (char& ch : route) {
    if (!(std::isalnum(static_cast<unsigned char>(ch)) || ch == '_')) {
      ch = '_';
    }
  }
  return "MenuSmokeLeague_" + route + "_" + std::to_string(milliseconds % 1000000000LL);
}

}  // namespace

LeaguePage::LeaguePage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData),
      pageCreatedTime_ms(EnvironmentManager::GetInstance().GetTime_ms()),
      autoAdvanceTriggered(false),
      autoStepTriggered(false) {
  Gui2Frame* bgPanel = new Gui2Frame(windowManager, "bg_league", 15, 5, 70, 90, true);
  this->AddView(bgPanel);
  bgPanel->Show();

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_league", 2, 2, 66, 3, "League Mode");
  bgPanel->AddView(title);
  title->Show();

  captionTime = new Gui2Caption(windowManager, "caption_league_time", 40, 2, 28, 3, "time");
  bgPanel->AddView(captionTime);
  captionTime->Show();

  SetTimeCaption();

  Gui2Grid* grid = new Gui2Grid(windowManager, "grid_league_main", 2, 8, 66, 60);

  Gui2Button* buttonForward =
      new Gui2Button(windowManager, "button_league_forward", 0, 0, 36, 5, "Open Dashboard");
  buttonForward->sig_OnClick.connect([this](...) { GoForward(); });
  buttonForward->SetFocus();

  Gui2Button* buttonStepTime =
      new Gui2Button(windowManager, "button_league_steptime", 0, 0, 36, 5, "Advance One Day");
  buttonStepTime->sig_OnClick.connect([this](...) { StepTime(); });

  Gui2Button* buttonMainMenu =
      new Gui2Button(windowManager, "button_league_mainmenu", 0, 0, 36, 5, "Return to Main Menu");
  buttonMainMenu->sig_OnClick.connect([this](...) { GoMainMenu(); });

  grid->AddView(buttonForward, 0, 0);
  grid->AddView(buttonStepTime, 0, 1);
  grid->AddView(buttonMainMenu, 1, 0);

  bgPanel->AddView(grid);
  grid->UpdateLayout();
  grid->Show();

  this->Show();
}

LeaguePage::~LeaguePage() {}

void LeaguePage::Process() {
  Gui2Page::Process();

  if (league_menu_smoke::AnyEnabled() && !autoStepTriggered &&
      league_menu_smoke::Now_ms() >= pageCreatedTime_ms + league_menu_smoke::kAdvanceDelay_ms) {
    autoStepTriggered = true;
    printf("[menu-smoke] League page reached, advancing time once\n");
    StepTime();
  }

  if (league_menu_smoke::HasRoute() && !autoAdvanceTriggered &&
      league_menu_smoke::Now_ms() >= pageCreatedTime_ms + league_menu_smoke::kQuitDelay_ms) {
    autoAdvanceTriggered = true;
    printf("[menu-smoke] League page ready, opening dashboard\n");
    GoForward();
  } else if (league_menu_smoke::BootstrapEnabled() && !league_menu_smoke::HasRoute() &&
             !autoAdvanceTriggered &&
             league_menu_smoke::Now_ms() >= pageCreatedTime_ms + league_menu_smoke::kQuitDelay_ms) {
    autoAdvanceTriggered = true;
    printf("[menu-smoke] League page flow succeeded\n");
    GetMenuTask()->QuitGame();
  }
}

void LeaguePage::GoForward() {
  this->Exit();

  Properties properties;
  windowManager->GetPageFactory()->CreatePage((int)e_PageID_League_Forward, properties, 0);

  delete this;
}

void LeaguePage::GoMainMenu() {
  SaveAutosaveToDatabase();

  this->Exit();

  Properties properties;
  windowManager->GetPageFactory()->CreatePage((int)e_PageID_MainMenu, properties, 0);

  delete this;
}

void LeaguePage::StepTime() {
  if (StepLeagueTime()) {
    SetTimeCaption();
  }
}

void LeaguePage::SetTimeCaption() {
  auto result = GetDB()->Query("SELECT timestamp, strftime('%w', timestamp) FROM settings LIMIT 1");
  if (result->data.empty() || result->data.at(0).size() < 2)
    return;
  std::string dayName;
  switch (atoi(result->data.at(0).at(1).c_str())) {
    case 0:
      dayName = "sunday";
      break;
    case 1:
      dayName = "monday";
      break;
    case 2:
      dayName = "tuesday";
      break;
    case 3:
      dayName = "wednesday";
      break;
    case 4:
      dayName = "thursday";
      break;
    case 5:
      dayName = "friday";
      break;
    case 6:
      dayName = "saturday";
      break;
    default:
      dayName = "bug";
      break;
  }
  captionTime->SetCaption(result->data.at(0).at(0) + " (" + dayName + ")");
}

LeagueStartPage::LeagueStartPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData),
      pageCreatedTime_ms(league_menu_smoke::Now_ms()),
      autoAdvanceTriggered(false) {
  Gui2Frame* frame = new Gui2Frame(windowManager, "bg_league_start", 30, 30, 40, 40, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_league_start", 5, 4, 30, 3, "League Mode");
  frame->AddView(title);
  title->Show();

  Gui2Button* buttonLoad = new Gui2Button(windowManager, "button_league_start_load", 0, 0, 30, 3,
                                          "Continue Saved League");
  buttonLoad->sig_OnClick.connect([this](...) { GoLoad(); });
  Gui2Button* buttonNew =
      new Gui2Button(windowManager, "button_league_start_new", 0, 0, 30, 3, "Start New League");
  buttonNew->sig_OnClick.connect([this](...) { GoNew(); });
  Gui2Button* buttonBack = new Gui2Button(windowManager, "button_league_start_back", 0, 0, 30, 3,
                                          Localization::GetInstance().Translate("action_back"));
  buttonBack->sig_OnClick.connect([this](...) { GoBack(); });

  Gui2Grid* grid = new Gui2Grid(windowManager, "grid_league_start_choices", 5, 10, 90, 80);
  grid->AddView(buttonLoad, 0, 0);
  grid->AddView(buttonNew, 1, 0);
  grid->AddView(buttonBack, 2, 0);
  grid->UpdateLayout(0.5);
  frame->AddView(grid);
  grid->Show();

  buttonLoad->SetFocus();

  this->Show();
}

LeagueStartPage::~LeagueStartPage() {}

void LeagueStartPage::Process() {
  Gui2Page::Process();

  if (!autoAdvanceTriggered && league_menu_smoke::AnyEnabled() &&
      league_menu_smoke::Now_ms() >= pageCreatedTime_ms + league_menu_smoke::kAdvanceDelay_ms) {
    autoAdvanceTriggered = true;
    printf("[menu-smoke] League start reached, creating a new league\n");
    GoNew();
  }
}

void LeagueStartPage::GoLoad() {
  this->Exit();

  Properties properties;
  windowManager->GetPageFactory()->CreatePage((int)e_PageID_League_Start_Load, properties, 0);

  delete this;
}

void LeagueStartPage::GoNew() {
  this->Exit();

  Properties properties;
  windowManager->GetPageFactory()->CreatePage((int)e_PageID_League_Start_New, properties, 0);

  delete this;
}

LeagueStartLoadPage::LeagueStartLoadPage(Gui2WindowManager* windowManager,
                                         const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* frame = new Gui2Frame(windowManager, "bg_league_start_load", 10, 5, 80, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_league_start_load", 2, 2, 76, 3, "Load Saved League");
  frame->AddView(title);
  title->Show();

  browser = new Gui2FileBrowser(windowManager, "filebrowser_league_start_load", 2, 10, 76, 75,
                                "./saves", e_DirEntryType_Directory);
  frame->AddView(browser);
  browser->Show();

  browser->sig_OnClick.connect([this](...) { GoLoadSave(); });

  browser->SetFocus();

  this->Show();
}

LeagueStartLoadPage::~LeagueStartLoadPage() {}

void LeagueStartLoadPage::GoLoadSave() {
  std::string saveName = browser->GetClickedEntry().name;
  SetActiveSaveDirectory(saveName);

  std::filesystem::path saveLoc("saves");
  saveLoc /= saveName;

  SaveDatabaseToAutosave();

  GetDB()->Load(saveLoc.string() + "/autosave.sqlite");

  auto checkResult = GetDB()->Query(
      "SELECT name FROM sqlite_master WHERE type = 'table' AND name = 'match_results' LIMIT 1");
  if (checkResult->data.empty()) {
    GetDB()->Query(
        "CREATE TABLE match_results(id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "calendar_id INTEGER, "
        "team1_id INTEGER, "
        "team2_id INTEGER, "
        "team1_goals INTEGER DEFAULT 0, "
        "team2_goals INTEGER DEFAULT 0, "
        "played INTEGER DEFAULT 0, "
        "competition_id INTEGER)");
  }

  auto inboxCheck = GetDB()->Query(
      "SELECT name FROM sqlite_master WHERE type = 'table' AND name = 'inbox_messages' LIMIT 1");
  if (inboxCheck->data.empty()) {
    GetDB()->Query(
        "CREATE TABLE inbox_messages(id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "sender VARCHAR(64), "
        "subject VARCHAR(128), "
        "body TEXT, "
        "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP, "
        "read INTEGER DEFAULT 0)");
  }

  this->Exit();

  Properties properties;
  windowManager->GetPageFactory()->CreatePage((int)e_PageID_League, properties, 0);

  delete this;
}

LeagueStartNewPage::LeagueStartNewPage(Gui2WindowManager* windowManager,
                                       const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData),
      databaseSelectDialog(nullptr),
      databaseSelectBrowser(nullptr),
      previousFocus(nullptr),
      createSaveDialog(nullptr),
      success(false),
      pageCreatedTime_ms(league_menu_smoke::Now_ms()),
      dialogShownTime_ms(0),
      autoAdvanceTriggered(false),
      autoCloseDialogTriggered(false) {
  data_SelectedDatabase = "default";

  Gui2Frame* frame = new Gui2Frame(windowManager, "bg_league_start_new", 5, 5, 90, 90, true);

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_league_start_new", 5, 5, 20, 3, "Start new league");
  frame->AddView(title);
  title->Show();

  Gui2Grid* grid = new Gui2Grid(windowManager, "grid_league_start_new_choices", 5, 15, 90, 80);

  Gui2Caption* databaseSelectCaption =
      new Gui2Caption(windowManager, "caption_league_start_new_dbselect", 0, 0, 30, 2.5,
                      "Select foundation database");
  Gui2Caption* currencySelectCaption = new Gui2Caption(
      windowManager, "caption_league_start_new_currency", 0, 0, 30, 2.5, "Select currency");
  Gui2Caption* saveNameCaption = new Gui2Caption(
      windowManager, "caption_league_start_new_savegamename", 0, 0, 30, 2.5, "Savegame name");
  Gui2Caption* managerNameCaption = new Gui2Caption(
      windowManager, "caption_league_start_new_managername", 0, 0, 30, 2.5, "Manager name");

  Gui2Caption* teamSelectCaption = new Gui2Caption(
      windowManager, "caption_league_start_new_teamselect", 0, 0, 30, 2.5, "Select your team");

  databaseSelectButton = new Gui2Button(windowManager, "button_league_start_new_dbselect", 0, 0, 30,
                                        3, data_SelectedDatabase);
  databaseSelectButton->sig_OnClick.connect([this](...) { GoDatabaseSelectDialog(); });

  teamSelectPulldown =
      new Gui2Pulldown(windowManager, "pulldown_league_start_new_teamselect", 0, 0, 30, 3);
  teamSelectPulldown->sig_OnChange.connect(
      [this](Gui2Pulldown* pd) { data_SelectedTeamID = pd->GetSelected(); });

  currencySelectPulldown =
      new Gui2Pulldown(windowManager, "pulldown_league_start_new_currencyselect", 0, 0, 30, 3);
  currencySelectPulldown->AddEntry("Euro", "euro");
  currencySelectPulldown->AddEntry("Dollar", "dollar");
  currencySelectPulldown->AddEntry("Yen", "yen");
  currencySelectPulldown->AddEntry("Pound", "pound");
  currencySelectPulldown->AddEntry("Swiss franc", "swissfranc");
  currencySelectPulldown->AddEntry("Australian dollar", "ausdollar");
  currencySelectPulldown->AddEntry("Canadian dollar", "candollar");
  currencySelectPulldown->AddEntry("Swedish krone", "swekrone");
  currencySelectPulldown->AddEntry("Hong Kong dollar", "hongkongdollar");
  currencySelectPulldown->AddEntry("Norwegian krone", "norkrone");

  difficultySlider = new Gui2Slider(windowManager, "slider_league_start_new_difficulty", 0, 0, 30,
                                    6, "Initial difficulty");
  saveNameInput = new Gui2EditLine(windowManager, "editline_league_start_new_savegamename", 0, 0,
                                   30, 3, "NewLeague");
  saveNameInput->SetAllowedChars(
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890_ ");
  saveNameInput->SetMaxLength(24);
  managerNameInput = new Gui2EditLine(windowManager, "editline_league_start_new_managername", 0, 0,
                                      30, 3, "Titi Fillanovi");
  managerNameInput->SetAllowedChars(
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890-=`~@#$%^&*()_+[]{}\\,./"
      "<>?;':\" ");
  managerNameInput->SetMaxLength(32);
  Gui2Button* proceedButton =
      new Gui2Button(windowManager, "button_league_start_new_proceed", 0, 0, 30, 3, "Proceed");
  proceedButton->sig_OnClick.connect([this](...) { GoProceed(); });

  Gui2Grid* gridDBSelect =
      new Gui2Grid(windowManager, "grid_league_start_new_choices_dbselect", 0, 0, 1, 1);
  gridDBSelect->AddView(databaseSelectCaption, 0, 0);
  gridDBSelect->AddView(databaseSelectButton, 1, 0);
  gridDBSelect->UpdateLayout(0.0, 0.0, 0.5, 0.5);
  gridDBSelect->SetWrapping(false);
  grid->AddView(gridDBSelect, 0, 0);

  Gui2Grid* gridTeamSelect =
      new Gui2Grid(windowManager, "grid_league_start_new_teamselect", 0, 0, 1, 1);
  gridTeamSelect->AddView(teamSelectCaption, 0, 0);
  gridTeamSelect->AddView(teamSelectPulldown, 1, 0);
  gridTeamSelect->UpdateLayout(0.0, 0.0, 0.5, 0.5);
  gridTeamSelect->SetWrapping(false);
  grid->AddView(gridTeamSelect, 1, 0);

  Gui2Grid* gridCurrencySelect =
      new Gui2Grid(windowManager, "grid_league_start_new_choices_currencyselect", 0, 0, 1, 1);
  gridCurrencySelect->AddView(currencySelectCaption, 0, 0);
  gridCurrencySelect->AddView(currencySelectPulldown, 1, 0);
  gridCurrencySelect->UpdateLayout(0.0, 0.0, 0.5, 0.5);
  gridCurrencySelect->SetWrapping(false);
  grid->AddView(gridCurrencySelect, 2, 0);

  grid->AddView(difficultySlider, 3, 0);

  Gui2Grid* gridSaveName =
      new Gui2Grid(windowManager, "grid_league_start_new_choices_savegamename", 0, 0, 1, 1);
  gridSaveName->AddView(saveNameCaption, 0, 0);
  gridSaveName->AddView(saveNameInput, 1, 0);
  gridSaveName->UpdateLayout(0.0, 0.0, 0.5, 0.5);
  gridSaveName->SetWrapping(false);
  grid->AddView(gridSaveName, 5, 0);

  Gui2Grid* gridManagerName =
      new Gui2Grid(windowManager, "grid_league_start_new_choices_managername", 0, 0, 1, 1);
  gridManagerName->AddView(managerNameCaption, 0, 0);
  gridManagerName->AddView(managerNameInput, 1, 0);
  gridManagerName->UpdateLayout(0.0, 0.0, 0.5, 0.5);
  gridManagerName->SetWrapping(false);
  grid->AddView(gridManagerName, 6, 0);

  grid->AddView(proceedButton, 7, 0);

  grid->UpdateLayout(0.0, 0.0, 0.0, 3.0);
  frame->AddView(grid);
  grid->Show();

  Gui2Text* explanationText = new Gui2Text(
      windowManager, "grid_league_start_new_choices_explanation", 40, 15, 40, 75, 2.5, 40, "");

  explanationText->AddText((std::string)
                           "The foundation database will be copied to a new directory that will serve as a 'save file' for your league. So, this database is what " +
                           "your league will be based on; any changes to the foundation database later on won't affect your league save (or the other way round).");
  explanationText->AddEmptyLine();
  explanationText->AddText((std::string)
                           "Foundation databases are stored in the 'databases' subdirectory of your League Soccer installation. " +
                           "Saved leagues and cups are stored in the 'saves' directory.");

  frame->AddView(explanationText);
  explanationText->Show();

  currencySelectPulldown->SetSelected(0);
  RefreshTeamSelect();
  if (league_menu_smoke::AnyEnabled()) {
    saveNameInput->SetText(MakeMenuSmokeLeagueSaveName());
  }

  databaseSelectButton->SetFocus();

  this->AddView(frame);
  frame->Show();

  this->Show();
}

LeagueStartNewPage::~LeagueStartNewPage() {}

void LeagueStartNewPage::Process() {
  Gui2Page::Process();

  const unsigned long now_ms = league_menu_smoke::Now_ms();

  if (!autoAdvanceTriggered && league_menu_smoke::AnyEnabled() &&
      now_ms >= pageCreatedTime_ms + league_menu_smoke::kAdvanceDelay_ms) {
    autoAdvanceTriggered = true;
    printf("[menu-smoke] League creation page ready, proceeding with defaults\n");
    GoProceed();
    return;
  }

  if (!autoCloseDialogTriggered && createSaveDialog && league_menu_smoke::AnyEnabled() &&
      now_ms >= dialogShownTime_ms + league_menu_smoke::kDialogDelay_ms) {
    autoCloseDialogTriggered = true;
    if (success) {
      printf("[menu-smoke] League save created, entering league page\n");
    } else {
      printf("[menu-smoke] League creation failed, closing result dialog\n");
    }
    CloseCreateSaveDialog();
  }
}

void LeagueStartNewPage::GoDatabaseSelectDialog() {
  databaseSelectDialog = new Gui2Dialog(windowManager, "dialog_league_start_new_dbselect", 30, 25,
                                        40, 50, "Select source database");
  previousFocus = windowManager->GetFocus();

  databaseSelectBrowser =
      new Gui2FileBrowser(windowManager, "filebrowser_league_start_new_dbselect", 0, 0, 39, 40,
                          "./databases", e_DirEntryType_Directory);
  databaseSelectBrowser->sig_OnClick.connect([this](...) { CloseDatabaseSelectDialog(); });
  databaseSelectDialog->AddContent(databaseSelectBrowser);

  this->AddView(databaseSelectDialog);
  databaseSelectDialog->Show();

  databaseSelectBrowser->SetFocus();

  databaseSelectDialog->Show();
}

void LeagueStartNewPage::CloseDatabaseSelectDialog() {
  previousFocus->SetFocus();

  if (databaseSelectBrowser->GetClickedEntry().type == e_DirEntryType_Directory) {
    data_SelectedDatabase = databaseSelectBrowser->GetClickedEntry().name;
    databaseSelectButton->SetCaption(data_SelectedDatabase);
    RefreshTeamSelect();
  }

  databaseSelectDialog->Exit();
  delete databaseSelectDialog;
}

void LeagueStartNewPage::RefreshTeamSelect() {
  teamSelectPulldown->ClearEntries();
  try {
    Database foundationDB;
    std::filesystem::path dbPath("databases");
    dbPath /= data_SelectedDatabase;
    dbPath /= "database.sqlite";
    if (foundationDB.Load(dbPath.string())) {
      auto result = foundationDB.Query("SELECT id, name FROM teams ORDER BY name");
      for (unsigned int r = 0; r < result->data.size(); r++) {
        std::string id = result->data.at(r).at(0);
        std::string name = result->data.at(r).at(1);
        teamSelectPulldown->AddEntry(name, id);
      }
    } else {
      teamSelectPulldown->AddEntry("Cannot load database", "0");
    }
  } catch (...) {
    teamSelectPulldown->AddEntry("Select a database first", "0");
  }
  if (data_SelectedTeamID.empty()) {
    data_SelectedTeamID = "0";
  }
  teamSelectPulldown->SetSelected(0);
  data_SelectedTeamID = teamSelectPulldown->GetSelected();
}

void LeagueStartNewPage::GoProceed() {
  /* the values:
  printf("dbname: %s\n", data_SelectedDatabase.c_str());
  printf("currency: %s\n", currencySelectPulldown->GetSelected().c_str());
  printf("diff: %f\n", difficultySlider->GetValue());
  printf("savegame: %s\n", saveNameInput->GetText().c_str());
  printf("manager: %s\n", managerNameInput->GetText().c_str());
  */

  int errorCode = CreateNewLeagueSave(data_SelectedDatabase, saveNameInput->GetText());

  // result dialog

  previousFocus = windowManager->GetFocus();

  createSaveDialog = new Gui2Dialog(windowManager, "dialog_league_start_new_createsave", 25, 30, 50,
                                    40, "New league creation");
  dialogShownTime_ms = league_menu_smoke::Now_ms();
  autoCloseDialogTriggered = false;

  if (errorCode == 0) {
    Gui2Text* explanationText =
        new Gui2Text(windowManager, "text_league_start_new_createsave", 5, 5, 90, 75, 2.5, 60, "");
    explanationText->AddText((std::string)
                             "League save created successfully. To make a backup, copy: '<game directory>/saves/" + saveNameInput->GetText() + "/'");
    createSaveDialog->AddContent(explanationText);

    (createSaveDialog->AddSingleButton("Continue"))->SetFocus();
    createSaveDialog->sig_OnPositive.connect([this](...) { CloseCreateSaveDialog(); });

    success = true;

  } else {
    std::string errorString;
    switch (errorCode) {
      case 1:
        errorString =
            "Could not create save directory. Do you have write permissions? Or does it already "
            "exist?";
        break;
      case 2:
        errorString = "Could not copy the database file. Check disk space and file permissions.";
        break;
      case 3:
        errorString = "Could not open the copied database file.";
        break;
      case 4:
        errorString = "Could not copy one or more image files. Check disk space and permissions.";
        break;
    }

    Gui2Text* explanationText =
        new Gui2Text(windowManager, "text_league_start_new_createsave", 5, 5, 90, 75, 2.5, 60, "");
    explanationText->AddText((std::string) "League save creation failed: " + errorString);
    explanationText->AddEmptyLine();
    explanationText->AddText((std::string)
                             "Resolve the issue, then remove any incomplete save folder before trying again: <game directory>/saves/" + saveNameInput->GetText());
    createSaveDialog->AddContent(explanationText);

    (createSaveDialog->AddSingleButton("Close"))->SetFocus();
    createSaveDialog->sig_OnPositive.connect([this](...) { CloseCreateSaveDialog(); });
    // lol forwarding signals overload:
    // createSaveDialog->sig_OnPositive.connect(std::bind(boost:ref(Gui2Dialog::sig_OnClose),
    // createSaveDialog));

    success = false;
  }

  createSaveDialog->Show();

  /* test

  auto result = database->Query("select * from players");

  for (unsigned int h = 0; h < result->header.size(); h++) {
    printf("%s - ", result->header.at(h).c_str());
  }
  printf("\n");
  for (unsigned int r = 0; r < result->data.size(); r++) {
    for (unsigned int c = 0; c < result->data.at(r).size(); c++) {
      printf("%s - ", result->data.at(r).at(c).c_str());
    }
    printf("\n");
  }

  */
}

void LeagueStartNewPage::CloseCreateSaveDialog() {
  previousFocus->SetFocus();

  createSaveDialog->Exit();
  delete createSaveDialog;

  SetActiveSaveDirectory(saveNameInput->GetText());

  std::filesystem::path saveLoc("saves");
  saveLoc /= GetActiveSaveDirectory();

  if (!success) {
    if (league_menu_smoke::AnyEnabled()) {
      GetMenuTask()->QuitGame();
    }
    return;
  }

  GetDB()->Load(saveLoc.string() + "/autosave.sqlite");

  bool noError = PrepareDatabaseForLeague();
  if (!noError)
    Log(e_FatalError, "LeagueStartNewPage", "CloseCreateSaveDialog",
        "Could not prepare database for league");

  auto now_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::tm localTime = {};
  const int currentYear =
      blunted::GetLocalTime(now_time, localTime) ? 1900 + localTime.tm_year : 1970;
  std::string startDate = std::to_string(currentYear) + "-06-01";

  auto result = GetDB()->Query(
      "INSERT INTO settings (managername, team_id, currency, difficulty, seasonyear, timestamp) "
      "VALUES ('" +
      managerNameInput->GetText() + "', " + data_SelectedTeamID + ", '" +
      currencySelectPulldown->GetSelected() + "', " + real_to_str(difficultySlider->GetValue()) +
      ", " + std::to_string(currentYear) + ", '" + startDate + "')");

  GenerateSeasonCalendars();

  noError = SaveAutosaveToDatabase();
  if (!noError)
    Log(e_FatalError, "LeagueStartNewPage", "CloseCreateSaveDialog",
        "Could not save autosave file to persistent database");

  this->Exit();

  Properties properties;
  windowManager->GetPageFactory()->CreatePage((int)e_PageID_League, properties, 0);

  delete this;
}
