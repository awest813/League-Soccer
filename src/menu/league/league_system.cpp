#include "league_system.hpp"

#include <sqlite3.h>

#include "../../league/leaguecode.hpp"
#include "../../main.hpp"
#include "../pagefactory.hpp"
#include "base/utils.hpp"
#include "menu_smoke.hpp"
#include "utils/difficulty.hpp"
#include "utils/gui2/widgets/editline.hpp"
#include "utils/gui2/widgets/pulldown.hpp"
#include "utils/gui2/widgets/slider.hpp"
#include "utils/localization.hpp"

LeagueSystemPage::LeagueSystemPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData),
      pageCreatedTime_ms(league_menu_smoke::Now_ms()),
      autoAdvanceTriggered(false) {
  Gui2Frame* frame = new Gui2Frame(windowManager, "frame_league_system", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_league_system", 2, 2, 66, 3,
                                       Localization::GetInstance().Translate("league_system"));
  frame->AddView(title);
  title->Show();

  Gui2Button* btnSave = new Gui2Button(windowManager, "btn_system_save", 0, 0, 60, 3,
                                       Localization::GetInstance().Translate("action_save"));
  Gui2Button* btnSettings = new Gui2Button(windowManager, "btn_system_settings", 0, 0, 60, 3,
                                           Localization::GetInstance().Translate("settings_title"));
  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_system_back", 0, 0, 60, 3,
                     Localization::GetInstance().Translate("league_back_dashboard"));

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
    : Gui2Page(windowManager, pageData),
      frame(nullptr),
      feedbackCaption(nullptr),
      slotsGrid(nullptr) {
  frame = new Gui2Frame(windowManager, "frame_league_save", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_league_system_save", 2, 2, 66, 3,
                                       Localization::GetInstance().Translate("league_save_game"));
  frame->AddView(title);
  title->Show();

  std::string saveDir = GetActiveSaveDirectory();
  Gui2Caption* info = new Gui2Caption(windowManager, "caption_save_info", 2, 6, 66, 4,
                                      TRF("league_save_info", {saveDir}));
  frame->AddView(info);
  info->Show();

  Gui2Button* btnSave = new Gui2Button(windowManager, "btn_save_manual", 2, 11, 66, 3,
                                       Localization::GetInstance().Translate("league_manual_save"));
  btnSave->sig_OnClick.connect([this](...) {
    SaveAutosaveToDatabase();
    SetFeedback(Localization::GetInstance().Translate("league_save_success"));
  });
  frame->AddView(btnSave);
  btnSave->Show();

  // One replaceable feedback caption instead of a stacking pile of captions.
  feedbackCaption = new Gui2Caption(windowManager, "caption_save_feedback", 2, 84, 66, 3, "");
  frame->AddView(feedbackCaption);
  feedbackCaption->Show();

  RefreshSlots();

  Gui2Button* btnBack = new Gui2Button(windowManager, "btn_save_back", 2, 81, 66, 3,
                                       Localization::GetInstance().Translate("action_back"));
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

void LeagueSystemSavePage::SetFeedback(const std::string& message) {
  if (feedbackCaption) {
    feedbackCaption->SetCaption(message);
  }
}

void LeagueSystemSavePage::RefreshSlots() {
  if (slotsGrid) {
    slotsGrid->Exit();
    delete slotsGrid;
    slotsGrid = nullptr;
  }

  auto& loc = Localization::GetInstance();

  slotsGrid = new Gui2Grid(windowManager, "grid_save_slots", 2, 16, 66, 62);
  for (int slot = 1; slot <= kLeagueMaxSaveSlots; slot++) {
    namespace fs = std::filesystem;
    fs::path slotPath =
        fs::path("saves") / GetActiveSaveDirectory() / ("slot_" + int_to_str(slot) + ".sqlite");
    bool occupied = fs::exists(slotPath);
    std::string detail;
    if (occupied) {
      sqlite3* db = nullptr;
      if (sqlite3_open_v2(slotPath.string().c_str(), &db, SQLITE_OPEN_READONLY, nullptr) ==
          SQLITE_OK) {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, "SELECT managername, timestamp FROM settings LIMIT 1", -1, &stmt,
                               nullptr) == SQLITE_OK) {
          if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* mgr = (const char*)sqlite3_column_text(stmt, 0);
            const char* ts = (const char*)sqlite3_column_text(stmt, 1);
            if (mgr && mgr[0] != '\0') {
              detail = std::string(mgr);
            }
            if (ts && ts[0] != '\0') {
              if (!detail.empty())
                detail += " | ";
              detail += std::string(ts);
            }
          }
          sqlite3_finalize(stmt);
        }
        sqlite3_close(db);
      }
    }
    std::string state = !detail.empty() ? detail
                                        : (occupied ? loc.Translate("league_slot_inuse")
                                                    : loc.Translate("league_slot_empty"));

    Gui2Caption* label = new Gui2Caption(
        windowManager, "caption_slot_" + int_to_str(slot), 0, 0, 26, 2.5,
        loc.TranslateAndFormat("league_save_slot", {int_to_str(slot)}) + " (" + state + ")");
    slotsGrid->AddView(label, slot - 1, 0);

    Gui2Button* btnSave = new Gui2Button(windowManager, "btn_slot_save_" + int_to_str(slot), 0, 0,
                                         12, 2.5, loc.Translate("league_slot_save"));
    btnSave->sig_OnClick.connect([this, slot, occupied](...) {
      if (occupied) {
        Gui2Dialog* dlg =
            new Gui2Dialog(windowManager, "dialog_slot_overwrite", 25, 30, 50, 30,
                           Localization::GetInstance().Translate("league_slot_overwrite"));
        (dlg->AddPosNegButtons(Localization::GetInstance().Translate("league_yes"),
                               Localization::GetInstance().Translate("league_no")))
            ->SetFocus();
        dlg->sig_OnPositive.connect([this, dlg, slot](...) {
          dlg->Exit();
          delete dlg;
          SaveToSlot(slot);
        });
        dlg->sig_OnNegative.connect([this, dlg](...) {
          dlg->Exit();
          delete dlg;
        });
        this->AddView(dlg);
        dlg->Show();
      } else {
        SaveToSlot(slot);
      }
    });
    slotsGrid->AddView(btnSave, slot - 1, 1);

    Gui2Button* btnLoad = new Gui2Button(windowManager, "btn_slot_load_" + int_to_str(slot), 0, 0,
                                         12, 2.5, loc.Translate("league_slot_load"));
    btnLoad->SetActive(occupied);
    btnLoad->sig_OnClick.connect([this, slot](...) { LoadSlot(slot); });
    slotsGrid->AddView(btnLoad, slot - 1, 2);

    Gui2Button* btnDelete = new Gui2Button(windowManager, "btn_slot_delete_" + int_to_str(slot), 0,
                                           0, 12, 2.5, loc.Translate("league_slot_delete"));
    btnDelete->SetActive(occupied);
    btnDelete->sig_OnClick.connect([this, slot](...) { DeleteSlot(slot); });
    slotsGrid->AddView(btnDelete, slot - 1, 3);
  }

  slotsGrid->UpdateLayout(0.4);
  frame->AddView(slotsGrid);
  slotsGrid->Show();
}

void LeagueSystemSavePage::SaveToSlot(int slotIndex) {
  if (LeagueSaveToSlot(slotIndex)) {
    SetFeedback(Localization::GetInstance().TranslateAndFormat("league_slot_saved",
                                                               {int_to_str(slotIndex)}));
    RefreshSlots();
  } else {
    SetFeedback(Localization::GetInstance().Translate("league_slot_failed"));
  }
}

void LeagueSystemSavePage::LoadSlot(int slotIndex) {
  namespace fs = std::filesystem;
  if (!fs::exists(fs::path("saves") / GetActiveSaveDirectory() /
                  ("slot_" + int_to_str(slotIndex) + ".sqlite"))) {
    return;
  }

  Gui2Dialog* dlg =
      new Gui2Dialog(windowManager, "dialog_slot_load", 25, 30, 50, 30,
                     Localization::GetInstance().Translate("league_slot_load_confirm"));
  (dlg->AddPosNegButtons(Localization::GetInstance().Translate("league_yes"),
                         Localization::GetInstance().Translate("league_no")))
      ->SetFocus();
  dlg->sig_OnPositive.connect([this, dlg, slotIndex](...) {
    dlg->Exit();
    delete dlg;
    if (LeagueLoadSlot(slotIndex)) {
      // Everything on screen was built from the old database; restart into the
      // league hub so every page re-queries the freshly loaded slot.
      this->Exit();
      Properties properties;
      windowManager->GetPageFactory()->CreatePage((int)e_PageID_League, properties, 0);
      delete this;
    } else {
      SetFeedback(Localization::GetInstance().Translate("league_slot_failed"));
    }
  });
  dlg->sig_OnNegative.connect([this, dlg](...) {
    dlg->Exit();
    delete dlg;
  });
  this->AddView(dlg);
  dlg->Show();
}

void LeagueSystemSavePage::DeleteSlot(int slotIndex) {
  namespace fs = std::filesystem;
  fs::path slotFile =
      fs::path("saves") / GetActiveSaveDirectory() / ("slot_" + int_to_str(slotIndex) + ".sqlite");
  if (!fs::exists(slotFile)) {
    return;
  }

  Gui2Dialog* dlg =
      new Gui2Dialog(windowManager, "dialog_slot_delete", 25, 30, 50, 30,
                     Localization::GetInstance().Translate("league_slot_delete_confirm"));
  (dlg->AddPosNegButtons(Localization::GetInstance().Translate("league_yes"),
                         Localization::GetInstance().Translate("league_no")))
      ->SetFocus();
  dlg->sig_OnPositive.connect([this, dlg, slotFile](...) {
    dlg->Exit();
    delete dlg;
    std::error_code error;
    fs::remove(slotFile, error);
    SetFeedback(error ? Localization::GetInstance().Translate("league_slot_failed")
                      : Localization::GetInstance().Translate("league_slot_deleted"));
    RefreshSlots();
  });
  dlg->sig_OnNegative.connect([this, dlg](...) {
    dlg->Exit();
    delete dlg;
  });
  this->AddView(dlg);
  dlg->Show();
}

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
      new Gui2Caption(windowManager, "caption_set_mgr", 0, 0, 24, 2.5,
                      Localization::GetInstance().Translate("league_manager_name"));
  editManagerName =
      new Gui2EditLine(windowManager, "editline_set_mgr", 0, 0, 40, 3, curManagerName);
  editManagerName->SetMaxLength(32);
  grid->AddView(lblMgr, row, 0);
  grid->AddView(editManagerName, row++, 1);

  Gui2Caption* lblTeam =
      new Gui2Caption(windowManager, "caption_set_team", 0, 0, 24, 2.5,
                      Localization::GetInstance().Translate("league_team_colon"));
  Gui2Caption* valTeam =
      new Gui2Caption(windowManager, "caption_set_team_val", 0, 0, 40, 2.5, curTeamName);
  grid->AddView(lblTeam, row, 0);
  grid->AddView(valTeam, row++, 1);

  Gui2Caption* lblCur = new Gui2Caption(windowManager, "caption_set_currency", 0, 0, 24, 2.5,
                                        Localization::GetInstance().Translate("league_currency"));
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
      new Gui2Caption(windowManager, "caption_set_diff", 0, 0, 24, 2.5,
                      Localization::GetInstance().Translate("league_difficulty_label"));
  sliderDifficulty = new Gui2Slider(windowManager, "slider_set_diff", 0, 0, 40, 6,
                                    Localization::GetInstance().Translate("league_difficulty"));
  sliderDifficulty->SetQuantization(5);
  sliderDifficulty->AddHelperValue(
      Vector3(80, 80, 250), Localization::GetInstance().Translate("settings_factory_default"),
      _default_Difficulty);
  sliderDifficulty->SetValue(curDifficulty);
  sliderDifficulty->SetValueText(GetDifficultyName(sliderDifficulty->GetValue()));
  sliderDifficulty->sig_OnChange.connect(
      [](Gui2Slider* s) { s->SetValueText(GetDifficultyName(s->GetValue())); });
  grid->AddView(lblDiff, row, 0);
  grid->AddView(sliderDifficulty, row++, 1);

  Gui2Caption* lblSeason =
      new Gui2Caption(windowManager, "caption_set_season", 0, 0, 24, 2.5,
                      Localization::GetInstance().Translate("league_season_year"));
  editSeasonYear =
      new Gui2EditLine(windowManager, "editline_set_season", 0, 0, 40, 3, curSeasonYear);
  editSeasonYear->SetAllowedChars("0123456789");
  editSeasonYear->SetMaxLength(4);
  grid->AddView(lblSeason, row, 0);
  grid->AddView(editSeasonYear, row++, 1);

  Gui2Button* btnSave =
      new Gui2Button(windowManager, "btn_settings_save", 0, 0, 24, 3,
                     Localization::GetInstance().Translate("league_save_settings"));
  btnSave->sig_OnClick.connect([this](...) { SaveSettings(); });
  grid->AddView(btnSave, row++, 0);

  grid->UpdateLayout(0.5);
  frame->AddView(grid);
  grid->Show();

  Gui2Button* btnBack = new Gui2Button(windowManager, "btn_settings_back", 15, 86, 40, 3,
                                       Localization::GetInstance().Translate("action_back"));
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
      new Gui2Caption(windowManager, "caption_settings_saved", 2, 80, 66, 3,
                      Localization::GetInstance().Translate("league_settings_saved"));
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
