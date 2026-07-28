#include "league_forward.hpp"

#include <string>

#include "../../league/leaguecode.hpp"
#include "../../main.hpp"
#include "../pagefactory.hpp"
#include "base/utils.hpp"
#include "menu_smoke.hpp"

namespace {

std::string SafeValue(const DatabaseResult* result, size_t row, size_t col,
                      const std::string& fallback = "-") {
  if (result == nullptr || row >= result->data.size() || col >= result->data.at(row).size() ||
      result->data.at(row).at(col).empty()) {
    return fallback;
  }
  return result->data.at(row).at(col);
}

}  // namespace

LeagueForwardPage::LeagueForwardPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData),
      pageCreatedTime_ms(league_menu_smoke::Now_ms()),
      autoAdvanceTriggered(false) {
  auto result = GetDB()->Query("SELECT managername, timestamp FROM settings LIMIT 1");
  std::string mgrName = "Manager";
  std::string dateStr = "";
  if (!result->data.empty()) {
    if (result->data.at(0).size() > 0)
      mgrName = result->data.at(0).at(0);
    if (result->data.at(0).size() > 1)
      dateStr = result->data.at(0).at(1);
  }

  auto teamResult = GetDB()->Query(
      "SELECT t.id, t.name, l.name FROM teams t "
      "JOIN settings s ON t.id = s.team_id "
      "LEFT JOIN leagues l ON t.league_id = l.id "
      "LIMIT 1");
  const std::string teamID = SafeValue(teamResult.get(), 0, 0, "0");
  const std::string teamName = SafeValue(teamResult.get(), 0, 1, "Club");
  const std::string leagueName = SafeValue(teamResult.get(), 0, 2, "League");

  std::string squadSize = "0";
  auto squadResult =
      GetDB()->Query("SELECT COUNT(*) FROM players WHERE team_id = " + teamID + " LIMIT 1");
  if (!squadResult->data.empty()) {
    squadSize = SafeValue(squadResult.get(), 0, 0, "0");
  }

  std::string unreadMessages = "0";
  auto unreadResult = GetDB()->Query("SELECT COUNT(*) FROM inbox_messages WHERE read = 0 LIMIT 1");
  if (!unreadResult->data.empty()) {
    unreadMessages = SafeValue(unreadResult.get(), 0, 0, "0");
  }

  std::string nextFixture = "No upcoming fixture scheduled";
  auto fixtureResult = GetDB()->Query(
      "SELECT c.timestamp, t1.name, t2.name "
      "FROM calendar c "
      "JOIN teams t1 ON c.team1_id = t1.id "
      "JOIN teams t2 ON c.team2_id = t2.id "
      "JOIN settings s ON (c.team1_id = s.team_id OR c.team2_id = s.team_id) "
      "ORDER BY c.timestamp LIMIT 1");
  if (!fixtureResult->data.empty()) {
    nextFixture = SafeValue(fixtureResult.get(), 0, 0, "-").substr(0, 10) + "  " +
                  SafeValue(fixtureResult.get(), 0, 1, "Home") + " vs " +
                  SafeValue(fixtureResult.get(), 0, 2, "Away");
  }

  std::string standingsLine = "No standings data yet";
  auto standingsResult = GetDB()->Query(
      "SELECT "
      "  SUM(CASE WHEN goals_for > goals_against THEN 3 "
      "       WHEN goals_for = goals_against THEN 1 ELSE 0 END) AS pts, "
      "  COUNT(*) AS played "
      "FROM (SELECT team1_id AS team_id, team1_goals AS goals_for, team2_goals AS goals_against "
      "      FROM match_results WHERE played = 1 "
      "      UNION ALL "
      "      SELECT team2_id AS team_id, team2_goals AS goals_for, team1_goals AS goals_against "
      "      FROM match_results WHERE played = 1) "
      "WHERE team_id = " +
      teamID);
  if (!standingsResult->data.empty()) {
    standingsLine = SafeValue(standingsResult.get(), 0, 1, "0") + " matches played, " +
                    SafeValue(standingsResult.get(), 0, 0, "0") + " points earned";
  }

  Gui2Frame* frame = new Gui2Frame(windowManager, "frame_league_forward", 6, 5, 88, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_league_forward", 3, 2, 40, 3, "League Dashboard");
  frame->AddView(title);
  title->Show();

  Gui2Caption* info = new Gui2Caption(windowManager, "caption_forward_info", 3, 6, 40, 2,
                                      "Manager: " + mgrName + " | Date: " + dateStr);
  frame->AddView(info);
  info->Show();

  Gui2Frame* navPanel = new Gui2Frame(windowManager, "frame_forward_nav", 3, 12, 42, 72, true);
  frame->AddView(navPanel);
  navPanel->Show();

  Gui2Caption* navTitle =
      new Gui2Caption(windowManager, "caption_forward_nav_title", 2, 2, 36, 2, "Club Actions");
  navPanel->AddView(navTitle);
  navTitle->Show();

  Gui2Button* btnTeam =
      new Gui2Button(windowManager, "btn_forward_team", 0, 0, 36, 4, "Team Management");
  Gui2Button* btnCalendar =
      new Gui2Button(windowManager, "btn_forward_calendar", 0, 0, 36, 4, "Calendar / Fixtures");
  Gui2Button* btnStandings =
      new Gui2Button(windowManager, "btn_forward_standings", 0, 0, 36, 4, "Standings");
  Gui2Button* btnManagement =
      new Gui2Button(windowManager, "btn_forward_management", 0, 0, 36, 4, "Management");
  Gui2Button* btnInbox = new Gui2Button(windowManager, "btn_forward_inbox", 0, 0, 36, 4, "Inbox");
  Gui2Button* btnSystem =
      new Gui2Button(windowManager, "btn_forward_system", 0, 0, 36, 4, "System");
  Gui2Button* btnLeagueHub =
      new Gui2Button(windowManager, "btn_forward_league", 0, 0, 36, 4, "Back to League Hub");
  Gui2Button* btnMainMenu =
      new Gui2Button(windowManager, "btn_forward_mainmenu", 0, 0, 36, 4, "Return to Main Menu");

  btnTeam->sig_OnClick.connect([this](...) { GoPage(e_PageID_League_Team); });
  btnCalendar->sig_OnClick.connect([this](...) { GoPage(e_PageID_League_Calendar); });
  btnStandings->sig_OnClick.connect([this](...) { GoPage(e_PageID_League_Standings); });
  btnManagement->sig_OnClick.connect([this](...) { GoPage(e_PageID_League_Management); });
  btnInbox->sig_OnClick.connect([this](...) { GoPage(e_PageID_League_Inbox); });
  btnSystem->sig_OnClick.connect([this](...) { GoPage(e_PageID_League_System); });
  btnLeagueHub->sig_OnClick.connect([this](...) { GoPage(e_PageID_League); });
  btnMainMenu->sig_OnClick.connect([this](...) { GoMainMenu(); });

  Gui2Grid* grid = new Gui2Grid(windowManager, "grid_forward", 2, 7, 36, 60);
  grid->AddView(btnTeam, 0, 0);
  grid->AddView(btnCalendar, 1, 0);
  grid->AddView(btnStandings, 2, 0);
  grid->AddView(btnManagement, 3, 0);
  grid->AddView(btnInbox, 4, 0);
  grid->AddView(btnSystem, 5, 0);
  grid->AddView(btnLeagueHub, 6, 0);
  grid->AddView(btnMainMenu, 7, 0);
  grid->UpdateLayout(0.25, 0.25, 0.3, 0.3);
  navPanel->AddView(grid);
  grid->Show();

  Gui2Frame* clubPanel = new Gui2Frame(windowManager, "frame_forward_club", 48, 12, 37, 24, true);
  frame->AddView(clubPanel);
  clubPanel->Show();

  Gui2Caption* clubTitle =
      new Gui2Caption(windowManager, "caption_forward_club_title", 2, 2, 32, 2, "Club Snapshot");
  clubPanel->AddView(clubTitle);
  clubTitle->Show();

  Gui2Caption* clubBody =
      new Gui2Caption(windowManager, "caption_forward_club_body", 2, 6, 32, 10,
                      teamName + "\n" + leagueName + "\n" + squadSize + " registered players");
  clubPanel->AddView(clubBody);
  clubBody->Show();

  Gui2Frame* seasonPanel =
      new Gui2Frame(windowManager, "frame_forward_season", 48, 39, 37, 21, true);
  frame->AddView(seasonPanel);
  seasonPanel->Show();

  Gui2Caption* seasonTitle =
      new Gui2Caption(windowManager, "caption_forward_season_title", 2, 2, 32, 2, "Season Pulse");
  seasonPanel->AddView(seasonTitle);
  seasonTitle->Show();

  Gui2Caption* seasonBody = new Gui2Caption(windowManager, "caption_forward_season_body", 2, 6, 32,
                                            8, standingsLine + "\nUnread inbox: " + unreadMessages);
  seasonPanel->AddView(seasonBody);
  seasonBody->Show();

  Gui2Frame* fixturePanel =
      new Gui2Frame(windowManager, "frame_forward_fixture", 48, 63, 37, 21, true);
  frame->AddView(fixturePanel);
  fixturePanel->Show();

  Gui2Caption* fixtureTitle =
      new Gui2Caption(windowManager, "caption_forward_fixture_title", 2, 2, 32, 2, "Next Fixture");
  fixturePanel->AddView(fixtureTitle);
  fixtureTitle->Show();

  Gui2Caption* fixtureBody =
      new Gui2Caption(windowManager, "caption_forward_fixture_body", 2, 6, 32, 8, nextFixture);
  fixturePanel->AddView(fixtureBody);
  fixtureBody->Show();

  btnTeam->SetFocus();
  this->Show();
}

LeagueForwardPage::~LeagueForwardPage() {}

void LeagueForwardPage::Process() {
  Gui2Page::Process();

  if (!league_menu_smoke::HasRoute() || autoAdvanceTriggered ||
      league_menu_smoke::Now_ms() < pageCreatedTime_ms + league_menu_smoke::kAdvanceDelay_ms) {
    return;
  }

  autoAdvanceTriggered = true;

  if (league_menu_smoke::RouteEnabled("dashboard")) {
    printf("[menu-smoke] League dashboard reached successfully\n");
    GetMenuTask()->QuitGame();
  } else if (league_menu_smoke::RouteEnabled("inbox")) {
    printf("[menu-smoke] League dashboard opening Inbox\n");
    GoPage(e_PageID_League_Inbox);
  } else if (league_menu_smoke::RouteEnabled("calendar")) {
    printf("[menu-smoke] League dashboard opening Calendar\n");
    GoPage(e_PageID_League_Calendar);
  } else if (league_menu_smoke::RouteEnabled("standings_table")) {
    printf("[menu-smoke] League dashboard opening Standings\n");
    GoPage(e_PageID_League_Standings);
  } else if (league_menu_smoke::RouteEnabled("team_overview")) {
    printf("[menu-smoke] League dashboard opening Team Management\n");
    GoPage(e_PageID_League_Team);
  } else if (league_menu_smoke::RouteEnabled("management")) {
    printf("[menu-smoke] League dashboard opening Management\n");
    GoPage(e_PageID_League_Management);
  } else if (league_menu_smoke::RouteEnabled("management_contracts")) {
    printf("[menu-smoke] League dashboard opening Management\n");
    GoPage(e_PageID_League_Management);
  } else if (league_menu_smoke::RouteEnabled("system_settings")) {
    printf("[menu-smoke] League dashboard opening System\n");
    GoPage(e_PageID_League_System);
  } else {
    printf("[menu-smoke] League dashboard route '%s' is unsupported\n",
           league_menu_smoke::GetRoute().c_str());
    GetMenuTask()->QuitGame();
  }
}

void LeagueForwardPage::GoPage(e_PageID pageID) {
  this->Exit();
  Properties properties;
  windowManager->GetPageFactory()->CreatePage(static_cast<int>(pageID), properties, 0);
  delete this;
}

void LeagueForwardPage::GoMainMenu() {
  SaveAutosaveToDatabase();

  this->Exit();
  Properties properties;
  windowManager->GetPageFactory()->CreatePage(static_cast<int>(e_PageID_MainMenu), properties, 0);
  delete this;
}
