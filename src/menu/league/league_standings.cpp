#include "league_standings.hpp"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "../../main.hpp"
#include "../pagefactory.hpp"
#include "base/utils.hpp"
#include "menu_smoke.hpp"

LeagueStandingsPage::LeagueStandingsPage(Gui2WindowManager* windowManager,
                                         const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData),
      pageCreatedTime_ms(league_menu_smoke::Now_ms()),
      autoAdvanceTriggered(false) {
  Gui2Frame* frame = new Gui2Frame(windowManager, "frame_league_standings", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_league_standings", 2, 2, 66, 3, "Standings");
  frame->AddView(title);
  title->Show();

  Gui2Button* btnLeague =
      new Gui2Button(windowManager, "btn_standings_league", 0, 0, 60, 3, "League Table");
  Gui2Button* btnLeagueStats =
      new Gui2Button(windowManager, "btn_standings_league_stats", 0, 0, 60, 3, "League Stats");
  Gui2Button* btnNCup =
      new Gui2Button(windowManager, "btn_standings_ncup", 0, 0, 60, 3, "National Cup");
  Gui2Button* btnICup1 =
      new Gui2Button(windowManager, "btn_standings_icup1", 0, 0, 60, 3, "International Cup 1");
  Gui2Button* btnICup2 =
      new Gui2Button(windowManager, "btn_standings_icup2", 0, 0, 60, 3, "International Cup 2");
  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_standings_back", 0, 0, 60, 3, "Back to Dashboard");

  btnLeague->sig_OnClick.connect([this](...) { GoPage(e_PageID_League_Standings_League); });
  btnLeagueStats->sig_OnClick.connect(
      [this](...) { GoPage(e_PageID_League_Standings_League_Stats); });
  btnNCup->sig_OnClick.connect([this](...) { GoPage(e_PageID_League_Standings_NCup); });
  btnICup1->sig_OnClick.connect([this](...) { GoPage(e_PageID_League_Standings_ICup1); });
  btnICup2->sig_OnClick.connect([this](...) { GoPage(e_PageID_League_Standings_ICup2); });
  btnBack->sig_OnClick.connect([this](...) { GoPage(e_PageID_League_Forward); });

  Gui2Grid* grid = new Gui2Grid(windowManager, "grid_standings", 2, 10, 66, 60);
  grid->AddView(btnLeague, 0, 0);
  grid->AddView(btnLeagueStats, 1, 0);
  grid->AddView(btnNCup, 2, 0);
  grid->AddView(btnICup1, 3, 0);
  grid->AddView(btnICup2, 4, 0);
  grid->AddView(btnBack, 5, 0);
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
  GoPage(e_PageID_League_Standings_League);
}

void LeagueStandingsPage::GoPage(e_PageID pageID) {
  this->Exit();
  Properties properties;
  windowManager->GetPageFactory()->CreatePage(static_cast<int>(pageID), properties, 0);
  delete this;
}

LeagueStandingsLeaguePage::LeagueStandingsLeaguePage(Gui2WindowManager* windowManager,
                                                     const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData),
      pageCreatedTime_ms(league_menu_smoke::Now_ms()),
      autoAdvanceTriggered(false) {
  Gui2Frame* frame =
      new Gui2Frame(windowManager, "frame_league_standings_league", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_league_standings_league", 2, 2, 66, 3, "League");
  frame->AddView(title);
  title->Show();

  Gui2Button* btnTable = new Gui2Button(windowManager, "btn_league_table", 0, 0, 60, 3, "Table");
  Gui2Button* btnStats = new Gui2Button(windowManager, "btn_league_stats", 0, 0, 60, 3, "Stats");
  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_league_back", 0, 0, 60, 3, "Back to Standings");

  btnTable->sig_OnClick.connect([this](...) { GoPage(e_PageID_League_Standings_League_Table); });
  btnStats->sig_OnClick.connect([this](...) { GoPage(e_PageID_League_Standings_League_Stats); });
  btnBack->sig_OnClick.connect([this](...) { GoPage(e_PageID_League_Standings); });

  Gui2Grid* grid = new Gui2Grid(windowManager, "grid_league_sub", 2, 10, 66, 50);
  grid->AddView(btnTable, 0, 0);
  grid->AddView(btnStats, 1, 0);
  grid->AddView(btnBack, 2, 0);
  grid->UpdateLayout(0.5);
  frame->AddView(grid);
  grid->Show();

  btnTable->SetFocus();
  this->Show();
}

LeagueStandingsLeaguePage::~LeagueStandingsLeaguePage() {}

void LeagueStandingsLeaguePage::Process() {
  Gui2Page::Process();

  if (!league_menu_smoke::RouteEnabled("standings_table") || autoAdvanceTriggered ||
      league_menu_smoke::Now_ms() < pageCreatedTime_ms + league_menu_smoke::kAdvanceDelay_ms) {
    return;
  }

  autoAdvanceTriggered = true;
  printf("[menu-smoke] League standings opening table\n");
  GoPage(e_PageID_League_Standings_League_Table);
}

void LeagueStandingsLeaguePage::GoPage(e_PageID pageID) {
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

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_league_standings_league_table", 2, 2,
                                       86, 3, "League Table");
  frame->AddView(title);
  title->Show();

  Gui2Caption* header =
      new Gui2Caption(windowManager, "caption_table_header", 2, 6, 86, 2,
                      "Team                          | P  | W  | D  | L  | GF | GA | GD | Pts");
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
    std::string p, w, d, l, gf, ga, gd, pts;
    int ptsVal, gdVal;
  };

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
  for (const auto& tr : allTeams) {
    if (tr.league != currentLeague) {
      currentLeague = tr.league;
      Gui2Caption* sep = new Gui2Caption(windowManager, "caption_league_sep_" + int_to_str(row), 0,
                                         0, 90, 2.5, "--- " + currentLeague + " ---");
      grid->AddView(sep, row++, 0);
    }
    char buf[256];
    snprintf(buf, sizeof(buf), "%-29s | %2s | %2s | %2s | %2s | %2s | %2s | %2s | %3s",
             tr.name.c_str(), tr.p.c_str(), tr.w.c_str(), tr.d.c_str(), tr.l.c_str(), tr.gf.c_str(),
             tr.ga.c_str(), tr.gd.c_str(), tr.pts.c_str());
    Gui2Button* btn = new Gui2Button(windowManager, "btn_table_" + tr.id, 0, 0, 90, 2.5, buf);
    grid->AddView(btn, row++, 0);
  }
  grid->UpdateLayout(0.5);
  frame->AddView(grid);
  grid->Show();

  Gui2Button* btnBack = new Gui2Button(windowManager, "btn_table_back", 30, 92, 40, 3, "Back");
  btnBack->sig_OnClick.connect([this, windowManager](...) {
    this->Exit();
    Properties properties;
    windowManager->GetPageFactory()->CreatePage(static_cast<int>(e_PageID_League_Standings_League),
                                                properties, 0);
    delete this;
  });
  frame->AddView(btnBack);
  btnBack->Show();
  this->SetFocus();
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

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_league_standings_league_stats", 2, 2,
                                       66, 3, "League Stats");
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
  Gui2Caption* totalCap = new Gui2Caption(windowManager, "caption_stats_total", 0, 0, 86, 2.5,
                                          "Total Matches Played: " + totalMatches);
  grid->AddView(totalCap, row++, 0);

  if (!highScoring->data.empty()) {
    Gui2Caption* hdrCap = new Gui2Caption(windowManager, "caption_stats_high", 0, 0, 86, 2.5,
                                          "--- Highest Scoring Matches ---");
    grid->AddView(hdrCap, row++, 0);

    for (const auto& r : highScoring->data) {
      char buf[256];
      snprintf(buf, sizeof(buf), "%s %d - %d %s (%s)", r.at(0).c_str(), atoi(r.at(2).c_str()),
               atoi(r.at(3).c_str()), r.at(1).c_str(), r.at(4).c_str());
      Gui2Caption* matchCap = new Gui2Caption(
          windowManager, "caption_stats_match_" + std::to_string(row), 0, 0, 86, 2.5, buf);
      grid->AddView(matchCap, row++, 0);
    }
  } else {
    Gui2Caption* noData =
        new Gui2Caption(windowManager, "caption_stats_nodata", 0, 0, 86, 2.5,
                        "No matches played yet. Simulate matches from the Calendar.");
    grid->AddView(noData, row++, 0);
  }

  grid->UpdateLayout(0.5);
  frame->AddView(grid);
  grid->Show();

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_league_stats_back", 30, 92, 40, 3, "Back");
  btnBack->sig_OnClick.connect([this, windowManager](...) {
    this->Exit();
    Properties properties;
    windowManager->GetPageFactory()->CreatePage(static_cast<int>(e_PageID_League_Standings_League),
                                                properties, 0);
    delete this;
  });
  frame->AddView(btnBack);
  btnBack->Show();
  btnBack->SetFocus();
  this->Show();
}

LeagueStandingsLeagueStatsPage::~LeagueStandingsLeagueStatsPage() {}

LeagueStandingsNCupPage::LeagueStandingsNCupPage(Gui2WindowManager* windowManager,
                                                 const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* frame = new Gui2Frame(windowManager, "frame_standings_ncup", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_league_standings_ncup", 2, 2, 66, 3, "National Cup");
  frame->AddView(title);
  title->Show();

  Gui2Caption* info = new Gui2Caption(
      windowManager, "caption_ncup_info", 2, 10, 66, 8,
      "Coming Soon - National Cup tournaments will be available in a future update.");
  frame->AddView(info);
  info->Show();

  Gui2Button* btnBack = new Gui2Button(windowManager, "btn_ncup_back", 30, 92, 40, 3, "Back");
  btnBack->sig_OnClick.connect([this](...) { GoPage(e_PageID_League_Standings); });
  frame->AddView(btnBack);
  btnBack->Show();
  btnBack->SetFocus();
  this->Show();
}

LeagueStandingsNCupPage::~LeagueStandingsNCupPage() {}

void LeagueStandingsNCupPage::GoPage(e_PageID pageID) {
  this->Exit();
  Properties properties;
  windowManager->GetPageFactory()->CreatePage(static_cast<int>(pageID), properties, 0);
  delete this;
}

LeagueStandingsNCupTreePage::LeagueStandingsNCupTreePage(Gui2WindowManager* windowManager,
                                                         const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* frame = new Gui2Frame(windowManager, "frame_standings_ncup_tree", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_league_standings_ncup_tree", 10, 5,
                                       80, 3, "National Cup - Tournament Tree");
  frame->AddView(title);
  title->Show();

  Gui2Caption* info = new Gui2Caption(
      windowManager, "caption_ncup_tree_info", 10, 15, 80, 8,
      "Coming Soon - National Cup tournaments will be available in a future update.");
  frame->AddView(info);
  info->Show();

  Gui2Button* btnBack = new Gui2Button(windowManager, "btn_ncup_tree_back", 30, 90, 40, 3, "Back");
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

LeagueStandingsNCupTreePage::~LeagueStandingsNCupTreePage() {}

LeagueStandingsNCupStatsPage::LeagueStandingsNCupStatsPage(Gui2WindowManager* windowManager,
                                                           const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* frame =
      new Gui2Frame(windowManager, "frame_standings_ncup_stats", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_league_standings_ncup_stats", 10, 5,
                                       80, 3, "National Cup - Stats");
  frame->AddView(title);
  title->Show();

  Gui2Caption* info = new Gui2Caption(
      windowManager, "caption_ncup_stats_info", 10, 15, 80, 8,
      "Coming Soon - National Cup tournaments will be available in a future update.");
  frame->AddView(info);
  info->Show();

  Gui2Button* btnBack = new Gui2Button(windowManager, "btn_ncup_stats_back", 30, 90, 40, 3, "Back");
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

LeagueStandingsNCupStatsPage::~LeagueStandingsNCupStatsPage() {}

LeagueStandingsICup1Page::LeagueStandingsICup1Page(Gui2WindowManager* windowManager,
                                                   const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* frame = new Gui2Frame(windowManager, "frame_standings_icup1", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_league_standings_icup1", 10, 5, 80,
                                       3, "International Cup 1");
  frame->AddView(title);
  title->Show();

  Gui2Caption* info = new Gui2Caption(
      windowManager, "caption_icup1_info", 10, 15, 80, 8,
      "Coming Soon - International Cup 1 tournaments will be available in a future update.");
  frame->AddView(info);
  info->Show();

  Gui2Button* btnBack = new Gui2Button(windowManager, "btn_icup1_back", 30, 90, 40, 3, "Back");
  btnBack->sig_OnClick.connect([this](...) { GoPage(e_PageID_League_Standings); });
  frame->AddView(btnBack);
  btnBack->Show();
  btnBack->SetFocus();
  this->Show();
}

LeagueStandingsICup1Page::~LeagueStandingsICup1Page() {}

void LeagueStandingsICup1Page::GoPage(e_PageID pageID) {
  this->Exit();
  Properties properties;
  windowManager->GetPageFactory()->CreatePage(static_cast<int>(pageID), properties, 0);
  delete this;
}

LeagueStandingsICup1GroupTablePage::LeagueStandingsICup1GroupTablePage(
    Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* frame =
      new Gui2Frame(windowManager, "frame_standings_icup1_grouptable", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_league_standings_icup1_grouptable",
                                       10, 5, 80, 3, "International Cup 1 - Group Table");
  frame->AddView(title);
  title->Show();

  Gui2Caption* info = new Gui2Caption(
      windowManager, "caption_icup1_gt_info", 10, 15, 80, 8,
      "Coming Soon - International Cup 1 tournaments will be available in a future update.");
  frame->AddView(info);
  info->Show();

  Gui2Button* btnBack = new Gui2Button(windowManager, "btn_icup1_gt_back", 30, 90, 40, 3, "Back");
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

LeagueStandingsICup1GroupTablePage::~LeagueStandingsICup1GroupTablePage() {}

LeagueStandingsICup1TreePage::LeagueStandingsICup1TreePage(Gui2WindowManager* windowManager,
                                                           const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* frame =
      new Gui2Frame(windowManager, "frame_standings_icup1_tree", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_league_standings_icup1_tree", 10, 5,
                                       80, 3, "International Cup 1 - Tournament Tree");
  frame->AddView(title);
  title->Show();

  Gui2Caption* info = new Gui2Caption(
      windowManager, "caption_icup1_tree_info", 10, 15, 80, 8,
      "Coming Soon - International Cup 1 tournaments will be available in a future update.");
  frame->AddView(info);
  info->Show();

  Gui2Button* btnBack = new Gui2Button(windowManager, "btn_icup1_tree_back", 30, 90, 40, 3, "Back");
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

LeagueStandingsICup1TreePage::~LeagueStandingsICup1TreePage() {}

LeagueStandingsICup1StatsPage::LeagueStandingsICup1StatsPage(Gui2WindowManager* windowManager,
                                                             const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* frame =
      new Gui2Frame(windowManager, "frame_standings_icup1_stats", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_league_standings_icup1_stats", 10, 5,
                                       80, 3, "International Cup 1 - Stats");
  frame->AddView(title);
  title->Show();

  Gui2Caption* info = new Gui2Caption(
      windowManager, "caption_icup1_stats_info", 10, 15, 80, 8,
      "Coming Soon - International Cup 1 tournaments will be available in a future update.");
  frame->AddView(info);
  info->Show();

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_icup1_stats_back", 30, 90, 40, 3, "Back");
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

LeagueStandingsICup1StatsPage::~LeagueStandingsICup1StatsPage() {}

LeagueStandingsICup2Page::LeagueStandingsICup2Page(Gui2WindowManager* windowManager,
                                                   const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* frame = new Gui2Frame(windowManager, "frame_standings_icup2", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_league_standings_icup2", 10, 5, 80,
                                       3, "International Cup 2");
  frame->AddView(title);
  title->Show();

  Gui2Caption* info = new Gui2Caption(
      windowManager, "caption_icup2_info", 10, 15, 80, 8,
      "Coming Soon - International Cup 2 tournaments will be available in a future update.");
  frame->AddView(info);
  info->Show();

  Gui2Button* btnBack = new Gui2Button(windowManager, "btn_icup2_back", 30, 90, 40, 3, "Back");
  btnBack->sig_OnClick.connect([this](...) { GoPage(e_PageID_League_Standings); });
  frame->AddView(btnBack);
  btnBack->Show();
  btnBack->SetFocus();
  this->Show();
}

LeagueStandingsICup2Page::~LeagueStandingsICup2Page() {}

void LeagueStandingsICup2Page::GoPage(e_PageID pageID) {
  this->Exit();
  Properties properties;
  windowManager->GetPageFactory()->CreatePage(static_cast<int>(pageID), properties, 0);
  delete this;
}

LeagueStandingsICup2GroupTablePage::LeagueStandingsICup2GroupTablePage(
    Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* frame =
      new Gui2Frame(windowManager, "frame_standings_icup2_grouptable", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_league_standings_icup2_grouptable",
                                       10, 5, 80, 3, "International Cup 2 - Group Table");
  frame->AddView(title);
  title->Show();

  Gui2Caption* info = new Gui2Caption(
      windowManager, "caption_icup2_gt_info", 10, 15, 80, 8,
      "Coming Soon - International Cup 2 tournaments will be available in a future update.");
  frame->AddView(info);
  info->Show();

  Gui2Button* btnBack = new Gui2Button(windowManager, "btn_icup2_gt_back", 30, 90, 40, 3, "Back");
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

LeagueStandingsICup2GroupTablePage::~LeagueStandingsICup2GroupTablePage() {}

LeagueStandingsICup2TreePage::LeagueStandingsICup2TreePage(Gui2WindowManager* windowManager,
                                                           const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* frame =
      new Gui2Frame(windowManager, "frame_standings_icup2_tree", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_league_standings_icup2_tree", 10, 5,
                                       80, 3, "International Cup 2 - Tournament Tree");
  frame->AddView(title);
  title->Show();

  Gui2Caption* info = new Gui2Caption(
      windowManager, "caption_icup2_tree_info", 10, 15, 80, 8,
      "Coming Soon - International Cup 2 tournaments will be available in a future update.");
  frame->AddView(info);
  info->Show();

  Gui2Button* btnBack = new Gui2Button(windowManager, "btn_icup2_tree_back", 30, 90, 40, 3, "Back");
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

LeagueStandingsICup2TreePage::~LeagueStandingsICup2TreePage() {}

LeagueStandingsICup2StatsPage::LeagueStandingsICup2StatsPage(Gui2WindowManager* windowManager,
                                                             const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* frame =
      new Gui2Frame(windowManager, "frame_standings_icup2_stats", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_league_standings_icup2_stats", 10, 5,
                                       80, 3, "International Cup 2 - Stats");
  frame->AddView(title);
  title->Show();

  Gui2Caption* info = new Gui2Caption(
      windowManager, "caption_icup2_stats_info", 10, 15, 80, 8,
      "Coming Soon - International Cup 2 tournaments will be available in a future update.");
  frame->AddView(info);
  info->Show();

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_icup2_stats_back", 30, 90, 40, 3, "Back");
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

LeagueStandingsICup2StatsPage::~LeagueStandingsICup2StatsPage() {}
