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
#include "career_transfers.hpp"
#include "utils/gui2/widgets/dialog.hpp"
#include "utils/localization.hpp"

using namespace blunted;

namespace {

constexpr unsigned long kCareerMenuSmokeDelay_ms = 500;

std::string GetCareerModeDisplay(const CareerSave* save) {
  if (!save)
    return TR("career_mode_default");
  switch (save->mode) {
    case CareerMode::COACH:
      return TR("career_mode_coach");
    case CareerMode::GM:
      return TR("career_mode_gm");
    case CareerMode::PLAYER:
      return TR("career_mode_player");
    case CareerMode::OWNER:
      return TR("career_mode_owner");
    default:
      return TR("career_mode_manager");
  }
}

std::string FormatCareerMoney(long long amount) {
  const bool negative = amount < 0;
  unsigned long long value =
      negative ? static_cast<unsigned long long>(-amount) : static_cast<unsigned long long>(amount);
  std::string digits = std::to_string(value);
  std::string grouped;
  int count = 0;
  for (int i = static_cast<int>(digits.size()) - 1; i >= 0; --i) {
    if (count > 0 && count % 3 == 0)
      grouped.push_back(',');
    grouped.push_back(digits[static_cast<size_t>(i)]);
    ++count;
  }
  std::reverse(grouped.begin(), grouped.end());
  return std::string("EUR ") + (negative ? "-" : "") + grouped;
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
  return save && save->mode == CareerMode::OWNER;
}

static int GetHubPageID() {
  return IsOwnerMode() ? e_PageID_OwnerHub : e_PageID_CareerHub;
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

  Gui2Button* btnCoach =
      new Gui2Button(windowManager, "btn_mycoach", 0, 0, 47, 6, TR("career_menu_coach"));
  Gui2Button* btnGM = new Gui2Button(windowManager, "btn_mygm", 0, 0, 47, 6, TR("career_menu_gm"));
  Gui2Button* btnPlayer =
      new Gui2Button(windowManager, "btn_playercareer", 0, 0, 47, 6, TR("career_menu_player"));
  Gui2Button* btnManager =
      new Gui2Button(windowManager, "btn_managercareer", 0, 0, 47, 6, TR("career_menu_manager"));
  Gui2Button* btnOwner =
      new Gui2Button(windowManager, "btn_ownercareer", 0, 0, 47, 6, TR("career_menu_owner"));

  btnCoach->sig_OnClick.connect([this](...) { GoMyCoach(); });
  btnGM->sig_OnClick.connect([this](...) { GoMyGM(); });
  btnPlayer->sig_OnClick.connect([this](...) { GoPlayerCareer(); });
  btnManager->sig_OnClick.connect([this](...) { GoManagerCareer(); });
  btnOwner->sig_OnClick.connect([this](...) { GoOwnerCareer(); });

  Gui2Grid* grid = new Gui2Grid(windowManager, "career_grid", 2, 7, 49, 56);
  grid->AddView(btnCoach, 0, 0);
  grid->AddView(btnGM, 1, 0);
  grid->AddView(btnPlayer, 2, 0);
  grid->AddView(btnManager, 3, 0);
  grid->AddView(btnOwner, 4, 0);
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
    saveInfoStr = "🏆 " + summary.clubName + "\n" +
                  "📅 Season " + std::to_string(summary.season) + " (Week " + std::to_string(summary.week) + ")\n" +
                  "👔 " + summary.managerName + "\n" +
                  "💰 " + FormatCareerMoney(summary.transferBudget) + "\n" +
                  "🤝 Trust: " + std::to_string(summary.boardConfidence) + "%";
    if (!summary.timestamp.empty()) {
      saveInfoStr += "\n🕒 " + summary.timestamp;
    }
  } else {
    saveInfoStr = TR(hasSave ? "career_menu_save_ready" : "career_menu_save_empty");
  }

  Gui2Caption* saveStatus =
      new Gui2Caption(windowManager, "caption_career_save_status", 2, 6, 28, 12, saveInfoStr);
  saveFrame->AddView(saveStatus);
  saveStatus->Show();

  Gui2Button* btnContinue =
      new Gui2Button(windowManager, "btn_continue", 2, 19, 28, 6,
                     TR(hasSave ? "career_menu_continue" : "career_menu_continue_empty"));
  btnContinue->sig_OnClick.connect([this](...) { GoContinueCareer(); });
  btnContinue->SetActive(hasSave);
  saveFrame->AddView(btnContinue);
  btnContinue->Show();

  Gui2Caption* footer = new Gui2Caption(windowManager, "caption_career_footer", 2, 30, 28, 15,
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
    btnCoach->SetFocus();
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
    CreatePage(IsOwnerMode() ? e_PageID_OwnerHub : e_PageID_CareerHub);
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
  GoCareerMode("mycoach");
}
void CareerMenuPage::GoMyGM() {
  GoCareerMode("mygm");
}
void CareerMenuPage::GoPlayerCareer() {
  GoCareerMode("player");
}
void CareerMenuPage::GoManagerCareer() {
  GoCareerMode("manager");
}
void CareerMenuPage::GoOwnerCareer() {
  GoCareerMode("owner");
}

// ---------------------------------------------------------------------------
// CareerNewGamePage
// ---------------------------------------------------------------------------

CareerNewGamePage::CareerNewGamePage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* bgPanel = new Gui2Frame(windowManager, "bg_career_new", 8, 7, 84, 86, true);
  this->AddView(bgPanel);
  bgPanel->Show();
  m_mode = pageData.properties ? pageData.properties->Get("careerMode", "manager") : "manager";

  std::string modeLabel = TR("career_mode_manager");
  if (m_mode == "mycoach")
    modeLabel = TR("career_mode_coach");
  else if (m_mode == "mygm")
    modeLabel = TR("career_mode_gm");
  else if (m_mode == "player")
    modeLabel = TR("career_mode_player");
  else if (m_mode == "owner")
    modeLabel = TR("career_mode_owner");

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
  std::string nameDefault = TR("career_mode_manager");
  if (m_mode == "player") {
    nameFieldLabel = TR("career_new_player_name");
    nameDefault = TR("career_mode_player");
  } else if (m_mode == "mygm") {
    nameFieldLabel = TR("career_new_gm_name");
    nameDefault = TR("career_mode_gm");
  } else if (m_mode == "mycoach") {
    nameFieldLabel = TR("career_new_coach_name");
    nameDefault = TR("career_mode_coach");
  } else if (m_mode == "owner") {
    nameFieldLabel = TR("career_new_owner_name");
    nameDefault = TR("career_mode_owner");
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
  // Pulldown OnChange only fires on user input — seed the selected ID from the
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
      try {
        auto ageResult =
            GetDB()->Query("SELECT age FROM players WHERE id = " + int_to_str(pd->GetDatabaseID()));
        if (!ageResult->data.empty()) {
          age = atoi(ageResult->data.at(0).at(0).c_str());
          pot = std::min(99, ovr + static_cast<int>((99 - age) * 0.5));
        }
      } catch (...) {
      }

      const auto& roles = pd->GetRoles();
      std::string pos = roles.empty() ? "CM" : RoleToCareerPos(roles[0]);

      long long value = static_cast<long long>(ovr) * static_cast<long long>(ovr) * 5000;
      long long wage = (value / 1000) + static_cast<int>(random(500, 2000));

      PlayerCareerState cp;
      cp.name = pd->GetFirstName() + " " + pd->GetLastName();
      cp.position = pos;
      cp.preferredPosition = pos;
      cp.ovr = ovr;
      cp.pot = pot;
      cp.age = age;
      cp.value = value;
      cp.wage = wage;
      cp.databaseID = pd->GetDatabaseID();
      cp.contract.yearsRemaining = static_cast<int>(random(2, 5));
      save->roster.push_back(cp);
    }

    long long totalWage = 0;
    for (const auto& p : save->roster)
      totalWage += p.wage;
    save->wageBudget = totalWage * 130 / 100;
    save->transferBudget = 15000000;

    if (m_mode == "owner") {
      save->mode = CareerMode::OWNER;
      save->transferBudget = 60000000;
      save->wageBudget = totalWage * 150 / 100;
    }
  }

  if (IsOwnerMode()) {
    CreatePage(e_PageID_OwnerHub);
  } else {
    CreatePage(e_PageID_CareerHub);
  }
}

// ---------------------------------------------------------------------------
// CareerHubPage
// ---------------------------------------------------------------------------
CareerHubPage::CareerHubPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  if (IsOwnerMode()) {
    CreatePage(e_PageID_OwnerHub);
    return;
  }

  Gui2Frame* bgPanel = new Gui2Frame(windowManager, "bg_career_hub", 2, 1, 96, 98, true);
  this->AddView(bgPanel);
  bgPanel->Show();

  CareerSave* activeSave = CareerDatabase::GetInstance().GetActiveSave();

  // Top Header Banner
  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_careerhub", 2, 1.5f, 92, 3,
                      activeSave ? ("CAREER HUB | " + activeSave->name + " (" + activeSave->club.leagueName + ")")
                                 : TR("career_hub_title"));
  title->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  bgPanel->AddView(title);
  title->Show();

  int week = activeSave ? (activeSave->season.currentWeek + 1) : 1;
  int maxWeeks = activeSave ? activeSave->season.maxWeeks : 38;
  int seasonNum = activeSave ? activeSave->season.currentSeason : 1;

  std::string statusHeader = activeSave ?
      ("Manager: " + activeSave->managerName + " | Season " + std::to_string(seasonNum) + " | Week " + std::to_string(week) + "/" + std::to_string(maxWeeks) +
       " | Budget: " + FormatCareerMoney(activeSave->transferBudget) + " | Wage: " + FormatCareerMoney(activeSave->wageBudget) +
       " | Board Trust: " + std::to_string(activeSave->boardConfidence) + "% (" + CareerDatabase::GetInstance().GetReputationStatus() + ")")
      : TR("career_nosave");

  Gui2Caption* topLine = new Gui2Caption(windowManager, "caption_hub_team", 2, 4.5f, 92, 2.5f, statusHeader);
  topLine->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright1));
  bgPanel->AddView(topLine);
  topLine->Show();

  // Left Navigation Panel (Categorized Action Grid)
  Gui2Frame* navFrame = new Gui2Frame(windowManager, "frame_career_hub_nav", 2, 8, 44, 88, true);
  bgPanel->AddView(navFrame);
  navFrame->Show();

  Gui2Caption* navTitle = new Gui2Caption(windowManager, "caption_career_hub_nav", 2, 1.5f, 40, 2.5f,
                                          "MANAGEMENT COMMANDS");
  navTitle->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  navFrame->AddView(navTitle);
  navTitle->Show();

  std::string matchdayBtnLabel = "⚽ NEXT MATCHDAY (Week " + std::to_string(week) + ")";
  Gui2Button* btnMatchday =
      new Gui2Button(windowManager, "btn_matchday", 0, 0, 38, 3, matchdayBtnLabel);
  Gui2Button* btnSeason =
      new Gui2Button(windowManager, "btn_season_end", 0, 0, 38, 2.6f, "📅 Season Review & Standings");
  Gui2Button* btnSquad =
      new Gui2Button(windowManager, "btn_squad", 0, 0, 38, 2.6f, "📋 Squad Roster & Form");
  Gui2Button* btnStrategy =
      new Gui2Button(windowManager, "btn_strategy", 0, 0, 38, 2.6f, "📐 Tactics & Strategy");
  Gui2Button* btnTraining =
      new Gui2Button(windowManager, "btn_training", 0, 0, 38, 2.6f, "⚡ Squad Development & Training");
  Gui2Button* btnYouth =
      new Gui2Button(windowManager, "btn_youth", 0, 0, 38, 2.6f, "⭐ Youth Scouting & Academy");
  Gui2Button* btnTransfers =
      new Gui2Button(windowManager, "btn_transfers", 0, 0, 38, 2.6f, "💼 Transfer Market & Bids");
  Gui2Button* btnFreeAgency =
      new Gui2Button(windowManager, "btn_freeagency", 0, 0, 38, 2.6f, "🔍 Free Agent Recruiting");
  Gui2Button* btnPressConf =
      new Gui2Button(windowManager, "btn_pressconf", 0, 0, 38, 2.6f, "🎙️ Press Conference & Media");
  Gui2Button* btnLeagueExp =
      new Gui2Button(windowManager, "btn_leagueexp", 0, 0, 38, 2.6f, "🌐 League Expansion & Rules");
  Gui2Button* btnCustomLeague =
      new Gui2Button(windowManager, "btn_customleague", 0, 0, 38, 2.6f, "⚙️ Custom League Setup");
  Gui2Button* btnExit =
      new Gui2Button(windowManager, "btn_hub_exit", 0, 0, 38, 2.6f, "🚪 Exit to Career Modes");

  btnMatchday->sig_OnClick.connect([this](...) { GoMatchday(); });
  btnSeason->sig_OnClick.connect([this](...) { GoSeason(); });
  btnSquad->sig_OnClick.connect([this](...) { GoSquad(); });
  btnStrategy->sig_OnClick.connect([this](...) { GoStrategy(); });
  btnTraining->sig_OnClick.connect([this](...) { GoTraining(); });
  btnYouth->sig_OnClick.connect([this](...) { GoYouthAcademy(); });
  btnPressConf->sig_OnClick.connect([this](...) { GoPressConference(); });
  btnLeagueExp->sig_OnClick.connect([this](...) { GoLeagueExpansion(); });
  btnCustomLeague->sig_OnClick.connect([this](...) { GoCustomLeague(); });
  btnTransfers->sig_OnClick.connect([this](...) { GoTransferMarket(); });
  btnFreeAgency->sig_OnClick.connect([this](...) { GoFreeAgency(); });
  btnExit->sig_OnClick.connect([this](...) { CreatePage(e_PageID_CareerMenu); });

  Gui2Grid* grid = new Gui2Grid(windowManager, "hub_grid", 2, 4.5f, 40, 80);
  grid->AddView(btnMatchday, 0, 0);
  grid->AddView(btnSeason, 1, 0);
  grid->AddView(btnSquad, 2, 0);
  grid->AddView(btnStrategy, 3, 0);
  grid->AddView(btnTraining, 4, 0);
  grid->AddView(btnYouth, 5, 0);
  grid->AddView(btnTransfers, 6, 0);
  grid->AddView(btnFreeAgency, 7, 0);
  grid->AddView(btnPressConf, 8, 0);
  grid->AddView(btnLeagueExp, 9, 0);
  grid->AddView(btnCustomLeague, 10, 0);
  grid->AddView(btnExit, 11, 0);
  grid->UpdateLayout(0.5f, 0.5f, 0.25f, 0.25f);

  navFrame->AddView(grid);
  grid->Show();

  // Right Side Dashboard Widgets
  if (activeSave) {
    // Card 1: Next Fixture Spotlight & Tactical Report
    Gui2Frame* fixtureFrame =
        new Gui2Frame(windowManager, "frame_career_hub_fixture", 48, 8, 46, 26, true);
    Gui2Caption* fixtureTitle = new Gui2Caption(windowManager, "caption_hub_fixture_title", 2, 1.5f, 42, 2.2f,
                                                "NEXT MATCHDAY SPOTLIGHT");
    fixtureTitle->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
    fixtureFrame->AddView(fixtureTitle);
    fixtureTitle->Show();

    std::string previewText = CareerDatabase::GetInstance().GetNextOpponentPreview(activeSave->season.currentWeek);
    std::string formGuide = CareerDatabase::GetInstance().GetFormGuideString(5);

    std::string fixtureBody =
        "Upcoming Match: Matchweek " + std::to_string(week) + " / " + std::to_string(maxWeeks) + "\n" +
        "Opponent: " + previewText + "\n" +
        "Your Recent Form: " + formGuide + "\n" +
        "Active Tactic: " + activeSave->activeStrategy + " | Squad Size: " + std::to_string(activeSave->roster.size()) + " Players";

    Gui2Caption* fixtureInfo =
        new Gui2Caption(windowManager, "caption_hub_fixture_body", 2, 4.5f, 42, 20, fixtureBody);
    fixtureFrame->AddView(fixtureInfo);
    fixtureInfo->Show();
    bgPanel->AddView(fixtureFrame);
    fixtureFrame->Show();

    // Card 2: League Performance & Club Operations
    Gui2Frame* seasonFrame =
        new Gui2Frame(windowManager, "frame_career_hub_season", 48, 36, 46, 28, true);
    Gui2Caption* seasonTitle = new Gui2Caption(windowManager, "caption_hub_season_title", 2, 1.5f, 42,
                                               2.2f, "LEAGUE TABLE & PERFORMANCE");
    seasonTitle->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
    seasonFrame->AddView(seasonTitle);
    seasonTitle->Show();

    int pts = activeSave->seasonWins * 3 + activeSave->seasonDraws;
    int gd = activeSave->seasonGoalsFor - activeSave->seasonGoalsAgainst;
    int played = activeSave->seasonWins + activeSave->seasonDraws + activeSave->seasonLosses;
    int estPos = CareerDatabase::EstimateLeaguePosition(activeSave->seasonWins, activeSave->seasonDraws, activeSave->seasonLosses);

    char tableSummary[512];
    snprintf(tableSummary, sizeof(tableSummary),
             "Current Standing: #%d in %s\nRecord: %d Played | %d Wins | %d Draws | %d Losses\nGoals: %d Scored, %d Conceded (GD: %+d) | Total Points: %d PTS\nBoard Security: %d%% (%s) | Academy Prospects: %d\nTraining Points Available: %d TP",
             estPos, activeSave->club.leagueName.c_str(), played, activeSave->seasonWins, activeSave->seasonDraws,
             activeSave->seasonLosses, activeSave->seasonGoalsFor, activeSave->seasonGoalsAgainst, gd, pts,
             activeSave->boardConfidence, CareerDatabase::GetInstance().GetReputationStatus().c_str(),
             static_cast<int>(activeSave->youthAcademy.size()), activeSave->trainingPoints);

    Gui2Caption* seasonInfo =
        new Gui2Caption(windowManager, "caption_hub_season_body", 2, 4.5f, 42, 22, std::string(tableSummary));
    seasonFrame->AddView(seasonInfo);
    seasonInfo->Show();
    bgPanel->AddView(seasonFrame);
    seasonFrame->Show();

    // Card 3: Media & Press Ticker
    Gui2Frame* newsFrame =
        new Gui2Frame(windowManager, "frame_career_hub_news", 48, 66, 46, 30, true);
    Gui2Caption* newsTitle = new Gui2Caption(windowManager, "caption_hub_news_title", 2, 1.5f, 42, 2.2f,
                                             "MEDIA & PRESS FEED");
    newsTitle->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
    newsFrame->AddView(newsTitle);
    newsTitle->Show();

    auto headlines = CareerDatabase::GetInstance().GetNewsHeadlines(3);
    std::string newsBody = "";
    for (size_t i = 0; i < headlines.size(); i++) {
      newsBody += "• " + headlines[i] + (i + 1 < headlines.size() ? "\n\n" : "");
    }

    Gui2Caption* newsInfo =
        new Gui2Caption(windowManager, "caption_hub_news_body", 2, 4.5f, 42, 24, newsBody);
    newsFrame->AddView(newsInfo);
    newsInfo->Show();
    bgPanel->AddView(newsFrame);
    newsFrame->Show();
  }

  btnMatchday->SetFocus();
  this->Show();
}

CareerHubPage::~CareerHubPage() {}

void CareerHubPage::GoTransferMarket() {
  CreatePage(e_PageID_CareerTransferMarket);
}
void CareerHubPage::GoSquad() {
  CreatePage(e_PageID_CareerSquadRoster);
}
void CareerHubPage::GoPressConference() {
  CreatePage(e_PageID_CareerPressConference);
}
void CareerHubPage::GoLeagueExpansion() {
  CreatePage(e_PageID_CareerLeagueExpansion);
}
void CareerHubPage::GoCustomLeague() {
  CreatePage(e_PageID_CareerCustomLeague);
}
void CareerHubPage::GoFreeAgency() {
  CreatePage(e_PageID_CareerFreeAgency);
}
void CareerHubPage::GoTraining() {
  CreatePage(e_PageID_CareerTraining);
}
void CareerHubPage::GoStrategy() {
  CreatePage(e_PageID_CareerStrategy);
}
void CareerHubPage::GoYouthAcademy() {
  CreatePage(e_PageID_CareerYouthAcademy);
}
void CareerHubPage::GoSeason() {
  CreatePage(e_PageID_CareerSeason);
}
void CareerHubPage::GoMatchday() {
  CreatePage(e_PageID_CareerMatchday);
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
                                       "💼 TRANSFER NEGOTIATIONS: " + m_playerName);
  title->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright1));
  bgPanel->AddView(title);
  title->Show();

  if (found) {
    // Scouting Report & Agent Card
    Gui2Frame* profileFrame = new Gui2Frame(windowManager, "frame_bid_prof", 4, 6, 84, 15, true);
    Gui2Caption* profTitle = new Gui2Caption(windowManager, "cap_bid_proftitle", 2, 1, 80, 2,
                                             "📋 SCOUTING PROFILE & AGENT INTELLIGENCE");
    profTitle->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
    profileFrame->AddView(profTitle);
    profTitle->Show();

    char profBuf[512];
    snprintf(profBuf, sizeof(profBuf),
             "Position: %s | Overall: %d | Potential: %d | Age: %d\n"
             "Market Valuation: %s | Asking Price: %s | Desired Wage: %s/wk\n"
             "Agent Patience: [||||||||  ] 80%% | Target Interest: HIGH | Contract Desired: 3 Years",
             target.preferredPosition.c_str(), target.overallRating, target.potentialRating, target.age,
             FormatCareerMoney(target.value).c_str(),
             FormatCareerMoney(target.askingPrice).c_str(),
             FormatCareerMoney(target.wage).c_str());
    Gui2Caption* profBody = new Gui2Caption(windowManager, "cap_bid_profbody", 2, 4, 80, 10, std::string(profBuf));
    profileFrame->AddView(profBody);
    profBody->Show();
    bgPanel->AddView(profileFrame);
    profileFrame->Show();

    // Offers Grid
    Gui2Grid* grid = new Gui2Grid(windowManager, "grid_detail", 6, 23, 80, 36);

    long long askPrice = target.askingPrice;
    Gui2Button* bidFull = new Gui2Button(
        windowManager, "btn_bid_full", 0, 0, 76, 3.2f,
        "🥇 Meet Asking Price (" + FormatCareerMoney(askPrice) + ") — Guaranteed Acceptance [Key Player]");
    bidFull->sig_OnClick.connect([this, askPrice](...) { PlaceBidForPlayer(askPrice); });
    preferredBidButton = bidFull;
    grid->AddView(bidFull, 0, 0);

    long long bid85 = target.askingPrice * 85 / 100;
    Gui2Button* bid85Button = new Gui2Button(
        windowManager, "btn_bid_85", 0, 0, 76, 3.2f,
        "🥈 Negotiated Offer (" + FormatCareerMoney(bid85) + ") — 85% Price [First Team Role]");
    bid85Button->sig_OnClick.connect([this, bid85](...) { PlaceBidForPlayer(bid85); });
    grid->AddView(bid85Button, 1, 0);

    long long bid70 = target.askingPrice * 70 / 100;
    Gui2Button* bid70Button = new Gui2Button(
        windowManager, "btn_bid_70", 0, 0, 76, 3.2f,
        "🥉 Lowball Counter (" + FormatCareerMoney(bid70) + ") — 70% Price [High Risk of Rejection]");
    bid70Button->sig_OnClick.connect([this, bid70](...) { PlaceBidForPlayer(bid70); });
    grid->AddView(bid70Button, 2, 0);

    grid->UpdateLayout(0.5f, 0.5f, 0.25f, 0.25f);
    bgPanel->AddView(grid);
    grid->Show();

    CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
    if (save) {
      long long agentFee = target.askingPrice / 20;
      long long totalWithFee = target.askingPrice + agentFee;
      std::string budgetStr = "Financial Breakdown: +5% Agent Fee (" + FormatCareerMoney(agentFee) + ") = Total " + FormatCareerMoney(totalWithFee) +
                              " | Club Budget Remaining: " + FormatCareerMoney(save->transferBudget);
      if (totalWithFee > save->transferBudget) {
        budgetStr += "\n⚠️ WARNING: Proposed offer exceeds current club transfer funds!";
      }
      Gui2Caption* fee =
          new Gui2Caption(windowManager, "caption_detail_fee", 4, 62, 84, 5, budgetStr);
      fee->SetColor(totalWithFee > save->transferBudget ? windowManager->GetStyle()->GetColor(e_DecorationType_Dark1)
                                                        : windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
      bgPanel->AddView(fee);
      fee->Show();
    }
  }

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_detail_back", 26, 85, 40, 3, "🔙 Return to Transfer Market");
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
    Gui2Caption* warn = new Gui2Caption(windowManager, "caption_bid_warn", 10, 78, 80, 3,
                                        "❌ The selling club and agent have immediately rejected this valuation!");
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
  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_pressconf", 4, 2, 84, 3, "🎙️ OFFICIAL POST-MATCH PRESS CONFERENCE");
  title->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright1));
  bgPanel->AddView(title);
  title->Show();

  CareerSave* activeSave = CareerDatabase::GetInstance().GetActiveSave();
  if (activeSave) {
    std::string context =
        "Club: " + activeSave->name + " | Season " + std::to_string(activeSave->season.currentSeason) +
        " | Manager Reputation: " + CareerDatabase::GetInstance().GetReputationStatus() +
        " | Board Trust: " + std::to_string(activeSave->boardConfidence) + "% | Form: " +
        CareerDatabase::GetInstance().GetFormGuideString(5);
    Gui2Caption* ctxLine = new Gui2Caption(windowManager, "caption_pc_ctx", 4, 6, 84, 2, context);
    ctxLine->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
    bgPanel->AddView(ctxLine);
    ctxLine->Show();
  }

  Gui2Frame* questionFrame = new Gui2Frame(windowManager, "frame_pc_question", 4, 10, 84, 15, true);
  Gui2Caption* questionLabel = new Gui2Caption(windowManager, "caption_pc_q_label", 2, 1, 80, 2,
                                               "📰 Liam Vance (Sky Sports Football):");
  questionLabel->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  questionFrame->AddView(questionLabel);
  questionLabel->Show();
  Gui2Caption* question = new Gui2Caption(
      windowManager, "caption_pc_question", 2, 4, 80, 9,
      "\"Boss, your squad has been under intense tactical scrutiny in recent fixtures. "
      "How do you evaluate your players' tactical execution and what are your expectations for the season ahead?\"");
  questionFrame->AddView(question);
  question->Show();
  bgPanel->AddView(questionFrame);
  questionFrame->Show();

  Gui2Caption* answerHint = new Gui2Caption(windowManager, "caption_pc_answer_hint", 4, 27, 84, 2,
                                            "Select your official media response:");
  bgPanel->AddView(answerHint);
  answerHint->Show();

  Gui2Button* btnPositive =
      new Gui2Button(windowManager, "btn_pc_positive", 0, 0, 76, 4,
                     "🟢 \"The squad is working exceptionally hard. We take it one match at a time.\" [+1 Team Spirit, +1 Board Trust]");
  Gui2Button* btnNeutral =
      new Gui2Button(windowManager, "btn_pc_neutral", 0, 0, 76, 4,
                     "🟡 \"We stick to our training principles and ignore outside media noise.\" [Neutral Impact, Solid Focus]");
  Gui2Button* btnNegative =
      new Gui2Button(windowManager, "btn_pc_negative", 0, 0, 76, 4,
                     "🔴 \"Standards must be higher. Anyone not performing will be dropped.\" [High Pressure, Critical Message]");

  btnPositive->sig_OnClick.connect([this](...) { SelectAnswer(0); });
  btnNeutral->sig_OnClick.connect([this](...) { SelectAnswer(1); });
  btnNegative->sig_OnClick.connect([this](...) { SelectAnswer(2); });

  Gui2Grid* grid = new Gui2Grid(windowManager, "pc_grid", 6, 31, 76, 48);
  grid->AddView(btnPositive, 0, 0);
  grid->AddView(btnNeutral, 1, 0);
  grid->AddView(btnNegative, 2, 0);
  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_pc_back", 0, 0, 76, 3, "🔙 \"No further comment.\" [Conclude Press Conference]");
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
  if (save) {
    save->leagueSettings.enabled = true;
    if (save->leagueSettings.divisions.empty()) {
      save->leagueSettings.divisions.push_back({"Premier Division", 20, 3, 3, 0});
      save->leagueSettings.divisions.push_back({"Second Division", 20, 3, 3, 0});
    }
  }
  CreatePage(e_PageID_CareerLeagueExpansion);
}
void CareerLeagueExpansionPage::DisableRelegation() {
  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
  if (save)
    save->leagueSettings.enabled = false;
  CreatePage(e_PageID_CareerLeagueExpansion);
}
void CareerLeagueExpansionPage::AddDivision() {
  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
  if (save) {
    int divNum = static_cast<int>(save->leagueSettings.divisions.size()) + 1;
    save->leagueSettings.divisions.push_back({"Division " + std::to_string(divNum), 20, 3, 3, 0});
    save->leagueSettings.enabled = true;
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
  btnReset->sig_OnClick.connect([this, activeSave](...) {
    if (activeSave)
      activeSave->customLeague = CustomLeagueConfig();
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
  if (save) {
    save->customLeague.leagueName = TR("career_custom_league_name");
    save->customLeague.numDivisions = 2;
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
  Gui2Frame* bgPanel = new Gui2Frame(windowManager, "bg_career_train", 5, 0, 90, 100, true);
  this->AddView(bgPanel);
  bgPanel->Show();
  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_training", 10, 5, 80, 3, TR("career_training_title"));
  this->AddView(title);
  title->Show();

  CareerSave* activeSave = CareerDatabase::GetInstance().GetActiveSave();
  int tp = activeSave ? activeSave->trainingPoints : 0;

  Gui2Caption* info = new Gui2Caption(windowManager, "caption_tp", 10, 15, 80, 3,
                                      TRF("career_training_points", {std::to_string(tp)}));
  this->AddView(info);
  info->Show();

  Gui2Caption* hint = new Gui2Caption(windowManager, "caption_train_hint", 10, 19, 80, 3,
                                      TR("career_training_hint"));
  this->AddView(hint);
  hint->Show();

  Gui2Grid* grid = new Gui2Grid(windowManager, "train_grid", 15, 30, 70, 50);

  Gui2Button* btnGeneral =
      new Gui2Button(windowManager, "btn_train_gen", 0, 0, 66, 3, TR("career_train_general"));
  btnGeneral->sig_OnClick.connect([this](...) { TrainSquad(); });
  grid->AddView(btnGeneral, 0, 0);

  Gui2Button* btnAttacking =
      new Gui2Button(windowManager, "btn_train_atk", 0, 0, 66, 3, TR("career_train_attacking"));
  btnAttacking->sig_OnClick.connect([this](...) { TrainFocus("Attacking"); });
  grid->AddView(btnAttacking, 1, 0);

  Gui2Button* btnDefending =
      new Gui2Button(windowManager, "btn_train_def", 0, 0, 66, 3, TR("career_train_defending"));
  btnDefending->sig_OnClick.connect([this](...) { TrainFocus("Defending"); });
  grid->AddView(btnDefending, 2, 0);

  Gui2Button* btnPhysical =
      new Gui2Button(windowManager, "btn_train_phy", 0, 0, 66, 3, TR("career_train_physical"));
  btnPhysical->sig_OnClick.connect([this](...) { TrainFocus("Physical"); });
  grid->AddView(btnPhysical, 3, 0);

  Gui2Button* btnTactical =
      new Gui2Button(windowManager, "btn_train_tac", 0, 0, 66, 3, TR("career_train_tactical"));
  btnTactical->sig_OnClick.connect([this](...) { TrainFocus("Tactical"); });
  grid->AddView(btnTactical, 4, 0);

  Gui2Button* btnShooting =
      new Gui2Button(windowManager, "btn_train_shoot", 0, 0, 66, 3, TR("career_train_shooting"));
  btnShooting->sig_OnClick.connect([this](...) { TrainFocus("Shooting"); });
  grid->AddView(btnShooting, 5, 0);

  grid->UpdateLayout(0.5);
  this->AddView(grid);
  grid->Show();

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_tr_back", 30, 90, 40, 3, TR("career_back_hub"));
  btnBack->sig_OnClick.connect([this](...) { CreatePage(GetHubPageID()); });
  this->AddView(btnBack);
  btnBack->Show();
  const bool canTrain = tp > 0;
  btnGeneral->SetActive(canTrain);
  btnAttacking->SetActive(canTrain);
  btnDefending->SetActive(canTrain);
  btnPhysical->SetActive(canTrain);
  btnTactical->SetActive(canTrain);
  btnShooting->SetActive(canTrain);
  if (canTrain)
    btnGeneral->SetFocus();
  else
    btnBack->SetFocus();

  this->Show();
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
      new Gui2Caption(windowManager, "caption_strategy", 10, 5, 80, 3, TR("career_strategy_title"));
  this->AddView(title);
  title->Show();

  CareerSave* activeSave = CareerDatabase::GetInstance().GetActiveSave();
  std::string curStrat = activeSave ? activeSave->activeStrategy : TR("career_none");

  Gui2Caption* info = new Gui2Caption(windowManager, "caption_curstrat", 10, 15, 80, 3,
                                      TRF("career_strategy_current", {curStrat}));
  this->AddView(info);
  info->Show();

  Gui2Caption* hint = new Gui2Caption(windowManager, "caption_curstrat_hint", 10, 19, 80, 3,
                                      TR("career_strategy_hint"));
  this->AddView(hint);
  hint->Show();

  Gui2Grid* grid = new Gui2Grid(windowManager, "strat_grid", 20, 32, 60, 40);

  Gui2Button* btnAttacking =
      new Gui2Button(windowManager, "btn_strat_atk", 0, 0, 60, 3, TR("career_strategy_attacking"));
  btnAttacking->sig_OnClick.connect([this](...) { SetStrategy("Attacking"); });
  grid->AddView(btnAttacking, 0, 0);

  Gui2Button* btnBalanced =
      new Gui2Button(windowManager, "btn_strat_bal", 0, 0, 60, 3, TR("career_strategy_balanced"));
  btnBalanced->sig_OnClick.connect([this](...) { SetStrategy("Balanced"); });
  grid->AddView(btnBalanced, 1, 0);

  Gui2Button* btnDefensive =
      new Gui2Button(windowManager, "btn_strat_def", 0, 0, 60, 3, TR("career_strategy_defensive"));
  btnDefensive->sig_OnClick.connect([this](...) { SetStrategy("Defensive"); });
  grid->AddView(btnDefensive, 2, 0);

  btnAttacking->SetToggleable(true);
  btnBalanced->SetToggleable(true);
  btnDefensive->SetToggleable(true);
  btnAttacking->SetToggled(curStrat == "Attacking");
  btnBalanced->SetToggled(curStrat == "Balanced");
  btnDefensive->SetToggled(curStrat == "Defensive");

  grid->UpdateLayout(0.5);
  this->AddView(grid);
  grid->Show();

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_st_back", 30, 90, 40, 3, TR("career_back_hub"));
  btnBack->sig_OnClick.connect([this](...) { CreatePage(GetHubPageID()); });
  this->AddView(btnBack);
  btnBack->Show();
  btnAttacking->SetFocus();

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
  std::string clubTitle = activeSave ? (activeSave->name + " - SQUAD MANAGEMENT & ROSTER") : TR("career_squad_title");

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_squad", 4, 2, 90, 3, clubTitle);
  title->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  this->AddView(title);
  title->Show();

  Gui2Button* firstPlayerButton = nullptr;
  if (activeSave) {
    long long totalWage = 0;
    for (const auto& p : activeSave->roster) {
      totalWage += p.wage;
    }

    std::string headerText = "POS | PLAYER NAME                       | OVR/POT | AGE | VALUE / WAGE       | COND  | MORALE | CONTRACT";
    Gui2Caption* header = new Gui2Caption(windowManager, "caption_squad_header", 4, 5.5f, 90, 2.2f, headerText);
    header->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright1));
    this->AddView(header);
    header->Show();

    Gui2Caption* squadHint =
        new Gui2Caption(windowManager, "caption_squad_hint", 4, 8, 90, 2,
                        "Select any player to inspect full attributes, offer contract extension, transfer-list, or release.");
    this->AddView(squadHint);
    squadHint->Show();

    Gui2Grid* grid = new Gui2Grid(windowManager, "squad_grid", 3, 11, 92, 70);
    int row = 0;
    const int maxRosterRows = 18;
    for (const auto& player : activeSave->roster) {
      if (row >= maxRosterRows)
        break;

      std::string condArrow = CareerDatabase::GetInstance().GetConditionArrow(player.matchForm);
      std::string moraleStr = CareerDatabase::GetInstance().GetMoraleString(player.morale);
      std::string listedTag = player.contract.transferListed ? " [LISTED]" : "";

      char rowBuf[512];
      snprintf(rowBuf, sizeof(rowBuf), "%-4s %-28s %2d/%-2d  %2d   %-8s %-7s %-7s %-7s %dyrs%s",
               player.preferredPosition.c_str(),
               player.name.c_str(),
               player.ovr, player.pot,
               player.age,
               FormatCareerMoney(player.value).c_str(),
               FormatCareerMoney(player.wage).c_str(),
               condArrow.c_str(),
               moraleStr.c_str(),
               player.contract.yearsRemaining,
               listedTag.c_str());

      Gui2Button* btn =
          new Gui2Button(windowManager, "btn_player_" + std::to_string(row), 0, 0, 88, 2.4f, std::string(rowBuf));
      if (!firstPlayerButton)
        firstPlayerButton = btn;

      btn->sig_OnClick.connect([this, player](...) { InspectPlayer(player); });
      grid->AddView(btn, row++, 0);
    }
    grid->UpdateLayout(0.5f, 0.5f, 0.2f, 0.2f);
    this->AddView(grid);
    grid->Show();

    std::string footerText = "Total Squad Size: " + std::to_string(activeSave->roster.size()) + " Players | Weekly Wage Bill: " +
                             FormatCareerMoney(totalWage) + " / " + FormatCareerMoney(activeSave->wageBudget) +
                             " | Available Wage Margin: " + FormatCareerMoney(std::max(0LL, activeSave->wageBudget - totalWage));
    Gui2Caption* footer = new Gui2Caption(windowManager, "caption_squad_footer", 4, 83, 90, 2.2f, footerText);
    this->AddView(footer);
    footer->Show();
  }

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_squad_back", 30, 88, 40, 3, "🔙 Back to Career Hub");
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
  Gui2Dialog* dialog = new Gui2Dialog(windowManager, "dialog_inspect_player", 15, 15, 70, 70,
                                      "PLAYER PROFILE & ACTIONS | " + player.name);

  std::string condArrow = CareerDatabase::GetInstance().GetConditionArrow(player.matchForm);
  std::string moraleStr = CareerDatabase::GetInstance().GetMoraleString(player.morale);
  std::string statusTag = player.contract.transferListed ? "Transfer Listed" : "Squad Member";

  // Estimated FIFA/PES radar stats based on OVR and position
  int pace = std::min(99, player.ovr + (player.age < 25 ? 4 : -2));
  int shoot = player.preferredPosition == "CF" || player.preferredPosition == "ST" || player.preferredPosition == "FW" ? player.ovr + 5 : player.ovr - 8;
  int pass = player.preferredPosition == "CM" || player.preferredPosition == "AM" || player.preferredPosition == "DM" ? player.ovr + 4 : player.ovr - 4;
  int dribble = std::min(99, player.ovr + 2);
  int defend = player.preferredPosition == "CB" || player.preferredPosition == "LB" || player.preferredPosition == "RB" ? player.ovr + 6 : player.ovr - 12;
  int physical = std::min(99, player.ovr + 1);

  char profileBuf[1024];
  snprintf(profileBuf, sizeof(profileBuf),
           "Position: %s  |  Overall Rating: %d OVR  |  Potential: %d POT  |  Age: %d Years\n"
           "Market Value: %s  |  Weekly Wage: %s  |  Contract: %d Years Left (%s)\n"
           "Match Form / Condition: %s  |  Morale: %s (%d/100)\n\n"
           "ATTRIBUTE BREAKDOWN:\n"
           "• Pace: %d   • Shooting: %d   • Passing: %d\n"
           "• Dribbling: %d   • Defending: %d   • Physical: %d",
           player.preferredPosition.c_str(), player.ovr, player.pot, player.age,
           FormatCareerMoney(player.value).c_str(), FormatCareerMoney(player.wage).c_str(),
           player.contract.yearsRemaining, statusTag.c_str(),
           condArrow.c_str(), moraleStr.c_str(), player.morale,
           pace, shoot, pass, dribble, defend, physical);

  Gui2Caption* profileCaption = new Gui2Caption(windowManager, "cap_player_detail", 4, 8, 62, 32, std::string(profileBuf));
  dialog->AddView(profileCaption);
  profileCaption->Show();

  Gui2Grid* actionGrid = new Gui2Grid(windowManager, "grid_player_actions", 4, 42, 62, 24);

  Gui2Button* btnExtend = new Gui2Button(windowManager, "btn_extend_contract", 0, 0, 58, 2.5f,
                                         "📝 Offer Contract Extension (+2 Years, +10% Wage)");
  std::string pName = player.name;
  btnExtend->sig_OnClick.connect([this, pName, dialog](...) {
    dialog->Exit();
    delete dialog;
    ExtendContract(pName);
  });
  actionGrid->AddView(btnExtend, 0, 0);

  std::string listLabel = player.contract.transferListed ? "💼 Remove from Transfer List" : "💼 Place on Transfer List";
  Gui2Button* btnToggleList = new Gui2Button(windowManager, "btn_toggle_list", 0, 0, 58, 2.5f, listLabel);
  btnToggleList->sig_OnClick.connect([this, pName, dialog](...) {
    dialog->Exit();
    delete dialog;
    ToggleTransferList(pName);
  });
  actionGrid->AddView(btnToggleList, 1, 0);

  Gui2Button* btnRelease = new Gui2Button(windowManager, "btn_release_action", 0, 0, 58, 2.5f,
                                          "❌ Release Player from Club");
  btnRelease->sig_OnClick.connect([this, pName, dialog](...) {
    dialog->Exit();
    delete dialog;
    ReleasePlayer(pName);
  });
  actionGrid->AddView(btnRelease, 2, 0);

  Gui2Button* btnClose = new Gui2Button(windowManager, "btn_close_profile", 0, 0, 58, 2.5f,
                                        "🔙 Close Player Profile");
  btnClose->sig_OnClick.connect([dialog](...) {
    dialog->Exit();
    delete dialog;
  });
  actionGrid->AddView(btnClose, 3, 0);

  actionGrid->UpdateLayout(0.5f, 0.5f, 0.2f, 0.2f);
  dialog->AddView(actionGrid);
  actionGrid->Show();

  btnExtend->SetFocus();
  this->AddView(dialog);
  dialog->Show();
}

void CareerSquadRosterPage::ExtendContract(const std::string& playerName) {
  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
  if (save) {
    for (auto& p : save->roster) {
      if (p.name == playerName) {
        p.contract.yearsRemaining += 2;
        p.wage = p.wage * 110 / 100;
        p.morale = std::min(100, p.morale + 20);
        CareerDatabase::GetInstance().AddEvent("Contract Extension",
            p.name + " signed a 2-year contract extension with a 10% wage increase.", 3, false);
        break;
      }
    }
  }
  CreatePage(e_PageID_CareerSquadRoster);
}

void CareerSquadRosterPage::ToggleTransferList(const std::string& playerName) {
  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
  if (save) {
    for (auto& p : save->roster) {
      if (p.name == playerName) {
        p.contract.transferListed = !p.contract.transferListed;
        CareerDatabase::GetInstance().AddEvent("Transfer Status",
            p.name + (p.contract.transferListed ? " placed on transfer list." : " removed from transfer list."), 0, false);
        break;
      }
    }
  }
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
  dialog->sig_OnNegative.connect([dialog](...) {
    dialog->Exit();
    delete dialog;
  });
  this->AddView(dialog);
  dialog->Show();
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
        new Gui2Frame(windowManager, "frame_season_summary", 4, 12, 84, 12, true);
    std::string summary =
        TRF("career_season_summary_mode", {GetCareerModeDisplay(activeSave)}) + "\n" +
        TRF("career_season_summary_budgets", {FormatCareerMoney(activeSave->transferBudget),
                                              FormatCareerMoney(activeSave->wageBudget)}) +
        "\n" +
        TRF("career_season_summary_squad", {std::to_string(activeSave->roster.size()),
                                            std::to_string(activeSave->youthAcademy.size())});
    if (activeSave->mode == CareerMode::OWNER) {
      summary += "\n" + TRF("career_season_summary_owner",
                            {FormatCareerMoney(activeSave->finances.netWorth),
                             FormatCareerMoney(CareerDatabase::GetInstance().GetSeasonProfit())});
    }
    Gui2Caption* summaryCap =
        new Gui2Caption(windowManager, "caption_season_summary", 2, 2, 80, 8, summary);
    summaryFrame->AddView(summaryCap);
    summaryCap->Show();
    bgPanel->AddView(summaryFrame);
    summaryFrame->Show();

    Gui2Caption* progress = new Gui2Caption(windowManager, "caption_season_progress", 6, 24, 82, 2,
                                            BuildSeasonProgressLine(activeSave));
    bgPanel->AddView(progress);
    progress->Show();

    const bool earlyAdvance = activeSave->season.currentWeek < activeSave->season.maxWeeks;
    std::string warningText;
    if (earlyAdvance) {
      warningText = TRF("career_season_early_warn", {std::to_string(activeSave->season.currentWeek),
                                                     std::to_string(activeSave->season.maxWeeks)});
    } else if (activeSave->mode == CareerMode::OWNER) {
      warningText = TR("career_season_owner_proceed");
    } else {
      warningText = TR("career_season_proceed");
    }
    Gui2Caption* warning =
        new Gui2Caption(windowManager, "caption_season_warn", 6, 27, 82, 4, warningText);
    bgPanel->AddView(warning);
    warning->Show();

    if (activeSave->mode == CareerMode::OWNER) {
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

  Gui2Button* btnAdvance =
      new Gui2Button(windowManager, "btn_season_advance", 22, 80, 48, 4,
                     TR(IsOwnerMode() ? "career_season_advance_owner" : "career_season_advance"));
  btnAdvance->sig_OnClick.connect([this](...) { AdvanceSeason(); });
  bgPanel->AddView(btnAdvance);
  btnAdvance->Show();
  btnAdvance->SetFocus();

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_season_back", 30, 87, 30, 3, TR("career_back_hub"));
  btnBack->sig_OnClick.connect([this](...) { GoToHub(); });
  bgPanel->AddView(btnBack);
  btnBack->Show();

  this->Show();
}

CareerSeasonPage::~CareerSeasonPage() {}

void CareerSeasonPage::AdvanceSeason() {
  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
  if (save && save->mode == CareerMode::OWNER) {
    CareerDatabase::GetInstance().ProcessSeasonFinances();
  }
  // Record the closed season into history first so board evaluation can read
  // the finish that was just earned (not the previous season's).
  CareerDatabase::GetInstance().AdvanceSeason();
  if (save && save->mode == CareerMode::OWNER) {
    CareerDatabase::GetInstance().EvaluateBoardObjectives();
    CareerDatabase::GetInstance().GenerateSponsorOffers();
    CareerDatabase::GetInstance().GenerateBoardObjectives();
  }

  if (IsOwnerMode()) {
    CreatePage(e_PageID_OwnerHub);
  } else {
    CreatePage(e_PageID_CareerHub);
  }
}

void CareerSeasonPage::GoToHub() {
  if (IsOwnerMode()) {
    CreatePage(e_PageID_OwnerHub);
  } else {
    CreatePage(e_PageID_CareerHub);
  }
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
}

void CareerMatchdayPage::BuildFixtures() {
  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
  if (!save)
    return;

  static const std::vector<std::string> opponentNames = {
      "FC United",     "Athletic Club", "Wanderers FC",      "Real Deportivo", "Inter Milano",
      "Bayern Munich", "FC Barcelona",  "Chelsea FC",        "Arsenal FC",     "Juventus Turin",
      "AC Milan",      "Liverpool FC",  "Borussia Dortmund", "Paris SG",       "Ajax Amsterdam",
      "Porto FC",      "Benfica",       "Sporting CP",       "Napoli",         "Atletico Madrid",
      "Tottenham"};

  // One fixture per matchday visit — this is the next league match, not a
  // random multi-game block.
  const int numFixtures = 1;

  m_opponents.clear();
  m_isHome.clear();
  m_results.clear();
  fixtureScoreCaps.assign(numFixtures, nullptr);

  for (int i = 0; i < numFixtures; i++) {
    int opponentIdx = (m_week * 3 + i) % static_cast<int>(opponentNames.size());
    m_opponents.push_back(opponentNames[opponentIdx]);
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
      new Gui2Button(windowManager, "btn_md_playtop", 0, 0, 42, 2.8f, "🎮 PLAY 3D MATCH");
  btnPlayTop->sig_OnClick.connect([this](...) { PlayMatch(); });
  fixtureGrid->AddView(btnPlayTop, row, 0);

  Gui2Button* btnSimAllTop =
      new Gui2Button(windowManager, "btn_md_simalltop", 0, 0, 42, 2.8f, "⚡ QUICK SIMULATE MATCH");
  btnSimAllTop->sig_OnClick.connect([this](...) { SimulateAll(); });
  fixtureGrid->AddView(btnSimAllTop, row++, 1);

  int numFixtures = static_cast<int>(m_opponents.size());
  for (int i = 0; i < numFixtures; i++) {
    const auto& res = m_results[i];
    const bool isHome = (i < static_cast<int>(m_isHome.size())) ? m_isHome[i] : true;
    const std::string venue = isHome ? ("HOME (" + save->name + " Stadium)") : ("AWAY (" + m_opponents[i] + " Ground)");

    // Atmosphere & Matchday Header
    std::string atmosphere = "Atmosphere: Weather 19°C (Clear) | Pitch: Pristine | Attendance: 96% Capacity";
    Gui2Caption* header =
        new Gui2Caption(windowManager, "cap_md_hdr_" + std::to_string(i), 0, 0, 84, 2.2f,
                        "🏆 FIXTURE | " + venue + " | " + save->name + " vs " + m_opponents[i] + "\n" + atmosphere);
    header->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
    fixtureGrid->AddView(header, row++, 0);

    std::string scoreLabel = "STATUS: Ready for Kick-off | Tactics: Balanced Press | Form: " + CareerDatabase::GetInstance().GetFormGuideString(5);
    if (res.played) {
      if (isHome) {
        scoreLabel =
            "FINAL SCORE: " + save->name + " " + std::to_string(res.homeGoals) + " - " +
            std::to_string(res.awayGoals) + " " + m_opponents[i] + (res.homeGoals > res.awayGoals ? " [VICTORY!]" : (res.homeGoals == res.awayGoals ? " [DRAW]" : " [DEFEAT]"));
      } else {
        scoreLabel =
            "FINAL SCORE: " + m_opponents[i] + " " + std::to_string(res.awayGoals) + " - " +
            std::to_string(res.homeGoals) + " " + save->name + (res.homeGoals > res.awayGoals ? " [VICTORY!]" : (res.homeGoals == res.awayGoals ? " [DRAW]" : " [DEFEAT]"));
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
        scorersStr = "⚽ Goals: " + res.scorers[0];
        for (int s = 1; s < static_cast<int>(res.scorers.size()); s++) {
          scorersStr += ", " + res.scorers[s];
        }
      } else {
        scorersStr = "⚽ Goals: No goals scored (Scoreless Draw / Defensive Battle)";
      }

      int userShots = isHome ? res.homeShots : res.awayShots;
      int oppShots = isHome ? res.awayShots : res.homeShots;
      int userPoss = isHome ? res.homePossession : (100 - res.homePossession);
      int oppPoss = 100 - userPoss;
      int corners = 3 + (userShots % 5);
      int fouls = 6 + (oppShots % 7);
      float motmRating = 7.5f + static_cast<float>(std::max(res.homeGoals, res.awayGoals)) * 0.5f;
      if (motmRating > 9.8f) motmRating = 9.8f;

      std::string motmPlayer = !res.scorers.empty() ? res.scorers[0] : (!save->roster.empty() ? save->roster[0].name : "Goalkeeper");

      char statsBuf[512];
      snprintf(statsBuf, sizeof(statsBuf),
               "MATCH SUMMARY & PERFORMANCE:\n"
               "%s\n"
               "Shots (On Target): %d (%d) vs %d (%d) | Possession: %d%% vs %d%%\n"
               "Corner Kicks: %d - %d | Fouls Committed: %d - %d | Pass Accuracy: 86%%\n"
               "★ Man of the Match: %s (Rating: %.1f / 10.0)",
               scorersStr.c_str(),
               userShots, std::max(1, userShots * 6 / 10), oppShots, std::max(1, oppShots * 6 / 10),
               userPoss, oppPoss,
               corners, 4, fouls, 8,
               motmPlayer.c_str(), motmRating);

      Gui2Caption* statsCap =
          new Gui2Caption(windowManager, "cap_md_stats_" + std::to_string(i), 0, 0, 84, 10, std::string(statsBuf));
      fixtureGrid->AddView(statsCap, row++, 0);
    }

    if (!res.played) {
      Gui2Button* btnSim = new Gui2Button(windowManager, "btn_md_sim_" + std::to_string(i), 0, 0,
                                          42, 2.5f, "⚡ Quick Sim Match");
      btnSim->sig_OnClick.connect([this, i](...) { SimulateMatch(i); });
      fixtureGrid->AddView(btnSim, row, 0);

      Gui2Button* btnPlay = new Gui2Button(windowManager, "btn_md_play_" + std::to_string(i), 0, 0,
                                           42, 2.5f, "🎮 Play 3D Match");
      btnPlay->sig_OnClick.connect([this, i](...) { PlayMatchFixture(i); });
      fixtureGrid->AddView(btnPlay, row++, 1);
    }
  }

  // Last row: Back to Hub, reachable by the same navigation as everything else.
  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_matchday_back", 0, 0, 42, 2.5f, "🔙 Back to Career Hub");
  btnBack->sig_OnClick.connect([this](...) { GoBack(); });
  fixtureGrid->AddView(btnBack, row++, 0);

  fixtureGrid->UpdateLayout(0.5f, 0.5f, 0.25f, 0.25f);
  frame->AddView(fixtureGrid);
  fixtureGrid->Show();

  btnPlayTop->SetFocus();

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
      m_opponents[fixtureIndex], std::to_string(save ? save->club.clubID : 0), isHome);
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

  if (save) {
    CareerDatabase::GetInstance().ApplyMatchResult(res.homeGoals, res.awayGoals,
                                                   m_opponents[fixtureIndex], res.scorers);
  }

  PopulateGrid();
}

void CareerMatchdayPage::SimulateAll() {
  for (int i = 0; i < static_cast<int>(m_results.size()); i++) {
    if (!m_results[i].played)
      SimulateMatch(i);
  }
}

void CareerMatchdayPage::PlayMatch() {
  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
  if (!save)
    return;
  if (save->club.clubID <= 0)
    return;
  if (m_opponents.empty())
    BuildFixtures();

  int teamDBID = save->club.clubID;
  if (teamDBID <= 0)
    teamDBID = 1;

  // Find a valid opponent (try IDs until one exists)
  int opponentDBID = 1;
  for (int i = 0; i < 20; i++) {
    int testID = ((m_week * 3 + i + 1) % 20) + 1;
    if (testID != teamDBID) {
      try {
        auto result = GetDB()->Query("SELECT id FROM teams WHERE id = " + int_to_str(testID));
        if (!result->data.empty()) {
          opponentDBID = testID;
          break;
        }
      } catch (...) {
      }
    }
  }

  std::vector<SideSelection> sides(1);
  sides[0].controllerID = 0;
  sides[0].side = -1;
  GetMenuTask()->SetControllerSetup(sides);
  GetMenuTask()->SetTeamIDs(std::to_string(teamDBID), std::to_string(opponentDBID));

  Properties props;
  windowManager->GetPageFactory()->CreatePage((int)e_PageID_MatchOptions, props, 0);
}

void CareerMatchdayPage::PlayMatchFixture(int fixtureIndex) {
  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
  if (!save || fixtureIndex < 0 || fixtureIndex >= static_cast<int>(m_opponents.size()))
    return;
  if (save->club.clubID <= 0)
    return;

  int teamDBID = save->club.clubID;
  if (teamDBID <= 0)
    teamDBID = 1;

  // Validate opponent exists in database
  int opponentDBID = 1;
  for (int i = 0; i < 20; i++) {
    int testID = ((m_week * 7 + fixtureIndex + i + 1) % 20) + 1;
    if (testID != teamDBID) {
      try {
        auto result = GetDB()->Query("SELECT id FROM teams WHERE id = " + int_to_str(testID));
        if (!result->data.empty()) {
          opponentDBID = testID;
          break;
        }
      } catch (...) {
      }
    }
  }

  std::vector<SideSelection> sides(1);
  sides[0].controllerID = 0;
  sides[0].side = -1;
  GetMenuTask()->SetControllerSetup(sides);
  GetMenuTask()->SetTeamIDs(std::to_string(teamDBID), std::to_string(opponentDBID));

  Properties props;
  windowManager->GetPageFactory()->CreatePage((int)e_PageID_MatchOptions, props, 0);
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
  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
  // Match results are already applied to season W/D/L in SimulateMatch /
  // ApplyMatchResult. Only advance the calendar week here to avoid double-counting.
  if (save && m_matchesPlayed > 0) {
    save->season.currentWeek++;
    CareerDatabase::GetInstance().SaveCareerData();
  }
  // CreatePage already Exit()s and deletes this page — do not delete again.
  CreatePage(GetHubPageID());
}

// ---------------------------------------------------------------------------
// CareerMatchdayPage - 3D match result bookkeeping
// ---------------------------------------------------------------------------

void CareerMatchdayPage::Process3DMatchResult(int homeGoals, int awayGoals) {
  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
  if (!save)
    return;
  if (save->club.clubID <= 0)
    return;

  CareerDatabase::GetInstance().ApplyMatchResult(homeGoals, awayGoals, "(3D match)");
}
