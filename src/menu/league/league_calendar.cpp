#include "league_calendar.hpp"

#include <algorithm>
#include <cstdlib>
#include <functional>

#include "../../league/leaguecode.hpp"
#include "../../main.hpp"
#include "../pagefactory.hpp"
#include "base/utils.hpp"
#include "menu_smoke.hpp"
#include "utils/gui2/widgets/dialog.hpp"
#include "utils/gui2/widgets/text.hpp"
#include "utils/localization.hpp"

LeagueCalendarPage::LeagueCalendarPage(Gui2WindowManager* windowManager,
                                       const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData),
      fixturesHeader(nullptr),
      fixturesGrid(nullptr),
      pageCreatedTime_ms(league_menu_smoke::Now_ms()),
      autoAdvanceTriggered(false) {
  auto& loc = Localization::GetInstance();

  frame = new Gui2Frame(windowManager, "frame_league_cal", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_league_calendar", 2, 2, 66, 3,
                      loc.Translate("league_calendar"));
  frame->AddView(title);
  title->Show();

  Gui2Caption* filterLabel =
      new Gui2Caption(windowManager, "caption_cal_filter", 2, 6, 20, 2.5,
                      loc.Translate("league_calendar_filter"));
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
      "SELECT c.id, c.timestamp, t1.name, t2.name, l.name, "
      "(c.team1_id = (SELECT team_id FROM settings) OR "
      " c.team2_id = (SELECT team_id FROM settings)) AS mine "
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
                      Localization::GetInstance().Translate("league_calendar_header"));
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
    const bool isUserFixture = (r.size() > 5 && r.at(5) == "1");
    Gui2Button* btn =
        new Gui2Button(windowManager, "btn_fixture_" + std::to_string(row), 0, 0, 65, 2.5, buf);
    if (isUserFixture) {
      btn->SetColor(Vector3(250, 210, 60));
    }
    btn->sig_OnClick.connect([this, calID, homeTeam, awayTeam](...) {
      Gui2Dialog* dlg = new Gui2Dialog(windowManager, "dialog_fixture", 25, 25, 50, 50,
                                       homeTeam + " vs " + awayTeam);
      Gui2Text* txt = new Gui2Text(windowManager, "text_fixture", 5, 5, 90, 70, 2.5, 40, "");
      txt->AddText(homeTeam + " vs " + awayTeam);
      txt->AddEmptyLine();
      txt->AddText(Localization::GetInstance().Translate("league_simulate_prompt"));
      dlg->AddContent(txt);

      (dlg->AddSingleButton(Localization::GetInstance().Translate("league_simulate")))
          ->SetFocus();
      dlg->sig_OnPositive.connect([this, dlg, calID](...) {
        LeagueFixtureInfo fixture;
        if (LeagueGetFixtureByCalendarID(atoi(calID.c_str()), fixture)) {
          int goals1 = 0;
          int goals2 = 0;
          LeagueSimulateFixture(fixture, goals1, goals2);
          LeagueRecordResult(fixture, goals1, goals2);
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
  // Back lives in the same grid so keyboard/gamepad can reach it too.
  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_cal_back", 0, 0, 65, 2.5,
                     Localization::GetInstance().Translate("league_back_dashboard"));
  btnBack->sig_OnClick.connect([this](...) { GoBack(); });
  fixturesGrid->AddView(btnBack, row, 0);

  fixturesGrid->UpdateLayout(0.5);
  frame->AddView(fixturesGrid);
  fixturesGrid->Show();

  if (fixturesGrid->IsSelectable()) {
    fixturesGrid->SetFocus();
  }
}

void LeagueCalendarPage::GoBack() {
  this->Exit();
  Properties properties;
  windowManager->GetPageFactory()->CreatePage(static_cast<int>(e_PageID_League_Forward), properties,
                                              0);
  delete this;
}
