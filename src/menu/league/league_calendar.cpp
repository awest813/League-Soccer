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
      fixturesTable(nullptr),
      backButton(nullptr),
      pageCreatedTime_ms(league_menu_smoke::Now_ms()),
      autoAdvanceTriggered(false) {
  auto& loc = Localization::GetInstance();

  frame = new Gui2Frame(windowManager, "frame_league_cal", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_league_calendar", 2, 2, 66, 3,
                                       loc.Translate("league_calendar"));
  frame->AddView(title);
  title->Show();

  Gui2Caption* filterLabel = new Gui2Caption(windowManager, "caption_cal_filter", 2, 6, 20, 2.5,
                                             loc.Translate("league_calendar_filter"));
  frame->AddView(filterLabel);
  filterLabel->Show();

  leagueFilterPulldown = new Gui2Pulldown(windowManager, "pulldown_cal_league", 22, 6, 30, 3);
  leagueFilterPulldown->AddEntry(loc.Translate("league_all_leagues"), "all");

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
  if (fixturesTable) {
    fixturesTable->Exit();
    delete fixturesTable;
    fixturesTable = nullptr;
  }
  if (backButton) {
    backButton->Exit();
    delete backButton;
    backButton = nullptr;
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

  // Fixture list, home team right-aligned against "v"; the user's fixtures
  // highlighted. Activate offers to simulate one.
  auto& loc = Localization::GetInstance();
  using Align = Gui2Table::Align;
  fixturesTable = new Gui2Table(windowManager, "table_cal", 2, 10.5f,
                                {{loc.Translate("matchhistory_date"), 11, Align::Left},
                                 {"", 20, Align::Right},
                                 {"", 4, Align::Center},
                                 {"", 20, Align::Left},
                                 {loc.Translate("career_col_league"), 11, Align::Left}},
                                3.6f, 18);
  std::vector<std::string> ids, homes, aways;
  for (const auto& r : result->data) {
    Gui2Table::Row row;
    row.cells = {r.at(1).substr(0, 10), r.at(2), "v", r.at(3), r.at(4)};
    row.highlight = r.size() > 5 && r.at(5) == "1";
    fixturesTable->AddRow(row);
    ids.push_back(r.at(0));
    homes.push_back(r.at(2));
    aways.push_back(r.at(3));
  }
  fixturesTable->SetSelectable(true);
  fixturesTable->sig_OnActivate.connect([this, ids, homes, aways](int index) {
    if (index < 0 || index >= static_cast<int>(ids.size()))
      return;
    const std::string calID = ids[index];
    const std::string title = homes[index] + " v " + aways[index];
    Gui2Dialog* dlg = new Gui2Dialog(windowManager, "dialog_fixture", 25, 25, 50, 50, title);
    Gui2Text* txt = new Gui2Text(windowManager, "text_fixture", 5, 5, 90, 70, 2.5, 40, "");
    txt->AddText(title);
    txt->AddEmptyLine();
    txt->AddText(Localization::GetInstance().Translate("league_simulate_prompt"));
    dlg->AddContent(txt);
    (dlg->AddSingleButton(Localization::GetInstance().Translate("league_simulate")))->SetFocus();
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
  if (fixturesTable->GetRowCount() > 0) {
    fixturesTable->Build();
    frame->AddView(fixturesTable);
    fixturesTable->Show();
  }

  backButton = new Gui2Button(windowManager, "btn_cal_back", 20, 84, 30, 3.6f,
                              loc.Translate("league_back_dashboard"));
  backButton->sig_OnClick.connect([this](...) { GoBack(); });
  frame->AddView(backButton);
  backButton->Show();

  if (fixturesTable->GetRowCount() > 0)
    fixturesTable->SetFocus();
  else
    backButton->SetFocus();
}

void LeagueCalendarPage::GoBack() {
  this->Exit();
  Properties properties;
  windowManager->GetPageFactory()->CreatePage(static_cast<int>(e_PageID_League_Forward), properties,
                                              0);
  delete this;
}
