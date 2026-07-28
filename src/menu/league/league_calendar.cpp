#include "league_calendar.hpp"

#include <algorithm>
#include <cstdlib>
#include <functional>

#include "../../main.hpp"
#include "../pagefactory.hpp"
#include "base/utils.hpp"
#include "menu_smoke.hpp"
#include "utils/gui2/widgets/dialog.hpp"
#include "utils/gui2/widgets/text.hpp"

LeagueCalendarPage::LeagueCalendarPage(Gui2WindowManager* windowManager,
                                       const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData),
      fixturesHeader(nullptr),
      fixturesGrid(nullptr),
      pageCreatedTime_ms(league_menu_smoke::Now_ms()),
      autoAdvanceTriggered(false) {
  frame = new Gui2Frame(windowManager, "frame_league_cal", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_league_calendar", 2, 2, 66, 3, "Calendar / Fixtures");
  frame->AddView(title);
  title->Show();

  Gui2Caption* filterLabel =
      new Gui2Caption(windowManager, "caption_cal_filter", 2, 6, 20, 2.5, "Filter by League:");
  frame->AddView(filterLabel);
  filterLabel->Show();

  leagueFilterPulldown = new Gui2Pulldown(windowManager, "pulldown_cal_league", 22, 6, 30, 3);
  leagueFilterPulldown->AddEntry("All Leagues", "all");

  auto leaguesResult = GetDB()->Query("SELECT id, name FROM leagues ORDER BY name");
  for (const auto& row : leaguesResult->data) {
    leagueFilterPulldown->AddEntry(row.at(1), row.at(0));
  }
  m_selectedLeagueID = "all";
  leagueFilterPulldown->sig_OnChange.connect([this](Gui2Pulldown* pd) {
    m_selectedLeagueID = pd->GetSelected();
    RefreshFixtures();
  });
  frame->AddView(leagueFilterPulldown);
  leagueFilterPulldown->Show();

  RefreshFixtures();

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_cal_back", 30, 92, 40, 3, "Back to Dashboard");
  btnBack->sig_OnClick.connect([this](...) { GoBack(); });
  frame->AddView(btnBack);
  btnBack->Show();

  this->SetFocus();
  this->Show();
}

LeagueCalendarPage::~LeagueCalendarPage() {}

void LeagueCalendarPage::Process() {
  Gui2Page::Process();

  if (!league_menu_smoke::RouteEnabled("calendar") || autoAdvanceTriggered ||
      league_menu_smoke::Now_ms() < pageCreatedTime_ms + league_menu_smoke::kQuitDelay_ms) {
    return;
  }

  autoAdvanceTriggered = true;
  printf("[menu-smoke] League calendar reached successfully\n");
  GetMenuTask()->QuitGame();
}

void LeagueCalendarPage::RefreshFixtures() {
  if (fixturesHeader) {
    fixturesHeader->Exit();
    delete fixturesHeader;
    fixturesHeader = nullptr;
  }
  if (fixturesGrid) {
    fixturesGrid->Exit();
    delete fixturesGrid;
    fixturesGrid = nullptr;
  }

  std::string query =
      "SELECT c.id, c.timestamp, t1.name, t2.name, l.name "
      "FROM calendar c "
      "JOIN teams t1 ON c.team1_id = t1.id "
      "JOIN teams t2 ON c.team2_id = t2.id "
      "JOIN leagues l ON c.competition_id = l.id ";
  if (m_selectedLeagueID != "all") {
    query += "WHERE c.competition_id = " + m_selectedLeagueID + " ";
  }
  query += "ORDER BY c.timestamp LIMIT 40";

  auto result = GetDB()->Query(query);
  fixturesHeader =
      new Gui2Caption(windowManager, "caption_cal_header", 2, 10, 66, 2,
                      "Date              | Home                | Away                | League");
  frame->AddView(fixturesHeader);
  fixturesHeader->Show();

  fixturesGrid = new Gui2Grid(windowManager, "grid_cal", 2, 13, 66, 75);
  int row = 0;
  for (const auto& r : result->data) {
    std::string calID = r.at(0);
    char buf[256];
    snprintf(buf, sizeof(buf), "%-17s | %-19s | %-19s | %s", r.at(1).c_str(), r.at(2).c_str(),
             r.at(3).c_str(), r.at(4).c_str());
    std::string homeTeam = r.at(2);
    std::string awayTeam = r.at(3);
    Gui2Button* btn =
        new Gui2Button(windowManager, "btn_fixture_" + std::to_string(row), 0, 0, 86, 2.5, buf);
    btn->sig_OnClick.connect([this, calID, homeTeam, awayTeam](...) {
      Gui2Dialog* dlg = new Gui2Dialog(windowManager, "dialog_fixture", 25, 25, 50, 50,
                                       homeTeam + " vs " + awayTeam);
      Gui2Text* txt = new Gui2Text(windowManager, "text_fixture", 5, 5, 90, 70, 2.5, 40, "");
      txt->AddText(homeTeam + " vs " + awayTeam);
      txt->AddEmptyLine();
      txt->AddText("Simulate this match to generate a result?");
      dlg->AddContent(txt);

      (dlg->AddSingleButton("Simulate"))->SetFocus();
      dlg->sig_OnPositive.connect([this, dlg, calID, homeTeam, awayTeam](...) {
        // Strength-aware result: the league stores no explicit ratings, so
        // derive a stable rating from each team's name and give the home side
        // an edge. Fixtures are no longer pure coin-flips.
        auto teamRating = [](const std::string& name) {
          return 50 + static_cast<int>(std::hash<std::string>{}(name) % 41);  // 50..90
        };
        auto sampleGoals = [](int attack, int defense) {
          float expected =
              1.35f * static_cast<float>(attack) / static_cast<float>(std::max(1, defense));
          int goals = static_cast<int>(expected);
          float frac = expected - static_cast<float>(goals);
          // Stochastic rounding keeps the expected goal average honest.
          if ((rand() % 1000) / 1000.0f < frac)
            goals += 1;
          // Occasional flair for the odd extra goal.
          if (rand() % 100 < 15)
            goals += rand() % 2;
          return std::max(0, std::min(6, goals));
        };
        int homeRating = teamRating(homeTeam) + 6;  // home advantage
        int awayRating = teamRating(awayTeam);
        int goals1 = sampleGoals(homeRating, awayRating);
        int goals2 = sampleGoals(awayRating, homeRating);
        auto calRow = GetDB()->Query(
            "SELECT team1_id, team2_id, competition_id FROM calendar WHERE id = " + calID);
        if (!calRow->data.empty()) {
          std::string t1 = calRow->data.at(0).at(0);
          std::string t2 = calRow->data.at(0).at(1);
          std::string comp = calRow->data.at(0).at(2);
          GetDB()->Query(
              "INSERT INTO match_results (calendar_id, team1_id, team2_id, team1_goals, "
              "team2_goals, played, competition_id) "
              "VALUES (" +
              calID + ", " + t1 + ", " + t2 + ", " + std::to_string(goals1) + ", " +
              std::to_string(goals2) + ", 1, " + comp + ")");

          std::string resultStr = homeTeam + " " + std::to_string(goals1) + " - " +
                                  std::to_string(goals2) + " " + awayTeam;
          GetDB()->Query(
              "INSERT INTO inbox_messages (sender, subject, body) VALUES "
              "('Match Reporter', 'Match Result: " +
              resultStr +
              "', "
              "'Full-time: " +
              resultStr + ". Check the Standings page for updated league tables.')");
        }
        dlg->Exit();
        delete dlg;
        RefreshFixtures();
      });
      this->AddView(dlg);
      dlg->Show();
    });
    fixturesGrid->AddView(btn, row++, 0);
  }
  fixturesGrid->UpdateLayout(0.5);
  frame->AddView(fixturesGrid);
  fixturesGrid->Show();
}

void LeagueCalendarPage::GoBack() {
  this->Exit();
  Properties properties;
  windowManager->GetPageFactory()->CreatePage(static_cast<int>(e_PageID_League_Forward), properties,
                                              0);
  delete this;
}
