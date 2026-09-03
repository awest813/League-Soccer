#include "league_standings.hpp"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "../../league/leaguecode.hpp"
#include "../../main.hpp"
#include "../pagefactory.hpp"
#include "base/utils.hpp"
#include "menu_smoke.hpp"
#include "utils/localization.hpp"

LeagueStandingsPage::LeagueStandingsPage(Gui2WindowManager* windowManager,
                                         const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData),
      pageCreatedTime_ms(league_menu_smoke::Now_ms()),
      autoAdvanceTriggered(false) {
  Gui2Frame* frame = new Gui2Frame(windowManager, "frame_league_standings", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_league_standings", 2, 2, 66, 3,
                      Localization::GetInstance().Translate("league_standings"));
  frame->AddView(title);
  title->Show();

  Gui2Button* btnLeague =
      new Gui2Button(windowManager, "btn_standings_league", 0, 0, 60, 3,
                     Localization::GetInstance().Translate("league_table"));
  Gui2Button* btnLeagueStats =
      new Gui2Button(windowManager, "btn_standings_league_stats", 0, 0, 60, 3,
                     Localization::GetInstance().Translate("league_stats"));
  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_standings_back", 0, 0, 60, 3,
                     Localization::GetInstance().Translate("league_back_dashboard"));

  btnLeague->sig_OnClick.connect([this](...) { GoPage(e_PageID_League_Standings_League_Table); });
  btnLeagueStats->sig_OnClick.connect(
      [this](...) { GoPage(e_PageID_League_Standings_League_Stats); });
  btnBack->sig_OnClick.connect([this](...) { GoPage(e_PageID_League_Forward); });

  Gui2Grid* grid = new Gui2Grid(windowManager, "grid_standings", 2, 10, 66, 60);
  grid->AddView(btnLeague, 0, 0);
  grid->AddView(btnLeagueStats, 1, 0);
  grid->AddView(btnBack, 2, 0);
  grid->UpdateLayout(0.5);
  frame->AddView(grid);
  grid->Show();

  btnLeague->SetFocus();
  this->Show();
}

LeagueStandingsPage::~LeagueStandingsPage() {}

void LeagueStandingsPage::Process() {
  Gui2Page::Process();

  if (!league_menu_smoke::RouteEnabled("standings_table") || autoAdvanceTriggered ||
      league_menu_smoke::Now_ms() < pageCreatedTime_ms + league_menu_smoke::kAdvanceDelay_ms) {
    return;
  }

  autoAdvanceTriggered = true;
  printf("[menu-smoke] Standings page opening league standings\n");
  GoPage(e_PageID_League_Standings_League_Table);
}

void LeagueStandingsPage::GoPage(e_PageID pageID) {
  this->Exit();
  Properties properties;
  windowManager->GetPageFactory()->CreatePage(static_cast<int>(pageID), properties, 0);
  delete this;
}

LeagueStandingsLeagueTablePage::LeagueStandingsLeagueTablePage(Gui2WindowManager* windowManager,
                                                               const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData),
      pageCreatedTime_ms(league_menu_smoke::Now_ms()),
      autoAdvanceTriggered(false) {
  Gui2Frame* frame = new Gui2Frame(windowManager, "frame_league_table", 5, 5, 90, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title = new Gui2Caption(
      windowManager, "caption_league_standings_league_table", 2, 2, 86, 3,
      Localization::GetInstance().Translate("league_table_title"));
  frame->AddView(title);
  title->Show();

  Gui2Caption* header =
      new Gui2Caption(windowManager, "caption_table_header", 2, 6, 86, 2,
                      Localization::GetInstance().Translate("league_table_header"));
  frame->AddView(header);
  header->Show();

  auto result = GetDB()->Query(
      "SELECT t.id, t.name, l.name FROM teams t JOIN leagues l ON t.league_id = l.id "
      "ORDER BY l.name, t.name");

  auto standingsResult = GetDB()->Query(
      "SELECT team_id, "
      "  COUNT(*) AS played, "
      "  SUM(CASE WHEN goals_for > goals_against THEN 1 ELSE 0 END) AS won, "
      "  SUM(CASE WHEN goals_for = goals_against THEN 1 ELSE 0 END) AS drawn, "
      "  SUM(CASE WHEN goals_for < goals_against THEN 1 ELSE 0 END) AS lost, "
      "  SUM(goals_for) AS gf, "
      "  SUM(goals_against) AS ga, "
      "  SUM(CASE WHEN goals_for > goals_against THEN 3 "
      "       WHEN goals_for = goals_against THEN 1 ELSE 0 END) AS pts "
      "FROM (SELECT team1_id AS team_id, team1_goals AS goals_for, team2_goals AS goals_against "
      "      FROM match_results WHERE played = 1 "
      "      UNION ALL "
      "      SELECT team2_id AS team_id, team2_goals AS goals_for, team1_goals AS goals_against "
      "      FROM match_results WHERE played = 1) "
      "GROUP BY team_id");

  std::map<std::string, std::vector<std::string>> statsMap;
  if (!standingsResult->data.empty()) {
    for (const auto& row : standingsResult->data) {
      std::string teamID = row.at(0);
      statsMap[teamID] = row;
    }
  }

  struct TeamRow {
    std::string id, name, league;
    std::string p, w, d, l, gf, ga, gd, pts, form;
    int ptsVal, gdVal;
  };

  // Recent form per team: last five results, oldest first (e.g. "WDWLM").
  std::map<int, std::string> formMap;
  {
    auto formResult = GetDB()->Query(
        "SELECT mr.team1_id, mr.team2_id, mr.team1_goals, mr.team2_goals "
        "FROM match_results mr "
        "JOIN calendar c ON mr.calendar_id = c.id "
        "WHERE mr.played = 1 ORDER BY date(c.timestamp), mr.id");
    auto appendResult = [&formMap](int teamID, int goalsFor, int goalsAgainst) {
      std::string& form = formMap[teamID];
      form += (goalsFor > goalsAgainst) ? 'W' : (goalsFor == goalsAgainst) ? 'D' : 'L';
      if (form.size() > 5) {
        form.erase(0, form.size() - 5);
      }
    };
    for (const auto& row : formResult->data) {
      int t1 = atoi(row.at(0).c_str());
      int t2 = atoi(row.at(1).c_str());
      int g1 = atoi(row.at(2).c_str());
      int g2 = atoi(row.at(3).c_str());
      appendResult(t1, g1, g2);
      appendResult(t2, g2, g1);
    }
  }

  std::vector<TeamRow> allTeams;
  for (const auto& r : result->data) {
    TeamRow tr;
    tr.id = r.at(0);
    tr.name = r.at(1);
    tr.league = r.at(2);
    tr.p = "0";
    tr.w = "0";
    tr.d = "0";
    tr.l = "0";
    tr.gf = "0";
    tr.ga = "0";
    tr.gd = "0";
    tr.pts = "0";
    tr.ptsVal = 0;
    tr.gdVal = 0;
    auto it = statsMap.find(r.at(0));
    if (it != statsMap.end()) {
      const auto& s = it->second;
      tr.p = s.at(1);
      tr.w = s.at(2);
      tr.d = s.at(3);
      tr.l = s.at(4);
      tr.gf = s.at(5);
      tr.ga = s.at(6);
      tr.gdVal = atoi(tr.gf.c_str()) - atoi(tr.ga.c_str());
      tr.gd = std::to_string(tr.gdVal);
      tr.pts = s.at(8);
      tr.ptsVal = atoi(tr.pts.c_str());
    }
    auto formIt = formMap.find(atoi(tr.id.c_str()));
    if (formIt != formMap.end()) {
      tr.form = formIt->second;
    }
    allTeams.push_back(tr);
  }

  std::stable_sort(allTeams.begin(), allTeams.end(), [](const TeamRow& a, const TeamRow& b) {
    if (a.league != b.league)
      return a.league < b.league;
    if (a.ptsVal != b.ptsVal)
      return a.ptsVal > b.ptsVal;
    return a.gdVal > b.gdVal;
  });

  Gui2Grid* grid = new Gui2Grid(windowManager, "grid_league_table", 2, 9, 86, 78);
  int row = 0;
  std::string currentLeague;
  int leaguePos = 0;
  int leagueSize = 0;
  int userTeamID = 0;
  LeagueGetUserTeamID(userTeamID);

  const Vector3 colorUser(250, 210, 60);
  const Vector3 colorLeader(110, 220, 110);
  const Vector3 colorBottom(235, 105, 105);

  for (const auto& tr : allTeams) {
    if (tr.league != currentLeague) {
      currentLeague = tr.league;
      leaguePos = 0;
      leagueSize = static_cast<int>(std::count_if(allTeams.begin(), allTeams.end(),
                                                  [&tr](const TeamRow& t) {
                                                    return t.league == tr.league;
                                                  }));
      Gui2Caption* sep = new Gui2Caption(windowManager, "caption_league_sep_" + int_to_str(row), 0,
                                         0, 85, 2.5, "--- " + currentLeague + " ---");
      grid->AddView(sep, row++, 0);
    }
    leaguePos++;

    const bool isUserTeam = (atoi(tr.id.c_str()) == userTeamID);
    char buf[256];
    snprintf(buf, sizeof(buf), "%-2s%-27s | %2s | %2s | %2s | %2s | %2s | %2s | %2s | %3s | %-5s",
             isUserTeam ? "> " : "", tr.name.c_str(), tr.p.c_str(), tr.w.c_str(), tr.d.c_str(),
             tr.l.c_str(), tr.gf.c_str(), tr.ga.c_str(), tr.gd.c_str(), tr.pts.c_str(),
             tr.form.c_str());
    Gui2Button* btn = new Gui2Button(windowManager, "btn_table_" + tr.id, 0, 0, 85, 2.5, buf);
    if (isUserTeam) {
      btn->SetColor(colorUser);
    } else if (leaguePos == 1 && leagueSize > 1) {
      btn->SetColor(colorLeader);
    } else if (leaguePos == leagueSize && leagueSize > 2) {
      btn->SetColor(colorBottom);
    }
    grid->AddView(btn, row++, 0);
  }

  // Back lives in the same grid so keyboard/gamepad can reach it too.
  Gui2Button* btnBack = new Gui2Button(windowManager, "btn_table_back", 0, 0, 85, 2.5,
                                       Localization::GetInstance().Translate("action_back"));
  btnBack->sig_OnClick.connect([this, windowManager](...) {
    this->Exit();
    Properties properties;
    windowManager->GetPageFactory()->CreatePage(static_cast<int>(e_PageID_League_Standings),
                                                properties, 0);
    delete this;
  });
  grid->AddView(btnBack, row, 0);

  grid->UpdateLayout(0.5);
  frame->AddView(grid);
  grid->Show();

  if (grid->IsSelectable()) {
    grid->SetFocus();
  }

  this->Show();
}

LeagueStandingsLeagueTablePage::~LeagueStandingsLeagueTablePage() {}

void LeagueStandingsLeagueTablePage::Process() {
  Gui2Page::Process();

  if (!league_menu_smoke::RouteEnabled("standings_table") || autoAdvanceTriggered ||
      league_menu_smoke::Now_ms() < pageCreatedTime_ms + league_menu_smoke::kQuitDelay_ms) {
    return;
  }

  autoAdvanceTriggered = true;
  printf("[menu-smoke] League standings table reached successfully\n");
  GetMenuTask()->QuitGame();
}

LeagueStandingsLeagueStatsPage::LeagueStandingsLeagueStatsPage(Gui2WindowManager* windowManager,
                                                               const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* frame = new Gui2Frame(windowManager, "frame_league_stats", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title = new Gui2Caption(
      windowManager, "caption_league_standings_league_stats", 2, 2, 66, 3,
      Localization::GetInstance().Translate("league_stats"));
  frame->AddView(title);
  title->Show();

  auto totalResult = GetDB()->Query("SELECT COUNT(*) FROM match_results WHERE played = 1");
  auto highScoring = GetDB()->Query(
      "SELECT t1.name, t2.name, mr.team1_goals, mr.team2_goals, l.name "
      "FROM match_results mr "
      "JOIN teams t1 ON mr.team1_id = t1.id "
      "JOIN teams t2 ON mr.team2_id = t2.id "
      "JOIN leagues l ON mr.competition_id = l.id "
      "WHERE mr.played = 1 "
      "ORDER BY (mr.team1_goals + mr.team2_goals) DESC LIMIT 5");

  Gui2Grid* grid = new Gui2Grid(windowManager, "grid_league_stats", 2, 8, 66, 75);
  int row = 0;

  std::string totalMatches = totalResult->data.empty() ? "0" : totalResult->data.at(0).at(0);
  Gui2Caption* totalCap = new Gui2Caption(
      windowManager, "caption_stats_total", 0, 0, 65, 2.5,
      Localization::GetInstance().TranslateAndFormat("league_stats_total", {totalMatches}));
  grid->AddView(totalCap, row++, 0);

  if (!highScoring->data.empty()) {
    Gui2Caption* hdrCap = new Gui2Caption(
        windowManager, "caption_stats_high", 0, 0, 65, 2.5,
        Localization::GetInstance().Translate("league_stats_high_header"));
    grid->AddView(hdrCap, row++, 0);

    for (const auto& r : highScoring->data) {
      char buf[256];
      snprintf(buf, sizeof(buf), "%s %d - %d %s (%s)", r.at(0).c_str(), atoi(r.at(2).c_str()),
               atoi(r.at(3).c_str()), r.at(1).c_str(), r.at(4).c_str());
      Gui2Caption* matchCap = new Gui2Caption(
          windowManager, "caption_stats_match_" + std::to_string(row), 0, 0, 65, 2.5, buf);
      grid->AddView(matchCap, row++, 0);
    }
  } else {
    Gui2Caption* noData =
        new Gui2Caption(windowManager, "caption_stats_nodata", 0, 0, 65, 2.5,
                        Localization::GetInstance().Translate("league_stats_none"));
    grid->AddView(noData, row++, 0);
  }

  grid->UpdateLayout(0.5);
  frame->AddView(grid);
  grid->Show();

  Gui2Button* btnBack = new Gui2Button(windowManager, "btn_league_stats_back", 15, 86, 40, 3,
                                       Localization::GetInstance().Translate("action_back"));
  btnBack->sig_OnClick.connect([this, windowManager](...) {
    this->Exit();
    Properties properties;
    windowManager->GetPageFactory()->CreatePage(static_cast<int>(e_PageID_League_Standings),
                                                properties, 0);
    delete this;
  });
  frame->AddView(btnBack);
  btnBack->Show();
  btnBack->SetFocus();
  this->Show();
}

LeagueStandingsLeagueStatsPage::~LeagueStandingsLeagueStatsPage() {}
