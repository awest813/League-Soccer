#include "career_standings_page.hpp"

#include "../../main.hpp"
#include "../pagefactory.hpp"
#include "career_common.hpp"
#include "career_navigation.hpp"
#include "utils/database.hpp"
#include "utils/localization.hpp"

namespace blunted {

CareerStandingsPage::CareerStandingsPage(Gui2WindowManager* windowManager,
                                         const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData),
      frame(nullptr),
      standingsGrid(nullptr),
      scorersGrid(nullptr),
      m_fromMenu(false) {
  CareerSave* activeSave = CareerDatabase::GetInstance().GetActiveSave();

  frame = new Gui2Frame(windowManager, "frame_career_standings", 3, 2, 94, 96, true);
  this->AddView(frame);
  frame->Show();

  std::string leagueName = activeSave ? activeSave->club.leagueName : "";
  int seasonNum = activeSave ? activeSave->season.currentSeason : 1;
  int currentWeek = activeSave ? activeSave->season.currentWeek : 1;

  Gui2Caption* title =
      new Gui2Caption(windowManager, "cap_standings_title", 2, 1.5f, 90, 3,
                      leagueName.empty() ? TR("career_standings_title")
                                         : leagueName + " — " + TR("career_standings_title"));
  title->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  frame->AddView(title);
  title->Show();

  Gui2Caption* subtitle = new Gui2Caption(windowManager, "cap_standings_sub", 2, 4.5f, 90, 2, "");
  frame->AddView(subtitle);
  subtitle->Show();

  // Top scorers header
  Gui2Caption* scHdr = new Gui2Caption(windowManager, "cap_sc_hdr", 66, 7.5f, 26, 2.2f,
                                       TR("career_top_scorers_title"));
  scHdr->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  frame->AddView(scHdr);
  scHdr->Show();

  PopulateStandingsGrid();
  PopulateScorersGrid();
  if (activeSave) {
    // The grid has fixed this season's table; rank the club from it.
    subtitle->SetCaption(
        TRF("career_standings_sub", {std::to_string(seasonNum), std::to_string(currentWeek),
                                     std::to_string(activeSave->season.maxWeeks), activeSave->name,
                                     std::to_string(CareerSim::LeaguePosition(*activeSave))}));
  }

  // Bottom action bar
  Gui2Button* btnMatchday = new Gui2Button(windowManager, "btn_st_matchday", 2, 91.5f, 28, 3.2f,
                                           TR("career_play_matchday"));
  btnMatchday->sig_OnClick.connect([this](...) { GoMatchday(); });
  frame->AddView(btnMatchday);
  btnMatchday->Show();

  Gui2Button* btnSeason = new Gui2Button(windowManager, "btn_st_season", 33, 91.5f, 28, 3.2f,
                                         TR("career_season_review_title"));
  btnSeason->sig_OnClick.connect([this](...) { GoSeason(); });
  frame->AddView(btnSeason);
  btnSeason->Show();

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_st_back", 64, 91.5f, 28, 3.2f, TR("career_back_hub"));
  btnBack->sig_OnClick.connect([this](...) { GoBack(); });
  frame->AddView(btnBack);
  btnBack->Show();

  btnMatchday->SetFocus();
  this->Show();
}

CareerStandingsPage::~CareerStandingsPage() {}

void CareerStandingsPage::PopulateStandingsGrid() {
  CareerSave* activeSave = CareerDatabase::GetInstance().GetActiveSave();
  if (!activeSave)
    return;

  std::vector<std::pair<int, std::string>> leagueClubs;
  try {
    // Resolve the league through the club's integer ID rather than pasting the
    // saved league name into SQL (an apostrophe would break the query).
    auto result = GetDB()->Query(
        "SELECT t.id, t.name FROM teams t "
        "WHERE t.league_id = (SELECT league_id FROM teams WHERE id = " +
        std::to_string(activeSave->club.clubID) +
        ") "
        "ORDER BY t.id");
    if (result && !result->data.empty()) {
      for (unsigned int r = 0; r < result->data.size(); r++) {
        leagueClubs.emplace_back(atoi(result->data.at(r).at(0).c_str()), result->data.at(r).at(1));
      }
    }
  } catch (...) {
  }

  auto table = CareerDatabase::GetInstance().GetLeagueStandings(leagueClubs);

  // One caption per column, header row included, so the figures line up (a
  // space-padded string does not in a proportional font).
  standingsGrid = new Gui2Grid(windowManager, "grid_standings", 2, 7.5f, 62, 84);

  const Vector3 colorUser(250, 215, 60);        // Gold/accent for user
  const Vector3 colorLeader(110, 230, 110);     // Green for title leader
  const Vector3 colorEuro(100, 185, 245);       // Blue for top 4
  const Vector3 colorRelegated(240, 110, 110);  // Red for relegation
  const float widths[] = {3.5f, 20, 3.2f, 3.2f, 3.2f, 3.2f, 3.8f, 3.8f, 3.8f, 3.8f, 6.5f};
  auto addRow = [&](int row, const std::vector<std::string>& cells, const Vector3& color) {
    for (size_t c = 0; c < cells.size(); c++) {
      auto* cell =
          new Gui2Caption(windowManager, "cap_st_" + std::to_string(row) + "_" + std::to_string(c),
                          0, 0, widths[c], 2.3f, cells[c]);
      cell->SetColor(color);
      standingsGrid->AddView(cell, row, static_cast<int>(c));
    }
  };
  addRow(0,
         {TR("career_standings_pos"), TR("career_standings_club"), TR("career_standings_p"),
          TR("career_standings_w"), TR("career_standings_d"), TR("career_standings_l"),
          TR("career_standings_gf"), TR("career_standings_ga"), TR("career_standings_gd"),
          TR("career_standings_pts"), TR("career_standings_form")},
         windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));

  const int relegationFrom = static_cast<int>(table.size()) - 2;
  for (size_t i = 0; i < table.size(); i++) {
    const auto& tr = table[i];
    const int pos = static_cast<int>(i + 1);
    Vector3 color = windowManager->GetStyle()->GetColor(e_DecorationType_Bright1);
    if (tr.isUserTeam)
      color = colorUser;
    else if (pos == 1)
      color = colorLeader;
    else if (pos <= 4)
      color = colorEuro;
    else if (pos >= relegationFrom)
      color = colorRelegated;
    const std::string goalDiff = (tr.goalDiff > 0 ? "+" : "") + std::to_string(tr.goalDiff);
    addRow(pos,
           {std::to_string(pos) + ".", tr.name, std::to_string(tr.played), std::to_string(tr.wins),
            std::to_string(tr.draws), std::to_string(tr.losses), std::to_string(tr.goalsFor),
            std::to_string(tr.goalsAgainst), goalDiff, std::to_string(tr.points), tr.form},
           color);
  }

  standingsGrid->SetReadOnlyScrolling(true);
  standingsGrid->SetMaxVisibleRows(28);
  standingsGrid->UpdateLayout(0.2f, 0.2f, 0.1f, 0.1f);
  frame->AddView(standingsGrid);
  standingsGrid->Show();
}

void CareerStandingsPage::PopulateScorersGrid() {
  auto scorers = CareerDatabase::GetInstance().GetTopScorers();

  scorersGrid = new Gui2Grid(windowManager, "grid_scorers", 66, 10, 26, 80);

  const Vector3 colorUserPlayer(250, 215, 60);

  // Two lines per scorer: rank and name, then club and goals.
  int row = 0;
  for (size_t i = 0; i < scorers.size(); i++) {
    const auto& s = scorers[i];
    const Vector3 color = s.isUserPlayer
                              ? colorUserPlayer
                              : windowManager->GetStyle()->GetColor(e_DecorationType_Bright1);
    const std::string goals = s.goals == 1 ? TR("career_scorer_goal_one")
                                           : TRF("career_scorer_goals", {std::to_string(s.goals)});
    auto* name = new Gui2Caption(windowManager, "cap_sc_name_" + std::to_string(i), 0, 0, 26, 2.3f,
                                 std::to_string(i + 1) + ". " + s.playerName);
    name->SetColor(color);
    scorersGrid->AddView(name, row++, 0);
    auto* club = new Gui2Caption(windowManager, "cap_sc_club_" + std::to_string(i), 0, 0, 26, 1.9f,
                                 "    " + s.teamName + " — " + goals);
    club->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
    scorersGrid->AddView(club, row++, 0);
  }

  scorersGrid->SetReadOnlyScrolling(true);
  scorersGrid->SetMaxVisibleRows(20);
  scorersGrid->UpdateLayout(0.2f, 0.2f, 0.2f, 0.2f);
  frame->AddView(scorersGrid);
  scorersGrid->Show();
}

void CareerStandingsPage::GoBack() {
  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
  const int hubPage = CareerHubPageID(save);
  Properties props;
  CreatePage(hubPage, props);
}

void CareerStandingsPage::GoMatchday() {
  Properties props;
  CreatePage(e_PageID_CareerMatchday, props);
}

void CareerStandingsPage::GoSeason() {
  Properties props;
  CreatePage(e_PageID_CareerSeason, props);
}

}  // namespace blunted
