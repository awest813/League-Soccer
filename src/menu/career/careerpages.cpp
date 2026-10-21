#include "careerpages.hpp"

#include <algorithm>
#include <cstdio>

#include "../../data/playerdata.hpp"
#include "../../data/teamdata.hpp"
#include "../../gamedefines.hpp"
#include "../../main.hpp"
#include "../pagefactory.hpp"
#include "base/properties.hpp"
#include "base/utils.hpp"
#include "career_database.hpp"
#include "career_navigation.hpp"
#include "career_transfers.hpp"
#include "utils/gui2/widgets/dialog.hpp"
#include "utils/gui2/widgets/text.hpp"
#include "utils/localization.hpp"

using namespace blunted;

namespace {

constexpr unsigned long kCareerMenuSmokeDelay_ms = 500;

std::string GetCareerModeDisplay(const CareerSave* save) {
  return TR(save ? CareerModeLabelKey(save->mode) : "career_mode_default");
}

std::string BuildSeasonProgressLine(const CareerSave* save) {
  if (!save)
    return "";
  return TRF("career_progress_line",
             {std::to_string(save->season.currentWeek), std::to_string(save->season.maxWeeks),
              std::to_string(save->seasonWins), std::to_string(save->seasonDraws),
              std::to_string(save->seasonLosses), std::to_string(save->seasonGoalsFor),
              std::to_string(save->seasonGoalsAgainst)});
}

}  // namespace

static bool IsOwnerMode() {
  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
  return save && save->mode == CareerMode::OWNER_GM;
}

static bool IsDelegatingClubMode() {
  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
  return save && CanManageClub(*save) && !save->handsOnManagement;
}

static bool IsCoachMode() {
  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
  return save && save->mode == CareerMode::COACH;
}

static bool IsPlayerMode() {
  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
  return save && save->mode == CareerMode::PLAYER;
}

static std::string CareerStrategyLabel(const std::string& strategy) {
  const char* names[] = {"Attacking",     "Balanced",   "Defensive",
                         "High Pressing", "Possession", "Counter Attack"};
  const char* keys[] = {"career_tactic_attack",     "career_tactic_balance",
                        "career_tactic_defend",     "career_tactic_press",
                        "career_tactic_possession", "career_tactic_counter"};
  for (int i = 0; i < 6; ++i)
    if (strategy == names[i])
      return TR(keys[i]);
  return strategy;
}

static int GetHubPageID() {
  return CareerHubPageID(CareerDatabase::GetInstance().GetActiveSave());
}

// ---------------------------------------------------------------------------
// CareerMenuPage
// ---------------------------------------------------------------------------

CareerMenuPage::CareerMenuPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData),
      pageCreatedTime_ms(EnvironmentManager::GetInstance().GetTime_ms()),
      autoAdvanceTriggered(false) {
  Gui2Frame* bgPanel = new Gui2Frame(windowManager, "bg_career_menu", 3, 3, 94, 94, true);
  this->AddView(bgPanel);
  bgPanel->Show();
  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_career", 3, 2, 88, 3, TR("career_menu_title"));
  bgPanel->AddView(title);
  title->Show();

  Gui2Caption* subtitle =
      new Gui2Caption(windowManager, "caption_career_sub", 3, 6, 88, 4, TR("career_menu_subtitle"));
  bgPanel->AddView(subtitle);
  subtitle->Show();

  const bool continueFailed =
      pageData.properties && pageData.properties->GetBool("continueFailed", false);
  if (continueFailed) {
    Gui2Caption* failLine = new Gui2Caption(windowManager, "caption_career_continue_fail", 3, 10,
                                            88, 3, TR("career_menu_continue_failed"));
    bgPanel->AddView(failLine);
    failLine->Show();
  }

  CareerDatabase::GetInstance().Initialize("user/career");
  const bool hasSave = CareerDatabase::GetInstance().HasSaveFile();

  Gui2Frame* modesFrame = new Gui2Frame(windowManager, "frame_career_modes", 3, 15, 53, 67, true);
  bgPanel->AddView(modesFrame);
  modesFrame->Show();

  Gui2Caption* modesTitle = new Gui2Caption(windowManager, "caption_career_modes_title", 2, 2, 49,
                                            3, TR("career_menu_choose_path"));
  modesFrame->AddView(modesTitle);
  modesTitle->Show();

  Gui2Button* btnOwner =
      new Gui2Button(windowManager, "btn_ownercareer", 0, 0, 48, 8, TR("career_menu_owner_gm"));
  Gui2Button* btnCoach =
      new Gui2Button(windowManager, "btn_mycoach", 0, 0, 48, 8, TR("career_menu_coach"));
  Gui2Button* btnPlayer =
      new Gui2Button(windowManager, "btn_playercareer", 0, 0, 48, 8, TR("career_menu_player"));
  btnOwner->sig_OnClick.connect([this](...) { GoOwnerCareer(); });
  btnCoach->sig_OnClick.connect([this](...) { GoMyCoach(); });
  btnPlayer->sig_OnClick.connect([this](...) { GoPlayerCareer(); });
  Gui2Grid* grid = new Gui2Grid(windowManager, "career_grid", 2, 7, 48, 56);
  grid->AddView(btnOwner, 0, 0);
  grid->AddView(btnCoach, 1, 0);
  grid->AddView(btnPlayer, 2, 0);
  grid->UpdateLayout(0.5f, 0.5f, 0.75f, 0.75f);

  modesFrame->AddView(grid);
  grid->Show();

  Gui2Frame* saveFrame = new Gui2Frame(windowManager, "frame_career_save", 59, 15, 32, 67, true);
  bgPanel->AddView(saveFrame);
  saveFrame->Show();

  Gui2Caption* saveTitle = new Gui2Caption(windowManager, "caption_career_save_title", 2, 2, 28, 3,
                                           TR("career_menu_saved_career"));
  saveFrame->AddView(saveTitle);
  saveTitle->Show();

  CareerPersistence::CareerSaveSummary summary;
  bool hasSummary = CareerDatabase::GetInstance().GetSlotSummary(0, summary);
  if (!hasSummary) {
    hasSummary = CareerDatabase::GetInstance().GetSlotSummary(-1, summary);
  }

  std::string saveInfoStr;
  if (hasSummary && summary.isValid) {
    saveInfoStr = summary.clubName + "\n" + "Season " + std::to_string(summary.season) + " (Week " +
                  std::to_string(summary.week) + ")\n" + "Manager: " + summary.managerName + "\n" +
                  "Budget: " + FormatCareerMoney(summary.transferBudget) + "\n" +
                  "Trust: " + std::to_string(summary.boardConfidence) + "%";
    if (!summary.timestamp.empty()) {
      saveInfoStr += "\nSaved: " + summary.timestamp;
    }
  } else {
    saveInfoStr = TR(hasSave ? "career_menu_save_ready" : "career_menu_save_empty");
  }

  Gui2Caption* saveStatus =
      new Gui2Caption(windowManager, "caption_career_save_status", 2, 6, 28, 12, saveInfoStr);
  saveFrame->AddView(saveStatus);
  saveStatus->Show();

  Gui2Button* btnContinue =
      new Gui2Button(windowManager, "btn_continue", 2, 19, 28, 5,
                     TR(hasSave ? "career_menu_continue" : "career_menu_continue_empty"));
  btnContinue->sig_OnClick.connect([this](...) { GoContinueCareer(); });
  btnContinue->SetActive(hasSave);
  saveFrame->AddView(btnContinue);
  btnContinue->Show();

  Gui2Button* btnLoadSlots = new Gui2Button(windowManager, "btn_career_load_slots", 2, 25, 28, 5,
                                            TR("career_menu_load_slots"));
  btnLoadSlots->sig_OnClick.connect([this](...) {
    Properties props;
    props.Set("fromMenu", "true");
    CreatePage(e_PageID_CareerSave, props);
  });
  saveFrame->AddView(btnLoadSlots);
  btnLoadSlots->Show();

  Gui2Caption* footer = new Gui2Caption(windowManager, "caption_career_footer", 2, 32, 28, 15,
                                        TR("career_menu_footer"));
  saveFrame->AddView(footer);
  footer->Show();

  Gui2Button* btnBack = new Gui2Button(windowManager, "btn_career_menu_back", 2, 55, 28, 5,
                                       TR("career_menu_back_main"));
  btnBack->sig_OnClick.connect([this](...) { CreatePage(e_PageID_MainMenu); });
  saveFrame->AddView(btnBack);
  btnBack->Show();

  if (hasSave && !continueFailed)
    btnContinue->SetFocus();
  else
    btnOwner->SetFocus();
  this->Show();
}

CareerMenuPage::~CareerMenuPage() {}

void CareerMenuPage::Process() {
  Gui2Page::Process();
  if (!autoAdvanceTriggered && GetConfiguration()->GetBool("menu_smoke_test_career", false) &&
      EnvironmentManager::GetInstance().GetTime_ms() >=
          pageCreatedTime_ms + kCareerMenuSmokeDelay_ms) {
    autoAdvanceTriggered = true;
    printf("[menu-smoke] Career mode menu reached successfully\n");
    GetMenuTask()->QuitGame();
  }
}

void CareerMenuPage::GoContinueCareer() {
  CareerDatabase::GetInstance().Initialize("user/career");
  bool loaded = false;
  if (CareerDatabase::GetInstance().HasSaveFile()) {
    if (CareerDatabase::GetInstance().GetActiveSave()) {
      std::string careerName = CareerDatabase::GetInstance().GetActiveSave()->name;
      if (!careerName.empty())
        loaded = CareerDatabase::GetInstance().LoadCareerSave(careerName);
    }
    if (!loaded)
      loaded = CareerDatabase::GetInstance().LoadCareerSave("save");
  }
  if (loaded && CareerDatabase::GetInstance().GetActiveSave()) {
    CreatePage(GetHubPageID());
    return;
  }
  Properties props;
  props.SetBool("continueFailed", true);
  CreatePage(e_PageID_CareerMenu, props);
}

void CareerMenuPage::GoCareerMode(const std::string& mode) {
  Properties props;
  props.Set("careerMode", mode);
  CreatePage(e_PageID_CareerNewGame, props);
}

void CareerMenuPage::GoMyCoach() {
  GoCareerMode("coach");
}
void CareerMenuPage::GoPlayerCareer() {
  GoCareerMode("player");
}
void CareerMenuPage::GoOwnerCareer() {
  GoCareerMode("owner_gm");
}

// ---------------------------------------------------------------------------
// CareerNewGamePage
// ---------------------------------------------------------------------------

CareerNewGamePage::CareerNewGamePage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* bgPanel = new Gui2Frame(windowManager, "bg_career_new", 8, 7, 84, 86, true);
  this->AddView(bgPanel);
  bgPanel->Show();
  m_mode = pageData.properties ? pageData.properties->Get("careerMode", "owner_gm") : "owner_gm";

  CareerMode selectedRole = CareerMode::OWNER_GM;
  bool handsOn = true;
  DecodeCareerRole(m_mode, selectedRole, handsOn, true);
  m_mode = CareerModeKey(selectedRole);
  std::string modeLabel = TR(CareerModeLabelKey(selectedRole));

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_newgame", 4, 3, 76, 3,
                                       TRF("career_new_mode_title", {modeLabel}));
  bgPanel->AddView(title);
  title->Show();

  Gui2Caption* setupHint = new Gui2Caption(windowManager, "caption_newgame_hint", 4, 7, 76, 4,
                                           TR("career_new_mode_hint"));
  bgPanel->AddView(setupHint);
  setupHint->Show();

  Gui2Frame* formFrame = new Gui2Frame(windowManager, "frame_career_new_form", 4, 14, 76, 36, true);
  bgPanel->AddView(formFrame);
  formFrame->Show();

  Gui2Caption* teamCaption = new Gui2Caption(windowManager, "caption_newgame_team", 3, 5, 29, 3,
                                             TR("career_new_select_team"));
  formFrame->AddView(teamCaption);
  teamCaption->Show();

  teamSelectPulldown = new Gui2Pulldown(windowManager, "pulldown_career_teamselect", 32, 5, 40, 4);
  RefreshTeamSelect();
  teamSelectPulldown->sig_OnChange.connect(
      [this](Gui2Pulldown* pd) { m_selectedTeamID = pd->GetSelected(); });
  formFrame->AddView(teamSelectPulldown);
  teamSelectPulldown->Show();

  std::string nameFieldLabel = TR("career_new_mgr_name");
  std::string nameDefault = TR("career_mode_owner_gm");
  if (m_mode == "player") {
    nameFieldLabel = TR("career_new_player_name");
    nameDefault = TR("career_mode_player");

  } else if (m_mode == "coach") {
    nameFieldLabel = TR("career_new_coach_name");
    nameDefault = TR("career_mode_coach");
  } else if (m_mode == "owner_gm") {
    nameFieldLabel = TR("career_new_owner_name");
    nameDefault = TR("career_mode_owner_gm");
  }

  Gui2Caption* mgrCaption =
      new Gui2Caption(windowManager, "caption_newgame_mgr", 3, 16, 29, 3, nameFieldLabel);
  formFrame->AddView(mgrCaption);
  mgrCaption->Show();

  managerNameInput =
      new Gui2EditLine(windowManager, "editline_career_mgrname", 32, 16, 40, 4, nameDefault);
  managerNameInput->SetMaxLength(32);
  formFrame->AddView(managerNameInput);
  managerNameInput->Show();

  Gui2Button* btnStart =
      new Gui2Button(windowManager, "btn_start_career", 19, 57, 46, 5, TR("career_new_start"));
  btnStart->sig_OnClick.connect([this](...) { StartCareer(); });
  btnStart->SetActive(m_selectedTeamID != "0");
  bgPanel->AddView(btnStart);
  btnStart->Show();

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_newgame_back", 19, 65, 46, 4, TR("career_new_back_modes"));
  btnBack->sig_OnClick.connect([this](...) { CreatePage(e_PageID_CareerMenu); });
  bgPanel->AddView(btnBack);
  btnBack->Show();

  if (m_selectedTeamID != "0")
    teamSelectPulldown->SetFocus();
  else
    btnBack->SetFocus();

  this->Show();
}

CareerNewGamePage::~CareerNewGamePage() {}

void CareerNewGamePage::RefreshTeamSelect() {
  teamSelectPulldown->ClearEntries();
  bool foundTeam = false;
  try {
    auto result = GetDB()->Query(
        "SELECT teams.id, teams.name, leagues.name FROM teams "
        "JOIN leagues ON teams.league_id = leagues.id ORDER BY leagues.name, teams.name");
    for (unsigned int r = 0; r < result->data.size(); r++) {
      std::string id = result->data.at(r).at(0);
      std::string teamName = result->data.at(r).at(1);
      std::string leagueName = result->data.at(r).at(2);
      teamSelectPulldown->AddEntry(teamName + " (" + leagueName + ")", id);
      foundTeam = true;
    }
  } catch (...) {
  }
  if (!foundTeam)
    teamSelectPulldown->AddEntry(TR("career_new_no_teams"), "0");
  teamSelectPulldown->SetSelected(0);
  // Pulldown OnChange only fires on user input -- seed the selected ID from the
  // first entry so Start Career never launches with an unset team.
  m_selectedTeamID = teamSelectPulldown->GetSelected();
  if (m_selectedTeamID.empty())
    m_selectedTeamID = "0";
}

static std::string RoleToCareerPos(e_PlayerRole role) {
  return GetRoleName(role);
}

static int ComputePlayerOVR(PlayerData* pd) {
  const char* statNames[] = {"physical_balance",
                             "physical_reaction",
                             "physical_acceleration",
                             "physical_velocity",
                             "physical_stamina",
                             "physical_agility",
                             "physical_shotpower",
                             "technical_standingtackle",
                             "technical_slidingtackle",
                             "technical_ballcontrol",
                             "technical_dribble",
                             "technical_shortpass",
                             "technical_highpass",
                             "technical_header",
                             "technical_shot",
                             "technical_volley",
                             "mental_calmness",
                             "mental_workrate",
                             "mental_resilience",
                             "mental_defensivepositioning",
                             "mental_offensivepositioning",
                             "mental_vision"};
  float total = 0.0f;
  int count = 0;
  for (const char* name : statNames) {
    total += pd->GetStat(name);
    count++;
  }
  return count > 0 ? static_cast<int>((total / count) * 100.0f) : 50;
}

void CareerNewGamePage::StartCareer() {
  int teamDBID = atoi(m_selectedTeamID.c_str());

  std::string teamName = TR("career_new_unknown");
  std::string leagueName = TR("career_new_unknown");
  try {
    auto result = GetDB()->Query(
        "SELECT teams.name, leagues.name FROM teams "
        "JOIN leagues ON teams.league_id = leagues.id WHERE teams.id = " +
        int_to_str(teamDBID));
    if (!result->data.empty()) {
      teamName = result->data.at(0).at(0);
      leagueName = result->data.at(0).at(1);
    }
  } catch (...) {
  }

  CareerDatabase::GetInstance().Initialize("user/career");
  CareerDatabase::GetInstance().CreateNewCareer(teamName, m_mode, managerNameInput->GetText());

  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
  if (save) {
    save->club.clubID = teamDBID;
    save->club.leagueName = leagueName;

    save->roster.clear();
    TeamData teamData(teamDBID);
    const auto& players = teamData.GetPlayerData();
    for (const auto& pd : players) {
      int ovr = ComputePlayerOVR(pd.get());
      int pot = std::min(99, ovr + static_cast<int>(random(3, 20)));
      int age = 22;
      const auto& roles = pd->GetRoles();
      std::string pos = roles.empty() ? "CM" : RoleToCareerPos(roles[0]);
      try {
        auto playerResult = GetDB()->Query("SELECT age, role FROM players WHERE id = " +
                                           int_to_str(pd->GetDatabaseID()));
        if (!playerResult->data.empty()) {
          age = atoi(playerResult->data.at(0).at(0).c_str());
          pot = std::min(99, ovr + static_cast<int>((99 - age) * 0.5));
          // The database stores Football Manager notation ("D C", "AM RLC");
          // the 3D role parser only recognises GK/DM, so map it here.
          pos = CareerCommon::PositionFromDbRole(playerResult->data.at(0).at(1));
        }
      } catch (...) {
      }

      long long value = static_cast<long long>(ovr) * static_cast<long long>(ovr) * 5000;
      long long wage = (value / 1000) + static_cast<int>(random(500, 2000));

      PlayerCareerState cp;
      cp.name = pd->GetFirstName().empty() ? pd->GetLastName()
                                           : pd->GetFirstName() + " " + pd->GetLastName();
      cp.position = pos;
      cp.preferredPosition = pos;
      cp.ovr = ovr;
      cp.pot = pot;
      cp.age = age;
      cp.value = value;
      cp.wage = wage;
      cp.databaseID = pd->GetDatabaseID();
      cp.contract.yearsRemaining = static_cast<int>(random(2, 5));
      cp.contract.wage = wage;
      save->roster.push_back(cp);
    }

    long long totalWage = 0;
    for (const auto& p : save->roster)
      totalWage += p.wage;
    save->wageBudget = totalWage * 130 / 100;
    save->transferBudget = 15000000;

    if (m_mode == "owner_gm") {
      save->mode = CareerMode::OWNER_GM;
      save->transferBudget = 60000000;
      save->wageBudget = totalWage * 150 / 100;
    }

    if (m_mode == "player") {
      PlayerCareerState pro;
      pro.name = managerNameInput->GetText();
      if (pro.name.empty())
        pro.name = "My Pro";
      pro.position = "CF";
      pro.preferredPosition = "CF";
      pro.ovr = 65;
      pro.pot = 92;
      pro.age = 18;
      pro.value = 1500000;
      pro.wage = 2500;
      pro.databaseID = 99999;
      pro.contract.yearsRemaining = 3;
      pro.role = ClubRole::PROSPECT;
      save->roster.push_back(pro);
      save->controlledEntityID = pro.databaseID;
    }

    // The free-agent pool was seeded before the club was known; reseed it so
    // the user's own players are excluded.
    save->freeAgents.clear();
    CareerTransfers::SeedFreeAgents(*save);

    // CreateNewCareer saved a club-less placeholder; persist the real club,
    // roster and budgets so Continue does not load an unplayable career.
    CareerDatabase::GetInstance().SaveCareerData();
    CareerDatabase::GetInstance().AutoSave();
  }

  CreatePage(GetHubPageID());
}

// ---------------------------------------------------------------------------
// CareerHubPage
// ---------------------------------------------------------------------------
CareerHubPage::CareerHubPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  auto& db = CareerDatabase::GetInstance();
  const auto* save = db.GetActiveSave();
  const int section =
      pageData.properties ? atoi(pageData.properties->Get("careerSection", "0").c_str()) : 0;
  if (section >= 0 && section <= 4)
    m_section = static_cast<CareerHubSection>(section);
  auto* root = new Gui2Frame(windowManager, "career_shared_hub", 3, 3, 94, 94, true);
  AddView(root);
  root->Show();
  auto text = [&](Gui2View* parent, const std::string& id, float x, float y, float w, float h,
                  const std::string& body, float font = 2.2f) {
    auto* view = new Gui2Text(windowManager, id, x, y, w, h, font, 60, body);
    view->SetSize(w, h);
    parent->AddView(view);
    view->Show();
    return view;
  };
  text(root, "hub_identity", 2, 1, 88, 5,
       save ? TR(CareerModeLabelKey(save->mode)) + " | " + save->name : TR("career_hub_title"),
       2.8f);
  text(root, "hub_calendar", 2, 7, 88, 5,
       save ? TRF("career_hub_identity",
                  {save->managerName, std::to_string(save->season.currentSeason),
                   std::to_string(std::min(save->season.currentWeek, save->season.maxWeeks)),
                   std::to_string(save->season.maxWeeks)})
            : TR("career_nosave"));
  auto* nav = new Gui2Grid(windowManager, "hub_navigation", 2, 15, 28, 74);
  root->AddView(nav);
  int row = 0;
  auto navButton = [&](const std::string& id, const std::string& label,
                       const std::function<void()>& click) {
    auto* button = new Gui2Button(windowManager, id, 0, 0, 26, 5.5f, label);
    button->sig_OnClick.connect([click](...) { click(); });
    nav->AddView(button, row++, 0);
    return button;
  };
  auto* next = navButton("hub_continue",
                         TR(db.CanAdvanceSeason() ? "career_nav_review" : "career_nav_continue"),
                         [this] { GoMatchday(); });
  next->SetActive(save != nullptr);
  const char* ids[] = {"overview", "team", "role", "competitions", "inbox"};
  const char* keys[] = {"career_nav_overview", "career_nav_team",
                        save ? CareerWorkspaceKey(save->mode) : "career_hub_title",
                        "career_nav_competitions", "career_nav_inbox"};
  Gui2Button* selectedTab = nullptr;
  for (int i = 0; i < 5; ++i) {
    const auto target = static_cast<CareerHubSection>(i);
    auto* tab =
        navButton(std::string("hub_nav_") + ids[i], (target == m_section ? "> " : "") + TR(keys[i]),
                  [this, target] { OpenSection(target); });
    tab->SetActive(save != nullptr);
    if (target == m_section)
      selectedTab = tab;
  }
  navButton("hub_save", TR("career_hub_btn_save_load"), [this] {
    Properties props;
    props.Set("fromMenu", "false");
    CreatePage(e_PageID_CareerSave, props);
  });
  auto* exit = navButton("hub_exit", TR("career_menu_back_modes"), [this] {
    auto& database = CareerDatabase::GetInstance();
    if (database.GetActiveSave() && !database.SaveCareerData()) {
      m_feedback->SetCaption(TR("career_save_retry"));
      return;
    }
    database.AutoSave();
    CreatePage(e_PageID_CareerMenu);
  });
  nav->UpdateLayout(0.5f, 0.5f, 0.5f, 0.5f);
  nav->Show();
  auto* content = new Gui2Frame(windowManager, "hub_content", 32, 15, 60, 74, true);
  root->AddView(content);
  content->Show();
  text(content, "hub_section_title", 2, 2, 56, 5, TR(keys[static_cast<int>(m_section)]), 2.7f);
  m_feedback = new Gui2Caption(windowManager, "hub_feedback", 2, 90, 88, 3, "");
  root->AddView(m_feedback);
  m_feedback->Show();
  if (!save) {
    text(content, "hub_empty", 2, 10, 56, 15, TR("career_nosave"));
    exit->SetFocus();
    Show();
    return;
  }
  int fitness = 0, injured = 0, unread = 0;
  for (const auto& p : save->roster) {
    fitness += p.fitness;
    injured += p.injury != InjuryStatus::HEALTHY ? 1 : 0;
  }
  if (!save->roster.empty())
    fitness /= static_cast<int>(save->roster.size());
  for (const auto& item : save->inbox)
    unread += item.read ? 0 : 1;
  const auto* pro = ControlledCareerPlayer(*save);
  const std::string teamBody =
      TRF("career_hub_team_summary",
          {std::to_string(save->roster.size()), std::to_string(fitness), std::to_string(injured),
           CareerStrategyLabel(save->activeStrategy), std::to_string(save->trainingPoints)});
  std::string roleBody;
  if (CanManageClub(*save)) {
    roleBody = TRF("career_hub_office_summary",
                   {FormatCareerMoney(save->transferBudget), FormatCareerMoney(save->wageBudget),
                    std::to_string(save->boardConfidence), std::to_string(save->staff.size()),
                    std::to_string(save->availableSponsorOffers.size())});
  } else if (save->mode == CareerMode::COACH) {
    roleBody = teamBody + "\n\n" + TR("career_hub_coach_focus");
  } else if (pro) {
    roleBody = TRF("career_hub_pro_summary",
                   {pro->name, pro->position, std::to_string(pro->ovr), std::to_string(pro->pot),
                    std::to_string(pro->fitness), std::to_string(pro->careerGoals),
                    std::to_string(pro->careerAssists),
                    std::to_string(pro->contract.yearsRemaining), FormatCareerMoney(pro->wage)});
  } else {
    roleBody = TR("career_hub_missing_pro");
  }
  if (m_section == CareerHubSection::OVERVIEW) {
    const std::string progress =
        db.CanAdvanceSeason()
            ? TR("career_hub_season_ready")
            : TRF("career_hub_next_week", {std::to_string(save->season.currentWeek),
                                           std::to_string(save->season.maxWeeks)});
    text(content, "hub_next_step", 2, 10, 56, 10, progress);
    text(content, "hub_role_snapshot", 2, 22, 56, 32, roleBody);
    text(content, "hub_inbox_summary", 2, 58, 56, 8,
         TRF("career_hub_unread", {std::to_string(unread)}));
  } else if (m_section == CareerHubSection::INBOX) {
    m_unreadSummary = text(content, "hub_inbox_summary", 2, 9, 56, 6,
                           TRF("career_hub_unread", {std::to_string(unread)}));
    auto* messages = new Gui2Grid(windowManager, "hub_messages", 2, 18, 56, 52);
    m_messages = messages;
    content->AddView(messages);
    int index = 0;
    for (const auto& item : save->inbox) {
      auto* button = new Gui2Button(windowManager, "hub_message_" + std::to_string(index), 0, 0, 54,
                                    6, (item.read ? "" : "* ") + item.subject);
      button->sig_OnClick.connect(
          [this, index](...) { OpenInboxItem(static_cast<size_t>(index)); });
      messages->AddView(button, index++, 0);
    }
    if (save->inbox.empty())
      text(content, "hub_inbox_empty", 2, 20, 56, 16, TR("career_hub_inbox_empty"));
    messages->SetMaxVisibleRows(7);
    messages->UpdateLayout(0.5f, 0.5f, 0.5f, 0.5f);
    messages->Show();
  } else {
    std::string body = roleBody;
    if (m_section == CareerHubSection::TEAM) {
      body = teamBody;
      if (save->mode == CareerMode::OWNER_GM && !CanManageTeam(*save))
        body += "\n\n" + TR("career_hub_delegated_team");
    } else if (m_section == CareerHubSection::COMPETITIONS) {
      body = TRF("career_hub_competition_summary",
                 {save->club.leagueName, std::to_string(save->seasonWins),
                  std::to_string(save->seasonDraws), std::to_string(save->seasonLosses),
                  std::to_string(save->seasonWins * 3 + save->seasonDraws),
                  std::to_string(save->history.size())});
    }
    text(content, "hub_section_body", 2, 9, 56, 28, body);
    auto* actions = new Gui2Grid(windowManager, "hub_actions", 2, 39, 56, 32);
    content->AddView(actions);
    int actionRow = 0;
    for (const auto& tool : CareerHubTools(*save, m_section)) {
      auto* button = new Gui2Button(windowManager, std::string("hub_action_") + tool.id, 0, 0, 54,
                                    4.5f, TR(tool.label));
      button->sig_OnClick.connect([this, tool](...) { OpenTool(tool.action); });
      actions->AddView(button, actionRow++, 0);
    }
    actions->SetMaxVisibleRows(6);
    actions->UpdateLayout(0.5f, 0.5f, 0.25f, 0.25f);
    actions->Show();
  }
  if (selectedTab && pageData.properties &&
      pageData.properties->Get("focusCareerTab", "false") == "true")
    selectedTab->SetFocus();
  else
    next->SetFocus();
  Show();
}

CareerHubPage::~CareerHubPage() {}

void CareerHubPage::OpenSection(CareerHubSection section) {
  Properties props;
  props.Set("careerSection", std::to_string(static_cast<int>(section)));
  props.Set("focusCareerTab", "true");
  CreatePage(e_PageID_CareerHub, props);
}

void CareerHubPage::OpenTool(CareerHubAction action) {
  auto& db = CareerDatabase::GetInstance();
  auto* save = db.GetActiveSave();
  if (!save)
    return;
  const auto tools = CareerHubTools(*save, m_section);
  if (std::none_of(tools.begin(), tools.end(),
                   [&](const CareerHubTool& t) { return t.action == action; }))
    return;
  using A = CareerHubAction;
  int page = -1;
  switch (action) {
    case A::SQUAD:
      page = e_PageID_CareerSquadRoster;
      break;
    case A::STRATEGY:
      page = e_PageID_CareerStrategy;
      break;
    case A::TRAINING:
      page = e_PageID_CareerTraining;
      break;
    case A::TRANSFERS:
      page = e_PageID_CareerTransferMarket;
      break;
    case A::FREE_AGENTS:
      page = e_PageID_CareerFreeAgency;
      break;
    case A::YOUTH:
      page = e_PageID_CareerYouthAcademy;
      break;
    case A::FINANCES:
      page = e_PageID_OwnerFinances;
      break;
    case A::STADIUM:
      page = e_PageID_OwnerStadium;
      break;
    case A::STAFF:
      page = e_PageID_OwnerStaff;
      break;
    case A::SPONSORS:
      page = e_PageID_OwnerSponsors;
      break;
    case A::BOARD:
      page = e_PageID_OwnerBoardRoom;
      break;
    case A::PRESS:
      page = e_PageID_CareerPressConference;
      break;
    case A::STANDINGS:
      page = e_PageID_CareerStandings;
      break;
    case A::SEASON:
      page = e_PageID_CareerSeason;
      break;
    case A::EXPANSION:
      page = e_PageID_CareerLeagueExpansion;
      break;
    case A::CUSTOM:
      page = e_PageID_CareerCustomLeague;
      break;
    case A::RESPONSIBILITIES:
      if (db.SetHandsOnManagement(!save->handsOnManagement))
        OpenSection(m_section);
      else
        m_feedback->SetCaption(TR("career_save_retry"));
      return;
    case A::REQUEST_TRANSFER:
      if (const auto* player = ControlledCareerPlayer(*save)) {
        if (db.ToggleTransferList(player->name))
          OpenSection(m_section);
        else
          m_feedback->SetCaption(TR("career_save_retry"));
      }
      return;
  }
  if (page >= 0)
    CreatePage(page);
}

void CareerHubPage::OpenInboxItem(size_t index) {
  auto& db = CareerDatabase::GetInstance();
  auto* save = db.GetActiveSave();
  if (!save || index >= save->inbox.size())
    return;
  const auto item = save->inbox[index];
  auto* dialog = new Gui2Dialog(windowManager, "hub_inbox_dialog", 16, 20, 68, 60, item.subject);
  auto* body = new Gui2Text(windowManager, "hub_message_body", 0, 0, 60, 35, 2.2f, 60, item.body);
  dialog->AddContent(body);
  auto* close = dialog->AddSingleButton(TR("career_close"));
  auto dismiss = [this, dialog, index](...) {
    dialog->Exit();
    delete dialog;
    if (m_messages)
      if (auto* message = m_messages->FindView(static_cast<int>(index), 0))
        message->SetFocus();
  };
  dialog->sig_OnPositive.connect(dismiss);
  dialog->sig_OnNegative.connect(dismiss);
  AddView(dialog);
  dialog->Show();
  close->SetFocus();
  if (!item.read) {
    save->inbox[index].read = true;
    if (!db.SaveCareerData()) {
      save->inbox[index].read = false;
      m_feedback->SetCaption(TR("career_inbox_save_failed"));
    } else {
      m_feedback->SetCaption("");
      if (m_messages)
        if (auto* message =
                dynamic_cast<Gui2Button*>(m_messages->FindView(static_cast<int>(index), 0)))
          message->SetCaption(item.subject);
      if (m_unreadSummary) {
        const auto unread = std::count_if(save->inbox.begin(), save->inbox.end(),
                                          [](const auto& message) { return !message.read; });
        m_unreadSummary->ClearText();
        m_unreadSummary->AddText(TRF("career_hub_unread", {std::to_string(unread)}));
        m_unreadSummary->SetSize(56, 6);
      }
    }
  }
}

void CareerHubPage::GoMatchday() {
  CreatePage(CareerDatabase::GetInstance().CanAdvanceSeason() ? e_PageID_CareerSeason
                                                              : e_PageID_CareerMatchday);
}

// ---------------------------------------------------------------------------
// CareerTransferMarketPage
// ---------------------------------------------------------------------------

CareerTransferMarketPage::CareerTransferMarketPage(Gui2WindowManager* windowManager,
                                                   const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* bgPanel = new Gui2Frame(windowManager, "bg_career_tm", 0, 0, 100, 100, true);
  this->AddView(bgPanel);
  bgPanel->Show();
  CareerDatabase::GetInstance().PopulateTransferMarket();

  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
  std::string budgetStr = save ? TRF("career_tm_budget", {FormatCareerMoney(save->transferBudget)})
                               : TR("career_nosave");

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_tm_title", 6, 3, 82, 3, TR("career_tm_title"));
  bgPanel->AddView(title);
  title->Show();

  Gui2Caption* budget = new Gui2Caption(windowManager, "caption_tm_budget", 6, 7, 82, 2, budgetStr);
  bgPanel->AddView(budget);
  budget->Show();

  Gui2Caption* marketHint =
      new Gui2Caption(windowManager, "caption_tm_hint", 6, 9, 82, 2, TR("career_tm_hint"));
  bgPanel->AddView(marketHint);
  marketHint->Show();

  Gui2Caption* header =
      new Gui2Caption(windowManager, "caption_tm_header", 3, 12, 94, 2, TR("career_tm_header"));
  bgPanel->AddView(header);
  header->Show();

  auto targets = CareerDatabase::GetInstance().GetTransferTargets();
  Gui2Grid* grid = new Gui2Grid(windowManager, "grid_tm", 3, 15, 94, 58);
  int row = 0;
  Gui2Button* firstTargetButton = nullptr;
  for (const auto& t : targets) {
    if (row >= 18)
      break;
    const std::string rowLabel =
        TRF("career_tm_row", {t.name, t.preferredPosition, std::to_string(t.overallRating),
                              std::to_string(t.potentialRating), std::to_string(t.age),
                              FormatCareerMoney(t.value), FormatCareerMoney(t.askingPrice)});
    Gui2Button* btn =
        new Gui2Button(windowManager, "btn_tm_" + std::to_string(row), 0, 0, 90, 2.5, rowLabel);
    if (!firstTargetButton)
      firstTargetButton = btn;
    btn->sig_OnClick.connect([this, t](...) {
      Properties props;
      props.Set("playerName", t.name);
      props.Set("askingPrice", std::to_string(t.askingPrice));
      props.Set("playerWage", std::to_string(t.wage));
      CreatePage(e_PageID_CareerTransferBidDetail, props);
    });
    grid->AddView(btn, row++, 0);
  }
  grid->SetMaxVisibleRows(15);
  grid->UpdateLayout(0.5);
  bgPanel->AddView(grid);
  grid->Show();

  if (targets.empty()) {
    Gui2Caption* empty =
        new Gui2Caption(windowManager, "caption_tm_empty", 6, 18, 82, 4, TR("career_tm_empty"));
    bgPanel->AddView(empty);
    empty->Show();
  }

  Gui2Button* btnBids =
      new Gui2Button(windowManager, "btn_tm_mybids", 5, 80, 40, 3, TR("career_tm_mybids"));
  btnBids->sig_OnClick.connect([this](...) { CreatePage(e_PageID_CareerTransferBids); });
  bgPanel->AddView(btnBids);
  btnBids->Show();

  Gui2Button* btnProcess =
      new Gui2Button(windowManager, "btn_tm_process", 50, 80, 40, 3, TR("career_tm_process"));
  btnProcess->sig_OnClick.connect([this](...) {
    CareerDatabase::GetInstance().ProcessPendingBids();
    CreatePage(e_PageID_CareerTransferBids);
  });
  bgPanel->AddView(btnProcess);
  btnProcess->Show();

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_tm_back", 30, 90, 40, 3, TR("career_back_hub"));
  btnBack->sig_OnClick.connect([this](...) { CreatePage(GetHubPageID()); });
  bgPanel->AddView(btnBack);
  btnBack->Show();
  if (firstTargetButton)
    firstTargetButton->SetFocus();
  else
    btnBids->SetFocus();

  this->Show();
}

CareerTransferMarketPage::~CareerTransferMarketPage() {}

// ---------------------------------------------------------------------------
// CareerTransferBidsPage
// ---------------------------------------------------------------------------

CareerTransferBidsPage::CareerTransferBidsPage(Gui2WindowManager* windowManager,
                                               const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* bgPanel = new Gui2Frame(windowManager, "bg_career_bids", 0, 0, 100, 100, true);
  this->AddView(bgPanel);
  bgPanel->Show();
  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_bids_title", 6, 3, 82, 3, TR("career_bids_title"));
  bgPanel->AddView(title);
  title->Show();

  auto& bids = CareerDatabase::GetInstance().GetActiveBids();
  Gui2Button* firstActionButton = nullptr;
  if (bids.empty()) {
    Gui2Caption* info = new Gui2Caption(windowManager, "caption_bids_empty", 10, 20, 80, 4,
                                        TR("career_bids_empty"));
    bgPanel->AddView(info);
    info->Show();
  } else {
    Gui2Caption* header = new Gui2Caption(windowManager, "caption_bids_header", 5, 10, 90, 2,
                                          TR("career_bids_header"));
    bgPanel->AddView(header);
    header->Show();

    Gui2Grid* grid = new Gui2Grid(windowManager, "grid_bids", 5, 13, 90, 62);
    int row = 0;
    for (const auto& b : bids) {
      if (row >= 18)
        break;
      const std::string rowLabel =
          TRF("career_bids_row", {b.playerName, FormatCareerMoney(b.bidAmount),
                                  FormatCareerMoney(b.offeredWage), std::to_string(b.contractYears),
                                  CareerDatabase::GetInstance().GetBidStatusString(b.status)});
      Gui2Button* btn =
          new Gui2Button(windowManager, "btn_bid_" + std::to_string(row), 0, 0, 86, 2.5, rowLabel);
      if (b.status == BidStatus::ACCEPTED) {
        std::string pName = b.playerName;
        btn->sig_OnClick.connect([this, pName](...) {
          CareerDatabase::GetInstance().CompleteTransfer(pName);
          CreatePage(e_PageID_CareerTransferBids);
        });
        if (!firstActionButton)
          firstActionButton = btn;
      } else if (b.status == BidStatus::PENDING) {
        std::string pName = b.playerName;
        btn->sig_OnClick.connect([this, pName](...) { NegotiateBid(pName); });
        if (!firstActionButton)
          firstActionButton = btn;
      } else {
        btn->SetActive(false);
      }
      grid->AddView(btn, row++, 0);
    }
    grid->UpdateLayout(0.5);
    bgPanel->AddView(grid);
    grid->Show();

    if (bids.size() > 18) {
      Gui2Caption* more =
          new Gui2Caption(windowManager, "caption_bids_more", 5, 77, 90, 2,
                          TRF("career_bids_showmore", {std::to_string(bids.size() - 18)}));
      bgPanel->AddView(more);
      more->Show();
    }
  }

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_bids_back", 5, 82, 40, 3, TR("career_back_market"));
  btnBack->sig_OnClick.connect([this](...) { CreatePage(e_PageID_CareerTransferMarket); });
  bgPanel->AddView(btnBack);
  btnBack->Show();

  Gui2Button* btnHub =
      new Gui2Button(windowManager, "btn_bids_hub", 50, 82, 40, 3, TR("career_back_hub"));
  btnHub->sig_OnClick.connect([this](...) { CreatePage(GetHubPageID()); });
  bgPanel->AddView(btnHub);
  btnHub->Show();
  if (firstActionButton)
    firstActionButton->SetFocus();
  else
    btnBack->SetFocus();

  this->Show();
}

CareerTransferBidsPage::~CareerTransferBidsPage() {}

void CareerTransferBidsPage::NegotiateBid(const std::string& playerName) {
  auto& bids = CareerDatabase::GetInstance().GetActiveBids();
  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
  for (auto& b : bids) {
    if (b.playerName == playerName && b.status == BidStatus::PENDING) {
      if (save)
        CareerTransfers::ImprovePendingBid(b, save->transferBudget);
      break;
    }
  }
  CreatePage(e_PageID_CareerTransferBids);
}

// ---------------------------------------------------------------------------
// CareerTransferBidDetailPage
// ---------------------------------------------------------------------------

CareerTransferBidDetailPage::CareerTransferBidDetailPage(Gui2WindowManager* windowManager,
                                                         const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* bgPanel = new Gui2Frame(windowManager, "bg_career_biddtl", 4, 2, 92, 96, true);
  this->AddView(bgPanel);
  bgPanel->Show();
  m_playerName = pageData.properties ? pageData.properties->Get("playerName", "") : "";
  m_askingPrice =
      pageData.properties ? atoll(pageData.properties->Get("askingPrice", "0").c_str()) : 0;
  m_playerWage =
      pageData.properties ? atoll(pageData.properties->Get("playerWage", "0").c_str()) : 0;

  auto targets = CareerDatabase::GetInstance().GetTransferTargets();
  TransferTarget target;
  bool found = false;
  Gui2Button* preferredBidButton = nullptr;
  for (const auto& t : targets) {
    if (t.name == m_playerName) {
      target = t;
      found = true;
      break;
    }
  }

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_detail_title", 4, 2, 84, 3,
                                       "TRANSFER NEGOTIATIONS: " + m_playerName);
  title->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright1));
  bgPanel->AddView(title);
  title->Show();

  if (found) {
    // Scouting Report & Agent Card
    Gui2Frame* profileFrame = new Gui2Frame(windowManager, "frame_bid_prof", 4, 6, 84, 15, true);
    Gui2Caption* profTitle = new Gui2Caption(windowManager, "cap_bid_proftitle", 2, 1, 80, 2,
                                             "SCOUTING PROFILE & AGENT INTELLIGENCE");
    profTitle->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
    profileFrame->AddView(profTitle);
    profTitle->Show();

    char profBuf[512];
    snprintf(
        profBuf, sizeof(profBuf),
        "Position: %s | Overall: %d | Potential: %d | Age: %d\n"
        "Market Valuation: %s | Asking Price: %s | Desired Wage: %s/wk\n"
        "Agent Patience: [||||||||  ] 80%% | Target Interest: HIGH | Contract Desired: 3 Years",
        target.preferredPosition.c_str(), target.overallRating, target.potentialRating, target.age,
        FormatCareerMoney(target.value).c_str(), FormatCareerMoney(target.askingPrice).c_str(),
        FormatCareerMoney(target.wage).c_str());
    Gui2Caption* profBody =
        new Gui2Caption(windowManager, "cap_bid_profbody", 2, 4, 80, 10, std::string(profBuf));
    profileFrame->AddView(profBody);
    profBody->Show();
    bgPanel->AddView(profileFrame);
    profileFrame->Show();

    // Offers Grid
    Gui2Grid* grid = new Gui2Grid(windowManager, "grid_detail", 6, 23, 80, 36);

    long long askPrice = target.askingPrice;
    Gui2Button* bidFull = new Gui2Button(windowManager, "btn_bid_full", 0, 0, 76, 3.2f,
                                         "Meet Asking Price (" + FormatCareerMoney(askPrice) +
                                             ") -- Guaranteed Acceptance [Key Player]");
    bidFull->sig_OnClick.connect([this, askPrice](...) { PlaceBidForPlayer(askPrice); });
    preferredBidButton = bidFull;
    grid->AddView(bidFull, 0, 0);

    long long bid85 = target.askingPrice * 85 / 100;
    Gui2Button* bid85Button = new Gui2Button(
        windowManager, "btn_bid_85", 0, 0, 76, 3.2f,
        "Negotiated Offer (" + FormatCareerMoney(bid85) + ") -- 85% Price [First Team Role]");
    bid85Button->sig_OnClick.connect([this, bid85](...) { PlaceBidForPlayer(bid85); });
    grid->AddView(bid85Button, 1, 0);

    long long bid70 = target.askingPrice * 70 / 100;
    Gui2Button* bid70Button = new Gui2Button(
        windowManager, "btn_bid_70", 0, 0, 76, 3.2f,
        "Lowball Counter (" + FormatCareerMoney(bid70) + ") -- 70% Price [High Risk of Rejection]");
    bid70Button->sig_OnClick.connect([this, bid70](...) { PlaceBidForPlayer(bid70); });
    grid->AddView(bid70Button, 2, 0);

    grid->UpdateLayout(0.5f, 0.5f, 0.25f, 0.25f);
    bgPanel->AddView(grid);
    grid->Show();

    CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
    if (save) {
      long long agentFee = target.askingPrice / 20;
      long long totalWithFee = target.askingPrice + agentFee;
      std::string budgetStr =
          "Financial Breakdown: +5% Agent Fee (" + FormatCareerMoney(agentFee) + ") = Total " +
          FormatCareerMoney(totalWithFee) +
          " | Club Budget Remaining: " + FormatCareerMoney(save->transferBudget);
      if (totalWithFee > save->transferBudget) {
        budgetStr += "\nWARNING: Proposed offer exceeds current club transfer funds!";
      }
      Gui2Caption* fee =
          new Gui2Caption(windowManager, "caption_detail_fee", 4, 62, 84, 5, budgetStr);
      fee->SetColor(totalWithFee > save->transferBudget
                        ? windowManager->GetStyle()->GetColor(e_DecorationType_Dark1)
                        : windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
      bgPanel->AddView(fee);
      fee->Show();
    }
  }

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_detail_back", 26, 85, 40, 3, "Return to Transfer Market");
  btnBack->sig_OnClick.connect([this](...) { CreatePage(e_PageID_CareerTransferMarket); });
  bgPanel->AddView(btnBack);
  btnBack->Show();
  if (preferredBidButton)
    preferredBidButton->SetFocus();
  else
    btnBack->SetFocus();

  this->Show();
}

CareerTransferBidDetailPage::~CareerTransferBidDetailPage() {}

void CareerTransferBidDetailPage::PlaceBidForPlayer(long long amount) {
  TransferBid bid = CareerDatabase::GetInstance().PlaceBid(m_playerName, amount,
                                                           static_cast<int>(m_playerWage), 3);
  if (bid.status == BidStatus::REJECTED) {
    Gui2Caption* warn =
        new Gui2Caption(windowManager, "caption_bid_warn", 10, 78, 80, 3,
                        "The selling club and agent have immediately rejected this valuation!");
    this->AddView(warn);
    warn->Show();
  } else {
    CreatePage(e_PageID_CareerTransferBids);
  }
}

// ---------------------------------------------------------------------------
// CareerPressConferencePage
// ---------------------------------------------------------------------------

CareerPressConferencePage::CareerPressConferencePage(Gui2WindowManager* windowManager,
                                                     const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* bgPanel = new Gui2Frame(windowManager, "bg_career_press", 4, 2, 92, 96, true);
  this->AddView(bgPanel);
  bgPanel->Show();
  auto addText = [&](const char* id, float y, float height, const std::string& value) {
    auto* text = new Gui2Text(windowManager, id, 4, y, 84, height, 2.4f, 65, value);
    text->SetSize(84, height);
    bgPanel->AddView(text);
    text->Show();
  };
  addText("caption_pressconf", 3, 5, TR("career_media_title"));
  addText("caption_pc_question", 13, 16, TR("career_media_question"));
  addText("caption_pc_answer_hint", 31, 8, TR("career_media_hint"));
  auto* btnPositive =
      new Gui2Button(windowManager, "btn_pc_positive", 0, 0, 76, 6, TR("career_media_positive"));
  auto* btnNeutral =
      new Gui2Button(windowManager, "btn_pc_neutral", 0, 0, 76, 6, TR("career_media_neutral"));
  auto* btnNegative =
      new Gui2Button(windowManager, "btn_pc_negative", 0, 0, 76, 6, TR("career_media_negative"));
  btnPositive->sig_OnClick.connect([this](...) { SelectAnswer(0); });
  btnNeutral->sig_OnClick.connect([this](...) { SelectAnswer(1); });
  btnNegative->sig_OnClick.connect([this](...) { SelectAnswer(2); });

  Gui2Grid* grid = new Gui2Grid(windowManager, "pc_grid", 6, 43, 76, 42);
  grid->AddView(btnPositive, 0, 0);
  grid->AddView(btnNeutral, 1, 0);
  grid->AddView(btnNegative, 2, 0);
  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_pc_back", 0, 0, 76, 3, TR("career_back_hub"));
  btnBack->sig_OnClick.connect([this](...) { CreatePage(GetHubPageID()); });
  grid->AddView(btnBack, 3, 0);
  grid->UpdateLayout(0.5f, 0.5f, 0.25f, 0.25f);

  bgPanel->AddView(grid);
  grid->Show();

  btnPositive->SetFocus();
  this->Show();
}

CareerPressConferencePage::~CareerPressConferencePage() {}

void CareerPressConferencePage::SelectAnswer(int answerIndex) {
  int delta = m_reputationDeltas[answerIndex];
  CareerDatabase::GetInstance().AddEvent("press_conference",
                                         delta > 0   ? TR("career_event_press_pos")
                                         : delta < 0 ? TR("career_event_press_neg")
                                                     : TR("career_event_press_neutral"),
                                         delta, false);
  if (delta > 0) {
    CareerDatabase::GetInstance().ModifyBoardConfidence(1);
  } else if (delta < 0) {
    CareerDatabase::GetInstance().ModifyBoardConfidence(-2);
  }
  CreatePage(GetHubPageID());
}

// ---------------------------------------------------------------------------
// CareerLeagueExpansionPage
// ---------------------------------------------------------------------------

CareerLeagueExpansionPage::CareerLeagueExpansionPage(Gui2WindowManager* windowManager,
                                                     const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* bgPanel = new Gui2Frame(windowManager, "bg_career_exp", 4, 2, 92, 96, true);
  this->AddView(bgPanel);
  bgPanel->Show();
  Gui2Caption* title = new Gui2Caption(windowManager, "caption_leagueexp", 6, 3, 82, 3,
                                       TR("career_leagueexp_title"));
  bgPanel->AddView(title);
  title->Show();

  CareerSave* activeSave = CareerDatabase::GetInstance().GetActiveSave();
  if (activeSave) {
    std::string currentConfig =
        TRF("career_leagueexp_status",
            {std::to_string(activeSave->leagueSettings.divisions.size()),
             activeSave->leagueSettings.enabled ? TR("career_enabled") : TR("career_disabled")});
    Gui2Caption* statusLine =
        new Gui2Caption(windowManager, "caption_leagueexp_status", 6, 8, 82, 2, currentConfig);
    bgPanel->AddView(statusLine);
    statusLine->Show();
  }

  Gui2Frame* infoFrame = new Gui2Frame(windowManager, "frame_exp_info", 6, 12, 84, 16, true);
  Gui2Caption* infoBody = new Gui2Caption(windowManager, "caption_leagueexp_body", 2, 2, 80, 12,
                                          TR("career_leagueexp_body"));
  infoFrame->AddView(infoBody);
  infoBody->Show();
  bgPanel->AddView(infoFrame);
  infoFrame->Show();

  Gui2Grid* grid = new Gui2Grid(windowManager, "leagueexp_grid", 10, 32, 76, 40);
  Gui2Button* btnEnable = new Gui2Button(windowManager, "btn_leagueexp_enable", 0, 0, 34, 5,
                                         TR("career_leagueexp_enable"));
  Gui2Button* btnDisable = new Gui2Button(windowManager, "btn_leagueexp_disable", 0, 0, 34, 5,
                                          TR("career_leagueexp_disable"));
  Gui2Button* btnAddDiv = new Gui2Button(windowManager, "btn_leagueexp_adddiv", 0, 0, 34, 5,
                                         TR("career_leagueexp_adddiv"));

  btnEnable->sig_OnClick.connect([this](...) { EnableRelegation(); });
  btnDisable->sig_OnClick.connect([this](...) { DisableRelegation(); });
  btnAddDiv->sig_OnClick.connect([this](...) { AddDivision(); });

  grid->AddView(btnEnable, 0, 0);
  grid->AddView(btnDisable, 0, 1);
  grid->AddView(btnAddDiv, 1, 0);
  grid->UpdateLayout(0.5);

  bgPanel->AddView(grid);
  grid->Show();

  if (activeSave && activeSave->leagueSettings.enabled) {
    Gui2Frame* divFrame = new Gui2Frame(windowManager, "frame_exp_divs", 6, 60, 84, 18, true);
    Gui2Caption* divTitle = new Gui2Caption(windowManager, "caption_exp_divlist", 2, 1, 80, 2,
                                            TR("career_leagueexp_divisions"));
    divFrame->AddView(divTitle);
    divTitle->Show();
    int divY = 4;
    for (int i = 0; i < static_cast<int>(activeSave->leagueSettings.divisions.size()); i++) {
      const auto& div = activeSave->leagueSettings.divisions[i];
      std::string divLine =
          TRF("career_leagueexp_divline",
              {std::to_string(i + 1), div.name, std::to_string(div.numTeams),
               std::to_string(div.promotionSpots), std::to_string(div.relegationSpots)});
      Gui2Caption* divCap = new Gui2Caption(windowManager, "caption_exp_div_" + std::to_string(i),
                                            2, divY, 80, 2, divLine);
      divFrame->AddView(divCap);
      divCap->Show();
      divY += 2;
    }
    bgPanel->AddView(divFrame);
    divFrame->Show();
  }

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_exp_back", 30, 88, 30, 3, TR("career_back_hub"));
  btnBack->sig_OnClick.connect([this](...) { CreatePage(GetHubPageID()); });
  bgPanel->AddView(btnBack);
  btnBack->Show();

  btnEnable->SetFocus();
  this->Show();
}

CareerLeagueExpansionPage::~CareerLeagueExpansionPage() {}
void CareerLeagueExpansionPage::EnableRelegation() {
  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
  if (save && CanManageClub(*save)) {
    save->leagueSettings.enabled = true;
    if (save->leagueSettings.divisions.empty()) {
      save->leagueSettings.divisions.push_back({"Premier Division", 20, 3, 3, 0});
      save->leagueSettings.divisions.push_back({"Second Division", 20, 3, 3, 0});
    }
    CareerDatabase::GetInstance().SaveCareerData();
  }
  CreatePage(e_PageID_CareerLeagueExpansion);
}
void CareerLeagueExpansionPage::DisableRelegation() {
  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
  if (save && CanManageClub(*save)) {
    save->leagueSettings.enabled = false;
    CareerDatabase::GetInstance().SaveCareerData();
  }
  CreatePage(e_PageID_CareerLeagueExpansion);
}
void CareerLeagueExpansionPage::AddDivision() {
  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
  if (save && CanManageClub(*save)) {
    int divNum = static_cast<int>(save->leagueSettings.divisions.size()) + 1;
    save->leagueSettings.divisions.push_back({"Division " + std::to_string(divNum), 20, 3, 3, 0});
    save->leagueSettings.enabled = true;
    CareerDatabase::GetInstance().SaveCareerData();
  }
  CreatePage(e_PageID_CareerLeagueExpansion);
}

// ---------------------------------------------------------------------------
// CareerCustomLeaguePage
// ---------------------------------------------------------------------------

CareerCustomLeaguePage::CareerCustomLeaguePage(Gui2WindowManager* windowManager,
                                               const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* bgPanel = new Gui2Frame(windowManager, "bg_career_cust", 4, 2, 92, 96, true);
  this->AddView(bgPanel);
  bgPanel->Show();
  Gui2Caption* title = new Gui2Caption(windowManager, "caption_customleague", 6, 3, 82, 3,
                                       TR("career_customleague_title"));
  bgPanel->AddView(title);
  title->Show();

  Gui2Frame* infoFrame = new Gui2Frame(windowManager, "frame_cust_info", 6, 10, 84, 18, true);
  Gui2Caption* infoBody = new Gui2Caption(windowManager, "caption_customleague_body", 2, 2, 80, 14,
                                          TR("career_customleague_body"));
  infoFrame->AddView(infoBody);
  infoBody->Show();
  bgPanel->AddView(infoFrame);
  infoFrame->Show();

  CareerSave* activeSave = CareerDatabase::GetInstance().GetActiveSave();
  if (activeSave) {
    Gui2Caption* current = new Gui2Caption(
        windowManager, "caption_cust_current", 6, 32, 82, 3,
        TRF("career_customleague_current",
            {activeSave->customLeague.leagueName.empty() ? TR("career_default_league")
                                                         : activeSave->customLeague.leagueName,
             std::to_string(activeSave->customLeague.numDivisions)}));
    bgPanel->AddView(current);
    current->Show();
  }

  Gui2Grid* grid = new Gui2Grid(windowManager, "cust_grid", 12, 40, 72, 30);
  Gui2Button* btnCreate = new Gui2Button(windowManager, "btn_customleague_create", 0, 0, 34, 5,
                                         TR("career_customleague_create"));
  Gui2Button* btnReset = new Gui2Button(windowManager, "btn_customleague_reset", 0, 0, 34, 5,
                                        TR("career_customleague_reset"));
  btnCreate->sig_OnClick.connect([this](...) { CreateCustomLeague(); });
  btnReset->sig_OnClick.connect([this](...) {
    CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
    if (save && CanManageClub(*save)) {
      save->customLeague = CustomLeagueConfig();
      CareerDatabase::GetInstance().SaveCareerData();
    }
    CreatePage(e_PageID_CareerCustomLeague);
  });
  grid->AddView(btnCreate, 0, 0);
  grid->AddView(btnReset, 0, 1);
  grid->UpdateLayout(0.5);
  bgPanel->AddView(grid);
  grid->Show();

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_cust_back", 30, 88, 30, 3, TR("career_back_hub"));
  btnBack->sig_OnClick.connect([this](...) { CreatePage(GetHubPageID()); });
  bgPanel->AddView(btnBack);
  btnBack->Show();

  btnCreate->SetFocus();

  this->Show();
}

CareerCustomLeaguePage::~CareerCustomLeaguePage() {}
void CareerCustomLeaguePage::CreateCustomLeague() {
  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
  if (save && CanManageClub(*save)) {
    save->customLeague.leagueName = TR("career_custom_league_name");
    save->customLeague.numDivisions = 2;
    CareerDatabase::GetInstance().SaveCareerData();
  }
  CreatePage(e_PageID_CareerCustomLeague);
}

// ---------------------------------------------------------------------------
// CareerFreeAgencyPage
// ---------------------------------------------------------------------------

CareerFreeAgencyPage::CareerFreeAgencyPage(Gui2WindowManager* windowManager,
                                           const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* bgPanel = new Gui2Frame(windowManager, "bg_career_fa", 5, 0, 90, 100, true);
  this->AddView(bgPanel);
  bgPanel->Show();
  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_freeagency", 10, 5, 80, 3, TR("career_fa_title"));
  this->AddView(title);
  title->Show();

  CareerSave* activeSave = CareerDatabase::GetInstance().GetActiveSave();
  Gui2Button* firstRecruitButton = nullptr;
  if (activeSave) {
    Gui2Caption* summary =
        new Gui2Caption(windowManager, "caption_fa_summary", 10, 10, 80, 3,
                        TRF("career_fa_summary", {FormatCareerMoney(activeSave->wageBudget),
                                                  std::to_string(activeSave->roster.size())}));
    this->AddView(summary);
    summary->Show();

    Gui2Grid* grid = new Gui2Grid(windowManager, "fa_grid", 10, 16, 80, 64);
    int row = 0;
    if (activeSave->freeAgents.empty()) {
      Gui2Caption* empty =
          new Gui2Caption(windowManager, "caption_fa_empty", 10, 20, 80, 4, TR("career_fa_empty"));
      this->AddView(empty);
      empty->Show();
    } else {
      for (const PlayerCareerState& fa : activeSave->freeAgents) {
        if (row >= 16)
          break;
        std::string label =
            TRF("career_fa_label", {fa.name, std::to_string(fa.ovr), FormatCareerMoney(fa.wage)});
        Gui2Button* btn = new Gui2Button(windowManager, "btn_recruit_" + fa.name, 0, 0, 76, 3,
                                         TRF("career_fa_recruit", {label}));
        if (!firstRecruitButton)
          firstRecruitButton = btn;
        btn->sig_OnClick.connect([this, fa](...) { RecruitPlayer(fa.name); });
        grid->AddView(btn, row++, 0);
      }
      grid->UpdateLayout(0.5);
      this->AddView(grid);
      grid->Show();

      if (activeSave->freeAgents.size() > 16) {
        Gui2Caption* more = new Gui2Caption(
            windowManager, "caption_fa_more", 10, 82, 80, 2,
            TRF("career_fa_showmore", {std::to_string(activeSave->freeAgents.size() - 16)}));
        this->AddView(more);
        more->Show();
      }
    }
  }

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_fa_back", 30, 90, 40, 3, TR("career_back_hub"));
  btnBack->sig_OnClick.connect([this](...) { CreatePage(GetHubPageID()); });
  this->AddView(btnBack);
  btnBack->Show();
  if (firstRecruitButton)
    firstRecruitButton->SetFocus();
  else
    btnBack->SetFocus();

  this->Show();
}

CareerFreeAgencyPage::~CareerFreeAgencyPage() {}

void CareerFreeAgencyPage::RecruitPlayer(const std::string& playerName) {
  CareerDatabase::GetInstance().RecruitFreeAgent(playerName);
  CreatePage(e_PageID_CareerFreeAgency);
}

// ---------------------------------------------------------------------------
// CareerTrainingPage
// ---------------------------------------------------------------------------

CareerTrainingPage::CareerTrainingPage(Gui2WindowManager* windowManager,
                                       const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  auto* background = new Gui2Frame(windowManager, "bg_career_train", 5, 0, 90, 100, true);
  AddView(background);
  background->Show();
  auto caption = [&](const std::string& id, float x, float y, float width,
                     const std::string& text) {
    auto* label = new Gui2Caption(windowManager, id, x, y, width, 3, text);
    AddView(label);
    label->Show();
  };
  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
  if (save && save->mode == CareerMode::PLAYER) {
    caption("training_title", 10, 5, 80, TR("career_nav_personal_training"));
    const auto* pro = ControlledCareerPlayer(*save);
    std::string details = TR("career_hub_missing_pro");
    if (pro) {
      details = pro->name + "\n\n" +
                TRF("career_development_stats",
                    {std::to_string(pro->ovr), std::to_string(pro->pot),
                     std::to_string(pro->developmentPoints), std::to_string(pro->fitness)});
    }
    auto* summary =
        new Gui2Text(windowManager, "personal_training_summary", 10, 15, 80, 30, 2.5f, 60, details);
    summary->SetSize(80, 30);
    AddView(summary);
    summary->Show();
    caption("session_points", 10, 50, 80,
            TRF("career_training_points", {std::to_string(save->trainingPoints)}));
    Gui2Button* train = nullptr;
    if (pro) {
      train = new Gui2Button(windowManager, "training_session_0", 10, 60, 80, 5,
                             TR("career_train_individual"));
      train->SetActive(save->trainingPoints > 0);
      train->sig_OnClick.connect([this](...) { TrainFocus("Individual"); });
      AddView(train);
      train->Show();
    }
    auto* back = new Gui2Button(windowManager, "btn_tr_back", 30, 90, 40, 3, TR("career_back_hub"));
    back->sig_OnClick.connect([this](...) { CreatePage(GetHubPageID()); });
    AddView(back);
    back->Show();
    if (train && save->trainingPoints > 0)
      train->SetFocus();
    else
      back->SetFocus();
    Show();
    return;
  }
  caption("training_title", 10, 5, 80, TR("career_training_title"));
  caption("plan_hint", 10, 10, 80, TR("career_plan_hint"));
  auto* plans = new Gui2Grid(windowManager, "training_plans", 10, 16, 80, 4);
  const char* planKeys[] = {"career_plan_balanced", "career_plan_development",
                            "career_plan_recovery"};
  for (int i = 0; i < 3; ++i) {
    const auto plan = static_cast<CareerTrainingPlan>(i);
    std::string label = TR(planKeys[i]);
    if (save && save->trainingPlan == plan)
      label = "[ " + label + " ]";
    auto* button =
        new Gui2Button(windowManager, "training_plan_" + std::to_string(i), 0, 0, 25, 3, label);
    button->SetActive(save && CanManageTeam(*save));
    button->sig_OnClick.connect([this, plan](...) {
      if (CareerDatabase::GetInstance().SetTrainingPlan(plan))
        CreatePage(e_PageID_CareerTraining);
    });
    plans->AddView(button, 0, i);
  }
  plans->UpdateLayout(1);
  AddView(plans);
  plans->Show();
  const char* detailKeys[] = {"career_plan_balanced_detail", "career_plan_development_detail",
                              "career_plan_recovery_detail"};
  int selected = save ? static_cast<int>(save->trainingPlan) : 0;
  if (selected < 0 || selected > 2)
    selected = 0;
  caption("plan_detail", 10, 23, 80, TR(detailKeys[selected]));
  caption("development_hint", 10, 28, 80, TR("career_development_hint"));
  caption("development_support", 10, 31, 80, TR("career_development_support"));
  caption("session_hint", 10, 85, 80, TR("career_session_hint"));
  caption("session_points", 10, 35, 34,
          TRF("career_training_points", {std::to_string(save ? save->trainingPoints : 0)}));
  caption("progress_title", 48, 35, 42, TR("career_development_title"));
  auto* sessions = new Gui2Grid(windowManager, "training_sessions", 10, 41, 34, 40);
  const char* focuses[] = {"General", "Attacking", "Defending", "Physical", "Tactical", "Shooting"};
  const char* keys[] = {"career_train_general",  "career_train_attacking", "career_train_defending",
                        "career_train_physical", "career_train_tactical",  "career_train_shooting"};
  for (int i = 0; i < (IsPlayerMode() ? 1 : 6); ++i) {
    std::string focus = IsPlayerMode() ? "Individual" : focuses[i];
    auto* button = new Gui2Button(windowManager, "training_session_" + std::to_string(i), 0, 0, 32,
                                  3, TR(IsPlayerMode() ? "career_train_individual" : keys[i]));
    button->SetActive(save && save->trainingPoints > 0);
    button->sig_OnClick.connect([this, focus](...) {
      if (focus == "General")
        TrainSquad();
      else
        TrainFocus(focus);
    });
    sessions->AddView(button, i, 0);
  }
  sessions->UpdateLayout(1);
  AddView(sessions);
  sessions->Show();
  auto* progress = new Gui2Grid(windowManager, "development_players", 48, 41, 42, 42);
  progress->SetMaxVisibleRows(6);
  progress->SetReadOnlyScrolling(true);
  int row = 0;
  auto addPlayer = [&](const PlayerCareerState& player, bool academy) {
    const std::string id = "development_" + std::to_string(row);
    auto* card = new Gui2Frame(windowManager, id, 0, 0, 40, 5.5, false);
    auto* name = new Gui2Caption(windowManager, id + "_name", 0, 0, 40, 2.5,
                                 (academy ? TR("career_academy_short") + " " : "") + player.name);
    auto* stats = new Gui2Caption(
        windowManager, id + "_stats", 0, 3, 40, 2.5,
        TRF("career_development_stats",
            {std::to_string(player.ovr), std::to_string(player.pot),
             std::to_string(player.developmentPoints), std::to_string(player.fitness)}));
    card->AddView(name);
    name->Show();
    card->AddView(stats);
    stats->Show();
    progress->AddView(card, row++, 0);
  };
  if (save) {
    for (const auto& player : save->roster)
      addPlayer(player, false);
    for (const auto& player : save->youthAcademy)
      addPlayer(player, true);
  }
  progress->UpdateLayout(0.5);
  AddView(progress);
  progress->Show();
  auto* back = new Gui2Button(windowManager, "btn_tr_back", 30, 90, 40, 3, TR("career_back_hub"));
  back->sig_OnClick.connect([this](...) { CreatePage(GetHubPageID()); });
  AddView(back);
  back->Show();
  if (save)
    plans->SetFocus();
  else
    back->SetFocus();
  Show();
}

CareerTrainingPage::~CareerTrainingPage() {}

void CareerTrainingPage::TrainSquad() {
  if (CareerDatabase::GetInstance().TrainSquad()) {
    CreatePage(e_PageID_CareerTraining);
  }
}

void CareerTrainingPage::TrainFocus(const std::string& focusArea) {
  if (CareerDatabase::GetInstance().TrainFocus(focusArea)) {
    CreatePage(e_PageID_CareerTraining);
  }
}

// ---------------------------------------------------------------------------
// CareerStrategyPage
// ---------------------------------------------------------------------------

CareerStrategyPage::CareerStrategyPage(Gui2WindowManager* windowManager,
                                       const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* bgPanel = new Gui2Frame(windowManager, "bg_career_strat", 5, 0, 90, 100, true);
  this->AddView(bgPanel);
  bgPanel->Show();
  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_strategy", 10, 5, 80, 3, TR("career_nav_tactics"));
  this->AddView(title);
  title->Show();

  CareerSave* activeSave = CareerDatabase::GetInstance().GetActiveSave();
  std::string curStrat = activeSave ? activeSave->activeStrategy : TR("career_none");

  Gui2Caption* info =
      new Gui2Caption(windowManager, "caption_curstrat", 10, 15, 80, 3,
                      TRF("career_tactic_current", {CareerStrategyLabel(curStrat)}));
  this->AddView(info);
  info->Show();

  Gui2Caption* hint =
      new Gui2Caption(windowManager, "caption_curstrat_hint", 10, 19, 80, 3,
                      TR(activeSave && CanManageTeam(*activeSave) ? "career_tactic_hint"
                                                                  : "career_tactic_readonly"));
  this->AddView(hint);
  hint->Show();

  Gui2Grid* grid = new Gui2Grid(windowManager, "strat_grid", 20, 26, 60, 58);

  const char* strategyNames[] = {"Attacking",     "Balanced",   "Defensive",
                                 "High Pressing", "Possession", "Counter Attack"};
  const char* strategyKeys[] = {"career_tactic_attack",     "career_tactic_balance",
                                "career_tactic_defend",     "career_tactic_press",
                                "career_tactic_possession", "career_tactic_counter"};
  Gui2Button* selected = nullptr;
  for (int i = 0; i < 6; ++i) {
    auto* button = new Gui2Button(windowManager, "career_tactic_" + std::to_string(i), 0, 0, 60, 5,
                                  TR(strategyKeys[i]));
    button->SetToggleable(true);
    button->SetToggled(curStrat == strategyNames[i]);
    button->SetActive(activeSave && CanManageTeam(*activeSave));
    const std::string name = strategyNames[i];
    button->sig_OnClick.connect([this, name](...) { SetStrategy(name); });
    grid->AddView(button, i, 0);
    if (curStrat == name)
      selected = button;
  }
  grid->UpdateLayout(0.5);
  this->AddView(grid);
  grid->Show();

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_st_back", 30, 90, 40, 3, TR("career_back_hub"));
  btnBack->sig_OnClick.connect([this](...) { CreatePage(GetHubPageID()); });
  this->AddView(btnBack);
  btnBack->Show();
  if (activeSave && CanManageTeam(*activeSave)) {
    if (selected)
      selected->SetFocus();
    else
      grid->SetFocus();
  } else
    btnBack->SetFocus();

  this->Show();
}

CareerStrategyPage::~CareerStrategyPage() {}

void CareerStrategyPage::SetStrategy(const std::string& strategyName) {
  CareerDatabase::GetInstance().SetStrategy(strategyName);
  CreatePage(e_PageID_CareerStrategy);
}

// ---------------------------------------------------------------------------
// CareerYouthAcademyPage
// ---------------------------------------------------------------------------

CareerYouthAcademyPage::CareerYouthAcademyPage(Gui2WindowManager* windowManager,
                                               const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* bgPanel = new Gui2Frame(windowManager, "bg_career_ya", 5, 0, 90, 100, true);
  this->AddView(bgPanel);
  bgPanel->Show();
  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_youth", 10, 5, 80, 3, TR("career_youth_title"));
  this->AddView(title);
  title->Show();

  CareerSave* activeSave = CareerDatabase::GetInstance().GetActiveSave();
  Gui2Button* firstAcademyButton = nullptr;
  Gui2Button* btnScout = nullptr;
  if (activeSave) {
    Gui2Caption* budget = new Gui2Caption(
        windowManager, "caption_ya_budget", 10, 10, 80, 3,
        TRF("career_youth_budget", {FormatCareerMoney(activeSave->transferBudget),
                                    std::to_string(activeSave->youthAcademy.size())}));
    this->AddView(budget);
    budget->Show();

    Gui2Grid* grid = new Gui2Grid(windowManager, "ya_grid", 10, 16, 80, 59);
    int row = 0;

    int scoutCost = 50000 * activeSave->scoutingNetworkLevel;
    btnScout = new Gui2Button(windowManager, "btn_scout_youth", 0, 0, 76, 3,
                              TRF("career_youth_scout", {FormatCareerMoney(scoutCost)}));
    btnScout->SetActive(activeSave->transferBudget >= scoutCost);
    btnScout->sig_OnClick.connect([this](...) { ScoutPlayer(); });
    grid->AddView(btnScout, row++, 0);

    if (activeSave->youthAcademy.empty()) {
      Gui2Caption* empty =
          new Gui2Caption(windowManager, "caption_ya_empty", 0, 0, 76, 3, TR("career_youth_empty"));
      grid->AddView(empty, row++, 0);
    } else {
      int academyRows = 0;
      for (const PlayerCareerState& ya : activeSave->youthAcademy) {
        if (academyRows >= 13)
          break;
        std::string label =
            TRF("career_youth_player",
                {ya.name, std::to_string(ya.age), std::to_string(ya.ovr), std::to_string(ya.pot)});
        Gui2Button* btn = new Gui2Button(windowManager, "btn_promote_" + ya.name, 0, 0, 76, 3,
                                         TRF("career_youth_promote", {label}));
        if (!firstAcademyButton)
          firstAcademyButton = btn;
        btn->sig_OnClick.connect([this, ya](...) { PromotePlayer(ya.name); });
        grid->AddView(btn, row++, 0);
        academyRows++;
      }

      if (activeSave->youthAcademy.size() > 13) {
        Gui2Caption* more = new Gui2Caption(
            windowManager, "caption_ya_more", 10, 75, 80, 2,
            TRF("career_youth_showmore", {std::to_string(activeSave->youthAcademy.size() - 13)}));
        this->AddView(more);
        more->Show();
      }
    }
    grid->UpdateLayout(0.5);
    this->AddView(grid);
    grid->Show();
  }

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_ya_back", 30, 90, 40, 3, TR("career_back_hub"));
  btnBack->sig_OnClick.connect([this](...) { CreatePage(GetHubPageID()); });
  this->AddView(btnBack);
  btnBack->Show();
  if (btnScout && activeSave &&
      activeSave->transferBudget >= 50000LL * activeSave->scoutingNetworkLevel)
    btnScout->SetFocus();
  else if (firstAcademyButton)
    firstAcademyButton->SetFocus();
  else
    btnBack->SetFocus();

  this->Show();
}

CareerYouthAcademyPage::~CareerYouthAcademyPage() {}

void CareerYouthAcademyPage::ScoutPlayer() {
  CareerDatabase::GetInstance().ScoutYouthPlayer();
  CreatePage(e_PageID_CareerYouthAcademy);
}

void CareerYouthAcademyPage::PromotePlayer(const std::string& playerName) {
  CareerDatabase::GetInstance().PromoteYouthPlayer(playerName);
  CreatePage(e_PageID_CareerYouthAcademy);
}

// ---------------------------------------------------------------------------
// CareerSquadRosterPage
// ---------------------------------------------------------------------------

CareerSquadRosterPage::CareerSquadRosterPage(Gui2WindowManager* windowManager,
                                             const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* bgPanel = new Gui2Frame(windowManager, "bg_career_squad", 2, 1, 96, 98, true);
  this->AddView(bgPanel);
  bgPanel->Show();

  CareerSave* activeSave = CareerDatabase::GetInstance().GetActiveSave();
  std::string clubTitle =
      activeSave ? (activeSave->name + " / " + TR("career_nav_squad")) : TR("career_squad_title");

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_squad", 4, 2, 90, 3, clubTitle);
  title->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  this->AddView(title);
  title->Show();

  Gui2Button* firstPlayerButton = nullptr;
  if (activeSave) {
    long long totalWage = 0;
    for (const auto& p : activeSave->roster) {
      totalWage += p.wage;
    }

    auto* squadHint = new Gui2Caption(windowManager, "caption_squad_hint", 4, 8, 90, 2.5f,
                                      TR("career_roster_hint"));
    AddView(squadHint);
    squadHint->Show();
    Gui2Grid* grid = new Gui2Grid(windowManager, "squad_grid", 3, 11, 92, 70);
    m_rosterGrid = grid;
    int row = 0;
    grid->SetMaxVisibleRows(12);
    for (const auto& player : activeSave->roster) {
      const std::string rowLabel =
          TRF("career_roster_row", {player.name, player.position, std::to_string(player.ovr),
                                    std::to_string(player.pot), std::to_string(player.fitness)});
      auto* btn = new Gui2Button(windowManager, "btn_player_" + std::to_string(row), 0, 0, 88, 4.5f,
                                 rowLabel);
      if (!firstPlayerButton)
        firstPlayerButton = btn;

      btn->sig_OnClick.connect([this, player](...) { InspectPlayer(player); });
      grid->AddView(btn, row++, 0);
    }
    grid->UpdateLayout(0.5f, 0.5f, 0.2f, 0.2f);
    this->AddView(grid);
    grid->Show();

    std::string footerText =
        TRF("career_roster_totals",
            {std::to_string(activeSave->roster.size()), FormatCareerMoney(totalWage),
             FormatCareerMoney(activeSave->wageBudget)});
    Gui2Caption* footer =
        new Gui2Caption(windowManager, "caption_squad_footer", 4, 83, 90, 2.2f, footerText);
    this->AddView(footer);
    footer->Show();
  }

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_squad_back", 30, 88, 40, 3, TR("career_back_hub"));
  btnBack->sig_OnClick.connect([this](...) { CreatePage(GetHubPageID()); });
  this->AddView(btnBack);
  btnBack->Show();

  if (firstPlayerButton)
    firstPlayerButton->SetFocus();
  else
    btnBack->SetFocus();

  this->Show();
}

CareerSquadRosterPage::~CareerSquadRosterPage() {}

void CareerSquadRosterPage::InspectPlayer(const PlayerCareerState& player) {
  Gui2Dialog* dialog =
      new Gui2Dialog(windowManager, "dialog_inspect_player", 15, 15, 70, 70, player.name);

  const std::string details =
      TRF("career_hub_pro_summary",
          {player.name, player.position, std::to_string(player.ovr), std::to_string(player.pot),
           std::to_string(player.fitness), std::to_string(player.careerGoals),
           std::to_string(player.careerAssists), std::to_string(player.contract.yearsRemaining),
           FormatCareerMoney(player.wage)}) +
      "\n\n" +
      TRF("career_profile_extra",
          {std::to_string(player.age), FormatCareerMoney(player.value),
           std::to_string(player.morale), std::to_string(player.developmentPoints)});
  auto* profile = new Gui2Text(windowManager, "cap_player_detail", 4, 8, 62, 30, 2.4f, 55, details);
  profile->SetSize(62, 30);
  dialog->AddView(profile);
  profile->Show();

  Gui2Grid* actionGrid = new Gui2Grid(windowManager, "grid_player_actions", 4, 42, 62, 24);
  std::string pName = player.name;
  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
  int actRow = 0;

  if (save && CanManageTeam(*save)) {
    Gui2Button* btnMotivate = new Gui2Button(windowManager, "btn_motivate_player", 0, 0, 58, 2.5f,
                                             TR("career_profile_talk"));
    btnMotivate->sig_OnClick.connect([this, pName, dialog](...) {
      dialog->Exit();
      delete dialog;
      MotivatePlayer(pName);
    });
    actionGrid->AddView(btnMotivate, actRow++, 0);

    const int tp = save ? save->trainingPoints : 0;
    std::string drillLabel = TRF("career_profile_drill", {std::to_string(tp)});
    Gui2Button* btnDrill =
        new Gui2Button(windowManager, "btn_drill_player", 0, 0, 58, 2.5f, drillLabel);
    btnDrill->sig_OnClick.connect([this, pName, dialog](...) {
      dialog->Exit();
      delete dialog;
      DrillPlayer(pName);
    });
    btnDrill->SetActive(tp > 0);
    actionGrid->AddView(btnDrill, actRow++, 0);
  }
  if (save && CanManageClub(*save)) {
    Gui2Button* btnExtend = new Gui2Button(windowManager, "btn_extend_contract", 0, 0, 58, 2.5f,
                                           TR("career_profile_extend"));
    btnExtend->sig_OnClick.connect([this, pName, dialog](...) {
      dialog->Exit();
      delete dialog;
      ExtendContract(pName);
    });
    actionGrid->AddView(btnExtend, actRow++, 0);

    std::string listLabel =
        TR(player.contract.transferListed ? "career_profile_unlist" : "career_profile_list");
    Gui2Button* btnToggleList =
        new Gui2Button(windowManager, "btn_toggle_list", 0, 0, 58, 2.5f, listLabel);
    btnToggleList->sig_OnClick.connect([this, pName, dialog](...) {
      dialog->Exit();
      delete dialog;
      ToggleTransferList(pName);
    });
    actionGrid->AddView(btnToggleList, actRow++, 0);

    Gui2Button* btnRelease = new Gui2Button(windowManager, "btn_release_action", 0, 0, 58, 2.5f,
                                            TR("career_profile_release"));
    btnRelease->sig_OnClick.connect([this, pName, dialog](...) {
      dialog->Exit();
      delete dialog;
      ReleasePlayer(pName);
    });
    actionGrid->AddView(btnRelease, actRow++, 0);
  } else if (IsPlayerMode() && save && player.databaseID == save->controlledEntityID) {
    std::string listLabel = TR(player.contract.transferListed ? "career_nav_cancel_request"
                                                              : "career_nav_request_transfer");
    Gui2Button* btnToggleList =
        new Gui2Button(windowManager, "btn_toggle_list", 0, 0, 58, 2.5f, listLabel);
    btnToggleList->sig_OnClick.connect([this, pName, dialog](...) {
      dialog->Exit();
      delete dialog;
      ToggleTransferList(pName);
    });
    actionGrid->AddView(btnToggleList, actRow++, 0);
  }

  Gui2Button* btnClose =
      new Gui2Button(windowManager, "btn_close_profile", 0, 0, 58, 2.5f, TR("career_close"));
  auto* previousFocus = windowManager->GetFocus();
  auto dismiss = [dialog, previousFocus](...) {
    dialog->Exit();
    delete dialog;
    if (previousFocus)
      previousFocus->SetFocus();
  };
  btnClose->sig_OnClick.connect(dismiss);
  dialog->sig_OnPositive.connect(dismiss);
  dialog->sig_OnNegative.connect(dismiss);
  actionGrid->AddView(btnClose, actRow++, 0);

  actionGrid->UpdateLayout(0.5f, 0.5f, 0.2f, 0.2f);
  dialog->AddView(actionGrid);
  actionGrid->Show();

  btnClose->SetFocus();
  this->AddView(dialog);
  dialog->Show();
}

void CareerSquadRosterPage::ExtendContract(const std::string& playerName) {
  CareerDatabase::GetInstance().ExtendContract(playerName);
  CreatePage(e_PageID_CareerSquadRoster);
}

void CareerSquadRosterPage::ToggleTransferList(const std::string& playerName) {
  CareerDatabase::GetInstance().ToggleTransferList(playerName);
  CreatePage(e_PageID_CareerSquadRoster);
}

void CareerSquadRosterPage::ReleasePlayer(const std::string& playerName) {
  Gui2Dialog* dialog = new Gui2Dialog(windowManager, "dialog_release_player", 23, 34, 54, 30,
                                      TRF("career_release_confirm", {playerName}));
  Gui2Button* confirm = dialog->AddPosNegButtons(TR("career_release_action"), TR("action_cancel"));
  confirm->SetFocus();
  dialog->sig_OnPositive.connect([this, playerName](...) {
    CareerDatabase::GetInstance().ReleasePlayer(playerName);
    CreatePage(e_PageID_CareerSquadRoster);
  });
  dialog->sig_OnNegative.connect([this, dialog](...) {
    dialog->Exit();
    delete dialog;
    if (m_rosterGrid && m_rosterGrid->GetSelectedView())
      m_rosterGrid->GetSelectedView()->SetFocus();
  });
  this->AddView(dialog);
  dialog->Show();
}

void CareerSquadRosterPage::MotivatePlayer(const std::string& playerName) {
  CareerDatabase::GetInstance().MotivatePlayer(playerName);
  CreatePage(e_PageID_CareerSquadRoster);
}

void CareerSquadRosterPage::DrillPlayer(const std::string& playerName) {
  CareerDatabase::GetInstance().DrillPlayer(playerName);
  CreatePage(e_PageID_CareerSquadRoster);
}

// ---------------------------------------------------------------------------
// CareerSeasonPage
// ---------------------------------------------------------------------------

CareerSeasonPage::CareerSeasonPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* bgPanel = new Gui2Frame(windowManager, "bg_career_season", 4, 2, 92, 96, true);
  this->AddView(bgPanel);
  bgPanel->Show();
  Gui2Caption* title = new Gui2Caption(
      windowManager, "caption_season", 6, 4, 80, 3,
      TR(IsOwnerMode() ? "career_season_review_title" : "career_end_of_season_title"));
  bgPanel->AddView(title);
  title->Show();

  CareerSave* activeSave = CareerDatabase::GetInstance().GetActiveSave();
  if (activeSave) {
    Gui2Caption* info = new Gui2Caption(
        windowManager, "caption_season_info", 6, 8, 82, 2,
        TRF("career_season_info", {std::to_string(activeSave->season.currentSeason),
                                   std::to_string(activeSave->boardConfidence),
                                   CareerDatabase::GetInstance().GetReputationStatus()}));
    bgPanel->AddView(info);
    info->Show();

    Gui2Frame* summaryFrame =
        new Gui2Frame(windowManager, "frame_season_summary", 4, 11, 84, 14, true);
    std::string summary =
        TRF("career_season_summary_mode", {GetCareerModeDisplay(activeSave)}) + "\n" +
        TRF("career_season_summary_budgets", {FormatCareerMoney(activeSave->transferBudget),
                                              FormatCareerMoney(activeSave->wageBudget)}) +
        "\n" +
        TRF("career_season_summary_squad", {std::to_string(activeSave->roster.size()),
                                            std::to_string(activeSave->youthAcademy.size())});
    if (activeSave->mode == CareerMode::OWNER_GM) {
      summary += "\n" + TRF("career_season_summary_owner",
                            {FormatCareerMoney(activeSave->finances.netWorth),
                             FormatCareerMoney(CareerDatabase::GetInstance().GetSeasonProfit())});
    }

    int estPos = CareerSim::LeaguePosition(*activeSave);
    long long prizeMoney = CareerSim::CalculateSeasonPrizeMoney(estPos);
    summary += "\n" + TRF("career_season_prize_money", {FormatCareerMoney(prizeMoney)});
    if (estPos == 1) {
      summary += "  |  " + TR("career_season_champions_banner");
    }

    int expiringContracts = 0;
    for (const auto& p : activeSave->roster) {
      if (p.contract.yearsRemaining <= 1) {
        expiringContracts++;
      }
    }
    if (expiringContracts > 0) {
      summary += "\n" + TRF("career_contracts_expiring_warn", {std::to_string(expiringContracts)});
    }

    Gui2Caption* summaryCap =
        new Gui2Caption(windowManager, "caption_season_summary", 2, 2, 80, 10, summary);
    summaryFrame->AddView(summaryCap);
    summaryCap->Show();
    bgPanel->AddView(summaryFrame);
    summaryFrame->Show();

    Gui2Caption* progress = new Gui2Caption(windowManager, "caption_season_progress", 6, 26, 82, 2,
                                            BuildSeasonProgressLine(activeSave));
    bgPanel->AddView(progress);
    progress->Show();

    const bool earlyAdvance = !CareerDatabase::GetInstance().CanAdvanceSeason();
    std::string warningText;
    if (earlyAdvance) {
      warningText = TRF("career_season_wait", {std::to_string(activeSave->season.currentWeek),
                                               std::to_string(activeSave->season.maxWeeks)});
    } else if (activeSave->mode == CareerMode::OWNER_GM) {
      warningText = TR("career_season_owner_proceed");
    } else {
      warningText = TR("career_season_proceed");
    }
    Gui2Caption* warning =
        new Gui2Caption(windowManager, "caption_season_warn", 6, 29, 82, 4, warningText);
    bgPanel->AddView(warning);
    warning->Show();

    if (activeSave->mode == CareerMode::OWNER_GM) {
      Gui2Frame* ownerFrame =
          new Gui2Frame(windowManager, "frame_season_owner", 4, 34, 84, 18, true);
      Gui2Caption* ownerTitle = new Gui2Caption(windowManager, "caption_season_owner_title", 2, 1,
                                                78, 2, TR("career_season_owner_checklist"));
      ownerFrame->AddView(ownerTitle);
      ownerTitle->Show();

      int ownerY = 4;
      std::string ownerLines[] = {
          TR("career_season_owner_1"),
          TR("career_season_owner_2"),
          TR("career_season_owner_3"),
          TR("career_season_owner_4"),
      };
      for (int i = 0; i < 4; ++i) {
        Gui2Caption* line =
            new Gui2Caption(windowManager, "caption_season_owner_" + std::to_string(i), 2, ownerY,
                            78, 2, ownerLines[i]);
        ownerFrame->AddView(line);
        line->Show();
        ownerY += 3;
      }
      bgPanel->AddView(ownerFrame);
      ownerFrame->Show();
    }

    if (!activeSave->season.seasonSummaries.empty()) {
      Gui2Caption* histTitle = new Gui2Caption(windowManager, "caption_season_hist", 6, 55, 80, 2,
                                               TR("career_season_past"));
      bgPanel->AddView(histTitle);
      histTitle->Show();

      Gui2Grid* histGrid = new Gui2Grid(windowManager, "season_hist_grid", 6, 58, 80, 18);
      int row = 0;
      int startIdx = std::max(0, static_cast<int>(activeSave->season.seasonSummaries.size()) - 5);
      for (int i = startIdx; i < static_cast<int>(activeSave->season.seasonSummaries.size()); i++) {
        Gui2Caption* entry = new Gui2Caption(windowManager, "caption_hist_" + std::to_string(row),
                                             0, 0, 76, 2, activeSave->season.seasonSummaries[i]);
        histGrid->AddView(entry, row++, 0);
      }
      histGrid->UpdateLayout(0.5);
      bgPanel->AddView(histGrid);
      histGrid->Show();
    }
  }

  Gui2Button* btnStandings = new Gui2Button(windowManager, "btn_season_standings", 8, 80, 26, 4,
                                            TR("career_hub_btn_standings"));
  btnStandings->sig_OnClick.connect([this](...) { CreatePage(e_PageID_CareerStandings); });
  bgPanel->AddView(btnStandings);
  btnStandings->Show();

  Gui2Button* btnAdvance =
      new Gui2Button(windowManager, "btn_season_advance", 38, 80, 44, 4,
                     TR(IsOwnerMode() ? "career_season_advance_owner" : "career_season_advance"));
  m_season = activeSave ? activeSave->season.currentSeason : 0;
  btnAdvance->SetActive(CareerDatabase::GetInstance().CanAdvanceSeason());
  btnAdvance->sig_OnClick.connect([this](...) { AdvanceSeason(); });
  bgPanel->AddView(btnAdvance);
  btnAdvance->Show();
  if (CareerDatabase::GetInstance().CanAdvanceSeason())
    btnAdvance->SetFocus();
  else
    btnStandings->SetFocus();

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_season_back", 32, 87, 28, 3, TR("career_back_hub"));
  btnBack->sig_OnClick.connect([this](...) { GoToHub(); });
  bgPanel->AddView(btnBack);
  btnBack->Show();

  this->Show();
}

CareerSeasonPage::~CareerSeasonPage() {}

void CareerSeasonPage::AdvanceSeason() {
  if (CareerDatabase::GetInstance().AdvanceSeason(m_season))
    CreatePage(GetHubPageID());
}

void CareerSeasonPage::GoToHub() {
  CreatePage(GetHubPageID());
}

// ---------------------------------------------------------------------------
// CareerMatchdayPage - Match Simulation
// ---------------------------------------------------------------------------

CareerMatchdayPage::CareerMatchdayPage(Gui2WindowManager* windowManager,
                                       const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData),
      frame(nullptr),
      fixtureGrid(nullptr),
      summaryCaption(nullptr),
      m_week(1),
      m_matchesPlayed(0),
      m_wins(0),
      m_draws(0),
      m_losses(0),
      m_goalsFor(0),
      m_goalsAgainst(0) {
  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
  if (save) {
    m_week = save->season.currentWeek;
    m_season = save->season.currentSeason;
  }

  frame = new Gui2Frame(windowManager, "frame_matchday", 4, 3, 92, 94, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_matchday", 2, 2, 88, 3,
                                       TRF("career_matchday", {std::to_string(m_week)}));
  frame->AddView(title);
  title->Show();

  Gui2Caption* subtitle = new Gui2Caption(
      windowManager, "caption_matchday_sub", 2, 6, 88, 2,
      save ? TRF("career_matchday_sub", {save->name, std::to_string(save->season.currentSeason),
                                         std::to_string(save->boardConfidence)})
           : TR("career_nosave"));
  frame->AddView(subtitle);
  subtitle->Show();

  Gui2Caption* hint = new Gui2Caption(windowManager, "caption_matchday_hint", 2, 12, 88, 2,
                                      TR("career_matchday_hint"));
  frame->AddView(hint);
  hint->Show();

  fixtureGrid = new Gui2Grid(windowManager, "grid_matchday", 2, 15, 88, 60);
  // Primary actions, fixture controls and Back all live in this one grid so
  // keyboard/gamepad direction keys can reach every control (a standalone
  // button held focus before, keeping the grid out of keyboard reach).
  BuildFixtures();
  PopulateGrid();

  summaryCaption = new Gui2Caption(windowManager, "caption_matchday_summary", 2, 80, 88, 2, "");
  frame->AddView(summaryCaption);
  summaryCaption->Show();

  UpdateSummary();

  this->Show();
}

CareerMatchdayPage::~CareerMatchdayPage() {}

void CareerMatchdayPage::Process() {
  Gui2Page::Process();
  // Smoke route: play this week's fixture as a 3D match.
  if (GetConfiguration()->Get("menu_smoke_test_page", "") == "career_match") {
    const unsigned long now = EnvironmentManager::GetInstance().GetTime_ms();
    if (smokeMatchAt_ms == 0) {
      smokeMatchAt_ms = now + 1000;
    } else if (now >= smokeMatchAt_ms) {
      smokeMatchAt_ms = ~0UL;
      printf("[menu-smoke] Career match: kicking off week %d\n", m_week);
      PlayMatchFixture(0);  // replaces this page
      return;
    }
  }
}

void CareerMatchdayPage::BuildFixtures() {
  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
  if (!save)
    return;

  // Query real opponent clubs from database so fixtures match actual clubs
  std::vector<std::pair<int, std::string>> availableOpponents;
  try {
    auto result = GetDB()->Query(
        "SELECT id, name FROM teams WHERE id != " + int_to_str(save->club.clubID) + " ORDER BY id");
    for (unsigned int r = 0; r < result->data.size(); r++) {
      int id = atoi(result->data.at(r).at(0).c_str());
      std::string name = result->data.at(r).at(1);
      availableOpponents.emplace_back(id, name);
    }
  } catch (...) {
  }

  if (availableOpponents.empty()) {
    static const std::vector<std::pair<int, std::string>> fallbackOpponents = {
        {1, "Ajax"},          {2, "Arsenal"},           {3, "FC Barcelona"},
        {4, "Bayern Munich"}, {5, "Borussia Dortmund"}, {6, "Manchester United"},
        {7, "PSV Eindhoven"}, {8, "Real Madrid"}};
    for (const auto& op : fallbackOpponents) {
      if (op.first != save->club.clubID) {
        availableOpponents.push_back(op);
      }
    }
  }

  // One fixture per matchday visit -- this is the next league match, not a
  // random multi-game block.
  const int numFixtures = 1;

  m_opponents.clear();
  m_opponentDBIDs.clear();
  m_isHome.clear();
  m_results.clear();
  fixtureScoreCaps.assign(numFixtures, nullptr);

  for (int i = 0; i < numFixtures; i++) {
    int opponentIdx = (m_week * 3 + i) % static_cast<int>(availableOpponents.size());
    m_opponents.push_back(availableOpponents[opponentIdx].second);
    m_opponentDBIDs.push_back(availableOpponents[opponentIdx].first);
    m_isHome.push_back(((m_week + i) % 2) == 0);
    m_results.emplace_back();
  }
}

void CareerMatchdayPage::PopulateGrid() {
  if (fixtureGrid) {
    fixtureGrid->Exit();
    delete fixtureGrid;
    fixtureGrid = nullptr;
  }

  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
  if (!save)
    return;

  fixtureGrid = new Gui2Grid(windowManager, "grid_matchday", 2, 15, 88, 60);

  int row = 0;

  // Top action row: everything reachable by arrow keys / d-pad.
  Gui2Button* btnPlayTop =
      new Gui2Button(windowManager, "btn_md_playtop", 0, 0, 42, 2.8f, "PLAY 3D MATCH");
  btnPlayTop->sig_OnClick.connect([this](...) { PlayMatch(); });
  if (IsDelegatingClubMode()) {
    btnPlayTop->SetActive(false);
  }
  fixtureGrid->AddView(btnPlayTop, row++, 0);

  Gui2Button* btnSimAllTop =
      new Gui2Button(windowManager, "btn_md_simalltop", 0, 0, 42, 2.8f, "QUICK SIMULATE MATCH");
  btnSimAllTop->sig_OnClick.connect([this](...) { SimulateAll(); });
  fixtureGrid->AddView(btnSimAllTop, row++, 0);

  int numFixtures = static_cast<int>(m_opponents.size());
  for (int i = 0; i < numFixtures; i++) {
    const auto& res = m_results[i];
    const bool isHome = (i < static_cast<int>(m_isHome.size())) ? m_isHome[i] : true;
    const std::string venue =
        isHome ? ("HOME (" + save->name + " Stadium)") : ("AWAY (" + m_opponents[i] + " Ground)");

    // Atmosphere & Matchday Header
    std::string atmosphere =
        "Atmosphere: Weather 19 deg C (Clear) | Pitch: Pristine | Attendance: 96% Capacity";
    Gui2Caption* header = new Gui2Caption(
        windowManager, "cap_md_hdr_" + std::to_string(i), 0, 0, 84, 2.2f,
        "FIXTURE | " + venue + " | " + save->name + " vs " + m_opponents[i] + "\n" + atmosphere);
    header->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
    fixtureGrid->AddView(header, row++, 0);

    std::string scoreLabel = "STATUS: Ready for Kick-off | Tactics: Balanced Press | Form: " +
                             CareerDatabase::GetInstance().GetFormGuideString(5);
    if (res.played) {
      if (isHome) {
        scoreLabel = "FINAL SCORE: " + save->name + " " + std::to_string(res.homeGoals) + " - " +
                     std::to_string(res.awayGoals) + " " + m_opponents[i] +
                     (res.homeGoals > res.awayGoals
                          ? " [VICTORY!]"
                          : (res.homeGoals == res.awayGoals ? " [DRAW]" : " [DEFEAT]"));
      } else {
        scoreLabel = "FINAL SCORE: " + m_opponents[i] + " " + std::to_string(res.awayGoals) +
                     " - " + std::to_string(res.homeGoals) + " " + save->name +
                     (res.homeGoals > res.awayGoals
                          ? " [VICTORY!]"
                          : (res.homeGoals == res.awayGoals ? " [DRAW]" : " [DEFEAT]"));
      }
    }
    Gui2Caption* scoreCap = new Gui2Caption(windowManager, "cap_md_score_" + std::to_string(i), 0,
                                            0, 84, 2.2f, scoreLabel);
    scoreCap->SetColor(res.played ? windowManager->GetStyle()->GetColor(e_DecorationType_Bright1)
                                  : windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
    fixtureGrid->AddView(scoreCap, row++, 0);
    fixtureScoreCaps[i] = scoreCap;

    if (res.played) {
      std::string scorersStr;
      if (!res.scorers.empty()) {
        scorersStr = "Goals: " + res.scorers[0];
        for (int s = 1; s < static_cast<int>(res.scorers.size()); s++) {
          scorersStr += ", " + res.scorers[s];
        }
      } else {
        scorersStr = "Goals: No goals scored (Scoreless Draw / Defensive Battle)";
      }

      int userShots = isHome ? res.homeShots : res.awayShots;
      int oppShots = isHome ? res.awayShots : res.homeShots;
      int userPoss = isHome ? res.homePossession : (100 - res.homePossession);
      int oppPoss = 100 - userPoss;
      int corners = 3 + (userShots % 5);
      int fouls = 6 + (oppShots % 7);
      float motmRating = 7.5f + static_cast<float>(std::max(res.homeGoals, res.awayGoals)) * 0.5f;
      if (motmRating > 9.8f)
        motmRating = 9.8f;

      std::string motmPlayer = !res.scorers.empty()
                                   ? res.scorers[0]
                                   : (!save->roster.empty() ? save->roster[0].name : "Goalkeeper");

      char statsBuf[512];
      snprintf(statsBuf, sizeof(statsBuf),
               "MATCH SUMMARY & PERFORMANCE:\n"
               "%s\n"
               "Shots (On Target): %d (%d) vs %d (%d) | Possession: %d%% vs %d%%\n"
               "Corner Kicks: %d - %d | Fouls Committed: %d - %d | Pass Accuracy: 86%%\n"
               "Man of the Match: %s (Rating: %.1f / 10.0)",
               scorersStr.c_str(), userShots, std::max(1, userShots * 6 / 10), oppShots,
               std::max(1, oppShots * 6 / 10), userPoss, oppPoss, corners, 4, fouls, 8,
               motmPlayer.c_str(), motmRating);

      Gui2Caption* statsCap = new Gui2Caption(windowManager, "cap_md_stats_" + std::to_string(i), 0,
                                              0, 84, 10, std::string(statsBuf));
      fixtureGrid->AddView(statsCap, row++, 0);
    }

    if (!res.played) {
      Gui2Button* btnSim = new Gui2Button(windowManager, "btn_md_sim_" + std::to_string(i), 0, 0,
                                          42, 2.5f, "Quick Sim Match");
      btnSim->sig_OnClick.connect([this, i](...) { SimulateMatch(i); });
      fixtureGrid->AddView(btnSim, row++, 0);

      Gui2Button* btnPlay = new Gui2Button(windowManager, "btn_md_play_" + std::to_string(i), 0, 0,
                                           42, 2.5f, "Play 3D Match");
      btnPlay->sig_OnClick.connect([this, i](...) { PlayMatchFixture(i); });
      if (IsDelegatingClubMode()) {
        btnPlay->SetActive(false);
      }
      fixtureGrid->AddView(btnPlay, row++, 0);
    }
  }

  // Last row: Back to Hub, reachable by the same navigation as everything else.
  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_matchday_back", 0, 0, 42, 2.5f, TR("career_back_hub"));
  btnBack->sig_OnClick.connect([this](...) { GoBack(); });
  fixtureGrid->AddView(btnBack, row++, 0);

  fixtureGrid->UpdateLayout(0.5f, 0.5f, 0.25f, 0.25f);
  frame->AddView(fixtureGrid);
  fixtureGrid->Show();

  if (IsDelegatingClubMode()) {
    btnSimAllTop->SetFocus();
  } else {
    btnPlayTop->SetFocus();
  }

  UpdateSummary();
}

void CareerMatchdayPage::SimulateMatch(int fixtureIndex) {
  if (fixtureIndex < 0 || fixtureIndex >= static_cast<int>(m_results.size()))
    return;
  if (m_results[fixtureIndex].played)
    return;

  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
  const bool isHome =
      (fixtureIndex < static_cast<int>(m_isHome.size())) ? m_isHome[fixtureIndex] : true;
  SimulatedMatch res = CareerDatabase::GetInstance().SimulateMatchResult(
      m_opponents[fixtureIndex], std::to_string(m_opponentDBIDs[fixtureIndex]), isHome);
  if (!save || !CareerDatabase::GetInstance().CompleteFixture(
                   m_season, m_week, isHome, save->club.clubID, m_opponentDBIDs[fixtureIndex],
                   m_opponents[fixtureIndex], res.homeGoals, res.awayGoals, res.scorers))
    return;
  m_results[fixtureIndex] = res;

  m_matchesPlayed++;
  if (res.homeGoals > res.awayGoals)
    m_wins++;
  else if (res.homeGoals == res.awayGoals)
    m_draws++;
  else
    m_losses++;
  m_goalsFor += res.homeGoals;
  m_goalsAgainst += res.awayGoals;

  PopulateGrid();
}

void CareerMatchdayPage::SimulateAll() {
  for (int i = 0; i < static_cast<int>(m_results.size()); i++) {
    if (!m_results[i].played)
      SimulateMatch(i);
  }
}

void CareerMatchdayPage::PlayMatch() {
  PlayMatchFixture(0);
}

namespace {

// The career roster as the 3D match squad: the match squad the simulation
// uses as the starting eleven, then up to seven substitutes by rating.
SquadOverride CareerMatchSquad(CareerSave& save) {
  CareerSim::AssignRosterIdentities(save);
  std::vector<int> order = CareerSim::SelectMatchSquad(save);
  std::vector<int> bench;
  for (int i = 0; i < static_cast<int>(save.roster.size()); i++) {
    if (std::find(order.begin(), order.end(), i) == order.end())
      bench.push_back(i);
  }
  std::sort(bench.begin(), bench.end(),
            [&save](int a, int b) { return save.roster[a].ovr > save.roster[b].ovr; });
  if (bench.size() > 7)
    bench.resize(7);
  order.insert(order.end(), bench.begin(), bench.end());

  SquadOverride squad;
  squad.teamDatabaseID = save.club.clubID;
  for (int index : order) {
    const PlayerCareerState& career = save.roster[index];
    SquadOverridePlayer player;
    player.databaseID = career.databaseID;
    std::string name = career.name;
    name.erase(0, name.find_first_not_of(' '));
    const size_t space = name.find(' ');
    player.firstName = space == std::string::npos ? "" : name.substr(0, space);
    player.lastName = space == std::string::npos ? name : name.substr(space + 1);
    player.role = career.preferredPosition.empty() ? career.position : career.preferredPosition;
    player.baseStat = std::clamp(career.ovr / 100.0f, 0.3f, 0.99f);
    player.age = career.age;
    squad.players.push_back(player);
  }
  if (const PlayerCareerState* controlled = ControlledCareerPlayer(save))
    squad.lockedPlayerDatabaseID = controlled->databaseID;
  return squad;
}

}  // namespace

void CareerMatchdayPage::PlayMatchFixture(int fixtureIndex) {
  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
  if (!save || !CanPlayCareerMatch(*save) || fixtureIndex < 0 ||
      fixtureIndex >= static_cast<int>(m_opponents.size()))
    return;
  if (save->club.clubID <= 0)
    return;

  int teamDBID = save->club.clubID;
  int opponentDBID =
      (fixtureIndex < static_cast<int>(m_opponentDBIDs.size())) ? m_opponentDBIDs[fixtureIndex] : 1;
  bool isHome = (fixtureIndex < static_cast<int>(m_isHome.size())) ? m_isHome[fixtureIndex] : true;
  std::string oppName = m_opponents[fixtureIndex];

  // Arm pending fixture so GameOverPage updates career state on completion
  if (m_results[fixtureIndex].played ||
      !CareerDatabase::GetInstance().SetPendingFixture(isHome, teamDBID, opponentDBID, oppName,
                                                       m_season, m_week))
    return;

  // Setup controllers and team IDs:
  // Home team is always team 0 (first ID), Away team is always team 1 (second ID).
  // side -1 controls team 0, side 1 controls team 1.
  std::vector<SideSelection> sides(1);
  sides[0].controllerID = 0;
  if (isHome) {
    sides[0].side = -1;  // user controls Home
    GetMenuTask()->SetTeamIDs(std::to_string(teamDBID), std::to_string(opponentDBID));
  } else {
    sides[0].side = 1;  // user controls Away
    GetMenuTask()->SetTeamIDs(std::to_string(opponentDBID), std::to_string(teamDBID));
  }
  GetMenuTask()->SetControllerSetup(sides);
  // Field the career squad, not the club's database squad; in Player mode the
  // controller stays on the user's footballer.
  SetSquadOverride(CareerMatchSquad(*save));

  Properties props;
  CreatePage((int)e_PageID_MatchOptions, props);
}

void CareerMatchdayPage::UpdateSummary() {
  if (summaryCaption) {
    summaryCaption->SetCaption(TRF(
        "career_matchday_summary",
        {std::to_string(m_matchesPlayed), std::to_string(m_wins), std::to_string(m_draws),
         std::to_string(m_losses), std::to_string(m_goalsFor), std::to_string(m_goalsAgainst)}));
  }
}

void CareerMatchdayPage::GoBack() {
  CreatePage(GetHubPageID());
}
