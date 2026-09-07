#include "league_matchday.hpp"

#include <cstdio>

#include "../../league/leaguecode.hpp"
#include "../../main.hpp"
#include "menu_smoke.hpp"
#include "utils/localization.hpp"
#include "utils/database.hpp"

namespace {

std::string SafeCell(const std::unique_ptr<DatabaseResult>& result, size_t row, size_t col,
                     const std::string& fallback = "-") {
  if (!result || row >= result->data.size() || col >= result->data.at(row).size() ||
      result->data.at(row).at(col).empty()) {
    return fallback;
  }
  return result->data.at(row).at(col);
}

}  // namespace

LeagueMatchdayPage::LeagueMatchdayPage(Gui2WindowManager* windowManager,
                                       const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData),
      frame(nullptr),
      matchdayHeader(nullptr),
      resultsGrid(nullptr),
      pageCreatedTime_ms(league_menu_smoke::Now_ms()),
      autoAdvanceTriggered(false) {
  frame = new Gui2Frame(windowManager, "frame_league_matchday", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_league_matchday", 2, 2, 66, 3,
                      Localization::GetInstance().Translate("league_matchday_title"));
  frame->AddView(title);
  title->Show();

  RefreshResults();

  this->Show();
}

LeagueMatchdayPage::~LeagueMatchdayPage() {}

void LeagueMatchdayPage::Process() {
  Gui2Page::Process();

  if (!league_menu_smoke::RouteEnabled("matchday") || autoAdvanceTriggered ||
      league_menu_smoke::Now_ms() < pageCreatedTime_ms + league_menu_smoke::kQuitDelay_ms) {
    return;
  }

  autoAdvanceTriggered = true;
  printf("[menu-smoke] League matchday summary reached successfully\n");
  GetMenuTask()->QuitGame();
}

void LeagueMatchdayPage::RefreshResults() {
  if (matchdayHeader) {
    matchdayHeader->Exit();
    delete matchdayHeader;
    matchdayHeader = nullptr;
  }
  if (resultsGrid) {
    resultsGrid->Exit();
    delete resultsGrid;
    resultsGrid = nullptr;
  }

  int userTeamID = 0;
  LeagueGetUserTeamID(userTeamID);

  std::string date = LeagueGetLastMatchdayDate();

  auto result = GetDB()->Query(
      "SELECT t1.name, t2.name, mr.team1_goals, mr.team2_goals, c.team1_id, c.team2_id "
      "FROM calendar c "
      "JOIN match_results mr ON mr.calendar_id = c.id AND mr.played = 1 "
      "JOIN teams t1 ON c.team1_id = t1.id "
      "JOIN teams t2 ON c.team2_id = t2.id "
      "WHERE date(c.timestamp) = date('" +
      date + "') "
      "ORDER BY c.id");

  auto& loc = Localization::GetInstance();
  std::string headerCaption =
      date.empty() ? loc.Translate("league_matchday_none") : date;
  matchdayHeader =
      new Gui2Caption(windowManager, "caption_matchday_header", 2, 7, 66, 2, headerCaption);
  frame->AddView(matchdayHeader);
  matchdayHeader->Show();

  resultsGrid = new Gui2Grid(windowManager, "grid_matchday_results", 2, 10, 66, 68);
  int row = 0;
  for (const auto& r : result->data) {
    const std::string homeName = SafeCell(result, row, 0, "Home");
    const std::string awayName = SafeCell(result, row, 1, "Away");
    const std::string goals1 = SafeCell(result, row, 2, "0");
    const std::string goals2 = SafeCell(result, row, 3, "0");

    const bool userInvolved =
        (atoi(SafeCell(result, row, 4, "0").c_str()) == userTeamID ||
         atoi(SafeCell(result, row, 5, "0").c_str()) == userTeamID);

    char buf[256];
    snprintf(buf, sizeof(buf), "%s%-19s %3s - %-3s %s", userInvolved ? "> " : "  ",
             homeName.c_str(), goals1.c_str(), goals2.c_str(), awayName.c_str());

    Gui2Button* btn =
        new Gui2Button(windowManager, "btn_matchday_result_" + std::to_string(row), 0, 0, 65, 2.5,
                       buf);
    if (userInvolved) {
      btn->SetColor(Vector3(250, 210, 60));
    }
    resultsGrid->AddView(btn, row, 0);
    row++;

    if (row >= 24) {  // keep the grid bounded; standings carry the full picture
      break;
    }
  }

  if (row == 0) {
    Gui2Caption* empty = new Gui2Caption(
        windowManager, "caption_matchday_empty", 2, 1, 60, 2,
        loc.Translate("league_matchday_empty"));
    resultsGrid->AddView(empty, 0, 0);
    row = 1;
  }

  Gui2Button* btnStartSeason = nullptr;
  if (LeagueSeasonComplete()) {
    Gui2Caption* banner = new Gui2Caption(windowManager, "caption_matchday_season_complete", 0, 0,
                                          65, 2.5, loc.Translate("league_season_complete"));
    banner->SetColor(Vector3(250, 210, 60));
    resultsGrid->AddView(banner, row++, 0);

    btnStartSeason =
        new Gui2Button(windowManager, "btn_matchday_startseason", 0, 0, 65, 2.5,
                       loc.Translate("league_start_new_season"));
    btnStartSeason->sig_OnClick.connect([this](...) { StartNewSeason(); });
    resultsGrid->AddView(btnStartSeason, row++, 0);
  }

  Gui2Button* btnContinue =
      new Gui2Button(windowManager, "btn_matchday_continue", 0, 0, 65, 2.5,
                     loc.Translate("league_matchday_continue"));
  btnContinue->sig_OnClick.connect([this](...) { GoBackToDashboard(); });
  resultsGrid->AddView(btnContinue, row, 0);

  resultsGrid->SetMaxVisibleRows(14);
  resultsGrid->UpdateLayout(0.5);
  frame->AddView(resultsGrid);
  resultsGrid->Show();

  if (btnStartSeason) {
    btnStartSeason->SetFocus();
  } else {
    btnContinue->SetFocus();
  }
}

void LeagueMatchdayPage::StartNewSeason() {
  LeagueAdvanceSeason();
  this->Exit();
  Properties properties;
  windowManager->GetPageFactory()->CreatePage((int)e_PageID_League, properties, 0);
  delete this;
}

void LeagueMatchdayPage::GoBackToDashboard() {
  this->Exit();
  Properties properties;
  windowManager->GetPageFactory()->CreatePage((int)e_PageID_League_Forward, properties, 0);
  delete this;
}
