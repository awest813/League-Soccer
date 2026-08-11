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
#include "utils/localization.hpp"

using namespace blunted;

namespace {

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
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* bgPanel = new Gui2Frame(windowManager, "bg_career_menu", 6, 6, 88, 88, true);
  this->AddView(bgPanel);
  bgPanel->Show();
  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_career", 10, 8, 68, 4, TR("career_menu_title"));
  this->AddView(title);
  title->Show();

  Gui2Caption* subtitle = new Gui2Caption(windowManager, "caption_career_sub", 10, 13, 74, 4,
                                          TR("career_menu_subtitle"));
  this->AddView(subtitle);
  subtitle->Show();

  const bool continueFailed =
      pageData.properties && pageData.properties->GetBool("continueFailed", false);
  if (continueFailed) {
    Gui2Caption* failLine = new Gui2Caption(windowManager, "caption_career_continue_fail", 10, 17,
                                            74, 3, TR("career_menu_continue_failed"));
    this->AddView(failLine);
    failLine->Show();
  }

  Gui2Button* btnCoach =
      new Gui2Button(windowManager, "btn_mycoach", 0, 0, 34, 5, TR("career_menu_coach"));
  Gui2Button* btnGM = new Gui2Button(windowManager, "btn_mygm", 0, 0, 34, 5, TR("career_menu_gm"));
  Gui2Button* btnPlayer =
      new Gui2Button(windowManager, "btn_playercareer", 0, 0, 34, 5, TR("career_menu_player"));
  Gui2Button* btnManager =
      new Gui2Button(windowManager, "btn_managercareer", 0, 0, 34, 5, TR("career_menu_manager"));
  Gui2Button* btnOwner =
      new Gui2Button(windowManager, "btn_ownercareer", 0, 0, 34, 5, TR("career_menu_owner"));

  btnCoach->sig_OnClick.connect([this](...) { GoMyCoach(); });
  btnGM->sig_OnClick.connect([this](...) { GoMyGM(); });
  btnPlayer->sig_OnClick.connect([this](...) { GoPlayerCareer(); });
  btnManager->sig_OnClick.connect([this](...) { GoManagerCareer(); });
  btnOwner->sig_OnClick.connect([this](...) { GoOwnerCareer(); });

  Gui2Grid* grid = new Gui2Grid(windowManager, "career_grid", 10, 22, 72, 56);
  grid->AddView(btnCoach, 0, 0);
  grid->AddView(btnGM, 0, 1);
  grid->AddView(btnPlayer, 1, 0);
  grid->AddView(btnManager, 1, 1);
  grid->AddView(btnOwner, 2, 0);
  grid->UpdateLayout(0.5);

  this->AddView(grid);
  grid->Show();

  Gui2Caption* footer = new Gui2Caption(windowManager, "caption_career_footer", 10, 82, 72, 4,
                                        TR("career_menu_footer"));
  this->AddView(footer);
  footer->Show();

  CareerDatabase::GetInstance().Initialize("user/career");
  const bool hasSave = CareerDatabase::GetInstance().HasSaveFile();

  Gui2Button* btnContinue =
      new Gui2Button(windowManager, "btn_continue", 0, 0, 34, 4,
                     TR(hasSave ? "career_menu_continue" : "career_menu_continue_empty"));
  btnContinue->sig_OnClick.connect([this](...) { GoContinueCareer(); });
  grid->AddView(btnContinue, 3, 0);
  grid->UpdateLayout(0.5);

  Gui2Button* btnBack = new Gui2Button(windowManager, "btn_career_menu_back", 10, 88, 34, 3,
                                       TR("career_menu_back_main"));
  btnBack->sig_OnClick.connect([this](...) { CreatePage(e_PageID_MainMenu); });
  this->AddView(btnBack);
  btnBack->Show();

  if (hasSave && !continueFailed)
    btnContinue->SetFocus();
  else
    btnCoach->SetFocus();
  this->Show();
}

CareerMenuPage::~CareerMenuPage() {}

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
  Gui2Frame* bgPanel = new Gui2Frame(windowManager, "bg_career_new", 5, 5, 90, 90, true);
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

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_newgame", 20, 10, 60, 3,
                                       TRF("career_new_mode_title", {modeLabel}));
  this->AddView(title);
  title->Show();

  Gui2Caption* setupHint = new Gui2Caption(windowManager, "caption_newgame_hint", 12, 14, 76, 3,
                                           TR("career_new_mode_hint"));
  this->AddView(setupHint);
  setupHint->Show();

  Gui2Caption* teamCaption = new Gui2Caption(windowManager, "caption_newgame_team", 10, 20, 30, 2.5,
                                             TR("career_new_select_team"));
  this->AddView(teamCaption);
  teamCaption->Show();

  teamSelectPulldown = new Gui2Pulldown(windowManager, "pulldown_career_teamselect", 40, 20, 30, 3);
  RefreshTeamSelect();
  teamSelectPulldown->sig_OnChange.connect(
      [this](Gui2Pulldown* pd) { m_selectedTeamID = pd->GetSelected(); });
  this->AddView(teamSelectPulldown);
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
      new Gui2Caption(windowManager, "caption_newgame_mgr", 10, 28, 30, 2.5, nameFieldLabel);
  this->AddView(mgrCaption);
  mgrCaption->Show();

  managerNameInput =
      new Gui2EditLine(windowManager, "editline_career_mgrname", 40, 28, 30, 3, nameDefault);
  managerNameInput->SetMaxLength(32);
  this->AddView(managerNameInput);
  managerNameInput->Show();

  Gui2Button* btnStart =
      new Gui2Button(windowManager, "btn_start_career", 30, 50, 40, 3, TR("career_new_start"));
  btnStart->sig_OnClick.connect([this](...) { StartCareer(); });
  this->AddView(btnStart);
  btnStart->Show();
  btnStart->SetFocus();

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_newgame_back", 30, 56, 40, 3, TR("career_new_back_modes"));
  btnBack->sig_OnClick.connect([this](...) { CreatePage(e_PageID_CareerMenu); });
  this->AddView(btnBack);
  btnBack->Show();

  this->Show();
}

CareerNewGamePage::~CareerNewGamePage() {}

void CareerNewGamePage::RefreshTeamSelect() {
  teamSelectPulldown->ClearEntries();
  try {
    auto result = GetDB()->Query(
        "SELECT teams.id, teams.name, leagues.name FROM teams "
        "JOIN leagues ON teams.league_id = leagues.id ORDER BY leagues.name, teams.name");
    for (unsigned int r = 0; r < result->data.size(); r++) {
      std::string id = result->data.at(r).at(0);
      std::string teamName = result->data.at(r).at(1);
      std::string leagueName = result->data.at(r).at(2);
      teamSelectPulldown->AddEntry(teamName + " (" + leagueName + ")", id);
    }
  } catch (...) {
    teamSelectPulldown->AddEntry(TR("career_new_no_teams"), "0");
  }
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
  Gui2Frame* bgPanel = new Gui2Frame(windowManager, "bg_career_hub", 5, 0, 90, 100, true);
  this->AddView(bgPanel);
  bgPanel->Show();
  if (IsOwnerMode()) {
    CreatePage(e_PageID_OwnerHub);
    return;
  }

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_careerhub", 20, 5, 60, 3, TR("career_hub_title"));
  this->AddView(title);
  title->Show();

  Gui2Button* btnTransfers =
      new Gui2Button(windowManager, "btn_transfers", 0, 0, 38, 3, TR("career_tm_title"));
  Gui2Button* btnFreeAgency =
      new Gui2Button(windowManager, "btn_freeagency", 0, 0, 38, 3, TR("career_fa_title"));
  Gui2Button* btnSquad =
      new Gui2Button(windowManager, "btn_squad", 0, 0, 38, 3, TR("career_squad_title"));
  Gui2Button* btnTraining =
      new Gui2Button(windowManager, "btn_training", 0, 0, 38, 3, TR("career_training_title"));
  Gui2Button* btnStrategy =
      new Gui2Button(windowManager, "btn_strategy", 0, 0, 38, 3, TR("career_strategy_title"));
  Gui2Button* btnYouth =
      new Gui2Button(windowManager, "btn_youth", 0, 0, 38, 3, TR("career_youth_title"));
  Gui2Button* btnPressConf =
      new Gui2Button(windowManager, "btn_pressconf", 0, 0, 38, 3, TR("career_press_title"));
  Gui2Button* btnLeagueExp =
      new Gui2Button(windowManager, "btn_leagueexp", 0, 0, 38, 3, TR("career_leagueexp_title"));
  Gui2Button* btnCustomLeague = new Gui2Button(windowManager, "btn_customleague", 0, 0, 38, 3,
                                               TR("career_customleague_title"));
  Gui2Button* btnSeason =
      new Gui2Button(windowManager, "btn_season_end", 0, 0, 38, 3, TR("career_season_title"));
  Gui2Button* btnMatchday =
      new Gui2Button(windowManager, "btn_matchday", 0, 0, 38, 3, TR("career_matchday_title"));
  Gui2Button* btnExit =
      new Gui2Button(windowManager, "btn_hub_exit", 0, 0, 38, 3, TR("career_menu_back_modes"));

  btnTransfers->sig_OnClick.connect([this](...) { GoTransferMarket(); });
  btnFreeAgency->sig_OnClick.connect([this](...) { GoFreeAgency(); });
  btnSquad->sig_OnClick.connect([this](...) { GoSquad(); });
  btnTraining->sig_OnClick.connect([this](...) { GoTraining(); });
  btnStrategy->sig_OnClick.connect([this](...) { GoStrategy(); });
  btnYouth->sig_OnClick.connect([this](...) { GoYouthAcademy(); });
  btnPressConf->sig_OnClick.connect([this](...) { GoPressConference(); });
  btnLeagueExp->sig_OnClick.connect([this](...) { GoLeagueExpansion(); });
  btnCustomLeague->sig_OnClick.connect([this](...) { GoCustomLeague(); });
  btnSeason->sig_OnClick.connect([this](...) { GoSeason(); });
  btnMatchday->sig_OnClick.connect([this](...) { GoMatchday(); });
  btnExit->sig_OnClick.connect([this](...) { CreatePage(e_PageID_CareerMenu); });

  CareerSave* activeSave = CareerDatabase::GetInstance().GetActiveSave();
  if (activeSave) {
    Gui2Caption* teamLabel = new Gui2Caption(
        windowManager, "caption_hub_team", 10, 8, 80, 2,
        TRF("career_hub_team_line",
            {GetCareerModeDisplay(activeSave), activeSave->name, activeSave->club.leagueName}));
    this->AddView(teamLabel);
    teamLabel->Show();

    std::string finInfo = TRF("career_hub_fin_line", {FormatCareerMoney(activeSave->transferBudget),
                                                      FormatCareerMoney(activeSave->wageBudget)});
    Gui2Caption* finances =
        new Gui2Caption(windowManager, "caption_hub_fin", 10, 10, 80, 2, finInfo);
    this->AddView(finances);
    finances->Show();

    std::string repInfo =
        TRF("career_hub_rep_line", {std::to_string(activeSave->boardConfidence),
                                    CareerDatabase::GetInstance().GetReputationStatus(),
                                    std::to_string(activeSave->season.currentSeason)});
    Gui2Caption* reputation =
        new Gui2Caption(windowManager, "caption_hub_rep", 10, 12, 80, 2, repInfo);
    this->AddView(reputation);
    reputation->Show();

    Gui2Caption* progress = new Gui2Caption(windowManager, "caption_hub_progress", 10, 14, 80, 2,
                                            BuildSeasonProgressLine(activeSave));
    this->AddView(progress);
    progress->Show();

    std::string squadInfo =
        TRF("career_hub_squad_line",
            {std::to_string(activeSave->roster.size()), std::to_string(activeSave->trainingPoints),
             std::to_string(activeSave->youthAcademy.size()), activeSave->activeStrategy});
    Gui2Caption* squad =
        new Gui2Caption(windowManager, "caption_hub_squad", 10, 16, 80, 2, squadInfo);
    this->AddView(squad);
    squad->Show();
  }

  Gui2Grid* grid = new Gui2Grid(windowManager, "hub_grid", 10, 20, 80, 68);
  // Primary loop actions first: Matchday, then squad tools, then admin.
  grid->AddView(btnMatchday, 0, 0);
  grid->AddView(btnSeason, 1, 0);
  grid->AddView(btnSquad, 2, 0);
  grid->AddView(btnStrategy, 3, 0);
  grid->AddView(btnTraining, 4, 0);
  grid->AddView(btnYouth, 5, 0);
  grid->AddView(btnTransfers, 0, 1);
  grid->AddView(btnFreeAgency, 1, 1);
  grid->AddView(btnPressConf, 2, 1);
  grid->AddView(btnLeagueExp, 3, 1);
  grid->AddView(btnCustomLeague, 4, 1);
  grid->AddView(btnExit, 5, 1);
  grid->UpdateLayout(0.5);

  this->AddView(grid);
  grid->Show();

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
  for (const auto& t : targets) {
    if (row >= 18)
      break;
    const std::string rowLabel =
        TRF("career_tm_row", {t.name, t.preferredPosition, std::to_string(t.overallRating),
                              std::to_string(t.potentialRating), std::to_string(t.age),
                              FormatCareerMoney(t.value), FormatCareerMoney(t.askingPrice)});
    Gui2Button* btn =
        new Gui2Button(windowManager, "btn_tm_" + std::to_string(row), 0, 0, 90, 2.5, rowLabel);
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
  btnBack->SetFocus();

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
      } else if (b.status == BidStatus::PENDING) {
        std::string pName = b.playerName;
        btn->sig_OnClick.connect([this, pName](...) { NegotiateBid(pName); });
      }
      grid->AddView(btn, row++, 0);
    }
    grid->UpdateLayout(0.5);
    bgPanel->AddView(grid);
    grid->Show();
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
  Gui2Frame* bgPanel = new Gui2Frame(windowManager, "bg_career_biddtl", 5, 0, 90, 100, true);
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
  for (const auto& t : targets) {
    if (t.name == m_playerName) {
      target = t;
      found = true;
      break;
    }
  }

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_detail_title", 6, 3, 82, 3,
                                       TRF("career_detail_title", {m_playerName}));
  bgPanel->AddView(title);
  title->Show();

  if (found) {
    std::string info1 = TRF("career_detail_info1",
                            {target.preferredPosition, std::to_string(target.overallRating),
                             std::to_string(target.potentialRating), std::to_string(target.age)});
    Gui2Caption* line1 =
        new Gui2Caption(windowManager, "caption_detail_info1", 6, 10, 82, 3, info1);
    bgPanel->AddView(line1);
    line1->Show();

    std::string info2 = TRF("career_detail_info2",
                            {FormatCareerMoney(target.value), FormatCareerMoney(target.askingPrice),
                             FormatCareerMoney(target.wage)});
    Gui2Caption* line2 =
        new Gui2Caption(windowManager, "caption_detail_info2", 6, 14, 82, 3, info2);
    bgPanel->AddView(line2);
    line2->Show();

    Gui2Grid* grid = new Gui2Grid(windowManager, "grid_detail", 10, 22, 80, 30);

    long long askPrice = target.askingPrice;
    Gui2Button* bidFull = new Gui2Button(windowManager, "btn_bid_full", 0, 0, 76, 3,
                                         TRF("career_bid_full", {FormatCareerMoney(askPrice)}));
    bidFull->sig_OnClick.connect([this, askPrice](...) { PlaceBidForPlayer(askPrice); });
    grid->AddView(bidFull, 0, 0);

    long long bid80 = target.askingPrice * 80 / 100;
    Gui2Button* bid80Button = new Gui2Button(
        windowManager, "btn_bid_80", 0, 0, 76, 3,
        TRF("career_bid_80", {FormatCareerMoney(bid80), TR("career_bid_reject_may")}));
    bid80Button->sig_OnClick.connect([this, bid80](...) { PlaceBidForPlayer(bid80); });
    grid->AddView(bid80Button, 1, 0);

    long long bid60 = target.askingPrice * 60 / 100;
    Gui2Button* bid60Button = new Gui2Button(
        windowManager, "btn_bid_60", 0, 0, 76, 3,
        TRF("career_bid_60", {FormatCareerMoney(bid60), TR("career_bid_reject_likely")}));
    bid60Button->sig_OnClick.connect([this, bid60](...) { PlaceBidForPlayer(bid60); });
    grid->AddView(bid60Button, 2, 0);

    grid->UpdateLayout(0.5);
    bgPanel->AddView(grid);
    grid->Show();

    CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
    if (save) {
      long long totalWithFee = target.askingPrice + (target.askingPrice / 20);
      std::string feeNote = TRF("career_detail_fee", {FormatCareerMoney(target.askingPrice / 20),
                                                      FormatCareerMoney(totalWithFee)});
      if (totalWithFee > save->transferBudget) {
        feeNote += " | " + TR("career_warn_exceeds_budget");
      }
      Gui2Caption* fee =
          new Gui2Caption(windowManager, "caption_detail_fee", 6, 56, 82, 4, feeNote);
      bgPanel->AddView(fee);
      fee->Show();
    }
  }

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_detail_back", 30, 85, 40, 3, TR("career_back_market"));
  btnBack->sig_OnClick.connect([this](...) { CreatePage(e_PageID_CareerTransferMarket); });
  bgPanel->AddView(btnBack);
  btnBack->Show();
  btnBack->SetFocus();

  this->Show();
}

CareerTransferBidDetailPage::~CareerTransferBidDetailPage() {}

void CareerTransferBidDetailPage::PlaceBidForPlayer(long long amount) {
  TransferBid bid = CareerDatabase::GetInstance().PlaceBid(m_playerName, amount,
                                                           static_cast<int>(m_playerWage), 3);
  if (bid.status == BidStatus::REJECTED) {
    Gui2Caption* warn = new Gui2Caption(windowManager, "caption_bid_warn", 10, 78, 80, 3,
                                        TR("career_bid_rejected"));
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
      new Gui2Caption(windowManager, "caption_pressconf", 6, 3, 82, 3, TR("career_press_title"));
  bgPanel->AddView(title);
  title->Show();

  CareerSave* activeSave = CareerDatabase::GetInstance().GetActiveSave();
  if (activeSave) {
    std::string context =
        TRF("career_press_context", {std::to_string(activeSave->season.currentSeason),
                                     CareerDatabase::GetInstance().GetReputationStatus(),
                                     std::to_string(activeSave->boardConfidence)});
    Gui2Caption* ctxLine = new Gui2Caption(windowManager, "caption_pc_ctx", 6, 7, 82, 2, context);
    bgPanel->AddView(ctxLine);
    ctxLine->Show();
  }

  Gui2Frame* questionFrame = new Gui2Frame(windowManager, "frame_pc_question", 6, 12, 84, 14, true);
  Gui2Caption* questionLabel = new Gui2Caption(windowManager, "caption_pc_q_label", 2, 1, 80, 2,
                                               TR("career_press_reporter"));
  questionFrame->AddView(questionLabel);
  questionLabel->Show();
  Gui2Caption* question = new Gui2Caption(windowManager, "caption_pc_question", 2, 4, 80, 8,
                                          TR("career_press_question"));
  questionFrame->AddView(question);
  question->Show();
  bgPanel->AddView(questionFrame);
  questionFrame->Show();

  Gui2Caption* answerHint = new Gui2Caption(windowManager, "caption_pc_answer_hint", 6, 28, 82, 2,
                                            TR("career_press_answer_hint"));
  bgPanel->AddView(answerHint);
  answerHint->Show();

  Gui2Button* btnPositive =
      new Gui2Button(windowManager, "btn_pc_positive", 0, 0, 76, 4, TR("career_press_positive"));
  Gui2Button* btnNeutral =
      new Gui2Button(windowManager, "btn_pc_neutral", 0, 0, 76, 4, TR("career_press_neutral"));
  Gui2Button* btnNegative =
      new Gui2Button(windowManager, "btn_pc_negative", 0, 0, 76, 4, TR("career_press_negative"));

  btnPositive->sig_OnClick.connect([this](...) { SelectAnswer(0); });
  btnNeutral->sig_OnClick.connect([this](...) { SelectAnswer(1); });
  btnNegative->sig_OnClick.connect([this](...) { SelectAnswer(2); });

  Gui2Grid* grid = new Gui2Grid(windowManager, "pc_grid", 8, 32, 76, 48);
  grid->AddView(btnPositive, 0, 0);
  grid->AddView(btnNeutral, 1, 0);
  grid->AddView(btnNegative, 2, 0);
  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_pc_back", 0, 0, 76, 3, TR("career_press_no_comment"));
  btnBack->sig_OnClick.connect([this](...) { CreatePage(GetHubPageID()); });
  grid->AddView(btnBack, 3, 0);
  grid->UpdateLayout(0.5);

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
  if (activeSave) {
    Gui2Grid* grid = new Gui2Grid(windowManager, "fa_grid", 10, 15, 80, 70);
    int row = 0;
    if (activeSave->freeAgents.empty()) {
      Gui2Caption* empty =
          new Gui2Caption(windowManager, "caption_fa_empty", 10, 20, 80, 4, TR("career_fa_empty"));
      this->AddView(empty);
      empty->Show();
    } else {
      for (const PlayerCareerState& fa : activeSave->freeAgents) {
        std::string label =
            TRF("career_fa_label", {fa.name, std::to_string(fa.ovr), FormatCareerMoney(fa.wage)});
        Gui2Button* btn = new Gui2Button(windowManager, "btn_recruit_" + fa.name, 0, 0, 76, 3,
                                         TRF("career_fa_recruit", {label}));
        btn->sig_OnClick.connect([this, fa](...) { RecruitPlayer(fa.name); });
        grid->AddView(btn, row++, 0);
      }
      grid->UpdateLayout(0.5);
      this->AddView(grid);
      grid->Show();
    }
  }

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_fa_back", 30, 90, 40, 3, TR("career_back_hub"));
  btnBack->sig_OnClick.connect([this](...) { CreatePage(GetHubPageID()); });
  this->AddView(btnBack);
  btnBack->Show();
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
  btnGeneral->SetFocus();

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
  if (activeSave) {
    Gui2Grid* grid = new Gui2Grid(windowManager, "ya_grid", 10, 15, 80, 60);
    int row = 0;

    int scoutCost = 50000 * activeSave->scoutingNetworkLevel;
    Gui2Button* btnScout =
        new Gui2Button(windowManager, "btn_scout_youth", 0, 0, 76, 3,
                       TRF("career_youth_scout", {FormatCareerMoney(scoutCost)}));
    btnScout->sig_OnClick.connect([this](...) { ScoutPlayer(); });
    grid->AddView(btnScout, row++, 0);

    if (activeSave->youthAcademy.empty()) {
      Gui2Caption* empty =
          new Gui2Caption(windowManager, "caption_ya_empty", 0, 0, 76, 3, TR("career_youth_empty"));
      grid->AddView(empty, row++, 0);
    } else {
      for (const PlayerCareerState& ya : activeSave->youthAcademy) {
        std::string label =
            TRF("career_youth_player",
                {ya.name, std::to_string(ya.age), std::to_string(ya.ovr), std::to_string(ya.pot)});
        Gui2Button* btn = new Gui2Button(windowManager, "btn_promote_" + ya.name, 0, 0, 76, 3,
                                         TRF("career_youth_promote", {label}));
        btn->sig_OnClick.connect([this, ya](...) { PromotePlayer(ya.name); });
        grid->AddView(btn, row++, 0);
      }
    }
    grid->UpdateLayout(0.5);
    this->AddView(grid);
    grid->Show();
    btnScout->SetFocus();
  }

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_ya_back", 30, 90, 40, 3, TR("career_back_hub"));
  btnBack->sig_OnClick.connect([this](...) { CreatePage(GetHubPageID()); });
  this->AddView(btnBack);
  btnBack->Show();

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
  Gui2Frame* bgPanel = new Gui2Frame(windowManager, "bg_career_squad", 0, 0, 100, 100, true);
  this->AddView(bgPanel);
  bgPanel->Show();
  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_squad", 10, 3, 80, 3, TR("career_squad_title"));
  this->AddView(title);
  title->Show();

  CareerSave* activeSave = CareerDatabase::GetInstance().GetActiveSave();
  if (activeSave) {
    long long totalWage = 0;
    for (const auto& p : activeSave->roster) {
      totalWage += p.wage;
    }
    Gui2Caption* header = new Gui2Caption(windowManager, "caption_squad_header", 5, 6, 90, 2,
                                          TR("career_squad_header"));
    this->AddView(header);
    header->Show();

    Gui2Caption* squadHint =
        new Gui2Caption(windowManager, "caption_squad_hint", 5, 8, 90, 2, TR("career_squad_hint"));
    this->AddView(squadHint);
    squadHint->Show();

    Gui2Grid* grid = new Gui2Grid(windowManager, "squad_grid", 5, 11, 90, 70);
    int row = 0;
    // Keep the roster grid inside its frame: squads can exceed what a 70%-tall
    // grid renders, so cap the rows and surface the count in the footer below.
    const int maxRosterRows = 24;
    for (const auto& player : activeSave->roster) {
      if (row >= maxRosterRows)
        break;
      std::string formStr = CareerDatabase::GetInstance().GetFormString(player.matchForm);
      std::string moraleStr = CareerDatabase::GetInstance().GetMoraleString(player.morale);

      const std::string label = TRF(
          "career_squad_row", {player.name, player.preferredPosition, std::to_string(player.ovr),
                               std::to_string(player.pot), std::to_string(player.age),
                               FormatCareerMoney(player.value), FormatCareerMoney(player.wage),
                               moraleStr, formStr, std::to_string(player.contract.yearsRemaining)});
      Gui2Button* btn =
          new Gui2Button(windowManager, "btn_release_" + std::to_string(row), 0, 0, 84, 2.5, label);
      btn->sig_OnClick.connect([this, player](...) { ReleasePlayer(player.name); });
      grid->AddView(btn, row++, 0);
    }
    grid->UpdateLayout(0.5);
    this->AddView(grid);
    grid->Show();

    Gui2Caption* footer = new Gui2Caption(
        windowManager, "caption_squad_footer", 5, 83, 90, 2,
        TRF("career_squad_footer", {std::to_string(activeSave->roster.size()),
                                    FormatCareerMoney(totalWage), std::to_string(maxRosterRows)}) +
            (activeSave->roster.size() > static_cast<size_t>(maxRosterRows)
                 ? " " + TR("career_squad_showmore")
                 : ""));
    this->AddView(footer);
    footer->Show();
  }

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_squad_back", 30, 90, 40, 3, TR("career_back_hub"));
  btnBack->sig_OnClick.connect([this](...) { CreatePage(GetHubPageID()); });
  this->AddView(btnBack);
  btnBack->Show();

  this->Show();
}

CareerSquadRosterPage::~CareerSquadRosterPage() {}

void CareerSquadRosterPage::ReleasePlayer(const std::string& playerName) {
  CareerDatabase::GetInstance().ReleasePlayer(playerName);
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
      new Gui2Button(windowManager, "btn_md_playtop", 0, 0, 42, 2.5, TR("career_play_match"));
  btnPlayTop->sig_OnClick.connect([this](...) { PlayMatch(); });
  fixtureGrid->AddView(btnPlayTop, row, 0);

  Gui2Button* btnSimAllTop =
      new Gui2Button(windowManager, "btn_md_simalltop", 0, 0, 42, 2.5, TR("career_simulate_all"));
  btnSimAllTop->sig_OnClick.connect([this](...) { SimulateAll(); });
  fixtureGrid->AddView(btnSimAllTop, row++, 1);

  int numFixtures = static_cast<int>(m_opponents.size());
  for (int i = 0; i < numFixtures; i++) {
    const auto& res = m_results[i];
    const bool isHome = (i < static_cast<int>(m_isHome.size())) ? m_isHome[i] : true;
    const std::string venue = isHome ? TR("career_venue_home") : TR("career_venue_away");

    Gui2Caption* header =
        new Gui2Caption(windowManager, "cap_md_hdr_" + std::to_string(i), 0, 0, 42, 2,
                        TRF("career_matchday_header", {venue, save->name, m_opponents[i]}));
    fixtureGrid->AddView(header, row++, 0);

    std::string scoreLabel = TR("career_not_played");
    if (res.played) {
      if (isHome) {
        scoreLabel =
            TRF("career_matchday_score_home", {save->name, std::to_string(res.homeGoals),
                                               std::to_string(res.awayGoals), m_opponents[i]});
      } else {
        scoreLabel =
            TRF("career_matchday_score_away", {m_opponents[i], std::to_string(res.awayGoals),
                                               std::to_string(res.homeGoals), save->name});
      }
    }
    Gui2Caption* scoreCap = new Gui2Caption(windowManager, "cap_md_score_" + std::to_string(i), 0,
                                            0, 42, 2, scoreLabel);
    fixtureGrid->AddView(scoreCap, row++, 0);
    fixtureScoreCaps[i] = scoreCap;

    if (res.played) {
      std::string scorersStr;
      if (!res.scorers.empty()) {
        scorersStr = TR("career_scorers") + " " + res.scorers[0];
        for (int s = 1; s < static_cast<int>(res.scorers.size()); s++) {
          scorersStr += ", " + res.scorers[s];
        }
      } else {
        scorersStr = TR("career_no_scorers");
      }
      const std::string stats = TRF("career_matchday_stats",
                                    {std::to_string(res.homeShots), std::to_string(res.awayShots),
                                     std::to_string(res.homePossession), scorersStr});
      Gui2Caption* statsCap =
          new Gui2Caption(windowManager, "cap_md_stats_" + std::to_string(i), 0, 0, 42, 2, stats);
      fixtureGrid->AddView(statsCap, row++, 0);
    }

    if (!res.played) {
      Gui2Button* btnSim = new Gui2Button(windowManager, "btn_md_sim_" + std::to_string(i), 0, 0,
                                          42, 2.5, TR("career_simulate"));
      btnSim->sig_OnClick.connect([this, i](...) { SimulateMatch(i); });
      fixtureGrid->AddView(btnSim, row, 0);

      Gui2Button* btnPlay = new Gui2Button(windowManager, "btn_md_play_" + std::to_string(i), 0, 0,
                                           42, 2.5, TR("career_play_match"));
      btnPlay->sig_OnClick.connect([this, i](...) { PlayMatchFixture(i); });
      fixtureGrid->AddView(btnPlay, row++, 1);
    }
  }

  // Last row: Back to Hub, reachable by the same navigation as everything else.
  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_matchday_back", 0, 0, 42, 2.5, TR("career_back_hub"));
  btnBack->sig_OnClick.connect([this](...) { GoBack(); });
  fixtureGrid->AddView(btnBack, row++, 0);

  fixtureGrid->UpdateLayout(0.5);
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
