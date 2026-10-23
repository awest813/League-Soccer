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
      standingsTable(nullptr),
      scorersTable(nullptr),
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
  Gui2Caption* scHdr = new Gui2Caption(windowManager, "cap_sc_hdr", 64.5f, 7.5f, 27, 2.4f,
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
  Gui2Button* btnMatchday =
      new Gui2Button(windowManager, "btn_st_matchday", 2, 88, 28, 3.6f, TR("career_play_matchday"));
  btnMatchday->sig_OnClick.connect([this](...) { GoMatchday(); });
  frame->AddView(btnMatchday);
  btnMatchday->Show();

  Gui2Button* btnSeason = new Gui2Button(windowManager, "btn_st_season", 33, 88, 28, 3.6f,
                                         TR("career_season_review_title"));
  btnSeason->sig_OnClick.connect([this](...) { GoSeason(); });
  frame->AddView(btnSeason);
  btnSeason->Show();

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_st_back", 64, 88, 28, 3.6f, TR("career_back_hub"));
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

  // Broadcast-style table: banded rows, zone strips (title, Europe,
  // relegation) and the user's club highlighted, as in FIFA/PES tables.
  using Column = Gui2Table::Column;
  using Align = Gui2Table::Align;
  standingsTable = new Gui2Table(windowManager, "table_standings", 2, 7.5f,
                                 {{TR("career_standings_pos"), 3.5f, Align::Center},
                                  {TR("career_standings_club"), 19.5f, Align::Left},
                                  {TR("career_standings_p"), 3.4f, Align::Center},
                                  {TR("career_standings_w"), 3.4f, Align::Center},
                                  {TR("career_standings_d"), 3.4f, Align::Center},
                                  {TR("career_standings_l"), 3.4f, Align::Center},
                                  {TR("career_standings_gf"), 3.8f, Align::Center},
                                  {TR("career_standings_ga"), 3.8f, Align::Center},
                                  {TR("career_standings_gd"), 3.8f, Align::Center},
                                  {TR("career_standings_pts"), 4.2f, Align::Center},
                                  {TR("career_standings_form"), 7.0f, Align::Center}},
                                 3.6f, 22);
  const Vector3 colorLeader(80, 200, 110);
  const Vector3 colorEuro(70, 150, 235);
  const Vector3 colorRelegated(225, 70, 70);
  const int relegationFrom = static_cast<int>(table.size()) - 2;
  for (size_t i = 0; i < table.size(); i++) {
    const auto& tr = table[i];
    const int pos = static_cast<int>(i + 1);
    Gui2Table::Row row;
    row.cells = {std::to_string(pos),
                 tr.name,
                 std::to_string(tr.played),
                 std::to_string(tr.wins),
                 std::to_string(tr.draws),
                 std::to_string(tr.losses),
                 std::to_string(tr.goalsFor),
                 std::to_string(tr.goalsAgainst),
                 (tr.goalDiff > 0 ? "+" : "") + std::to_string(tr.goalDiff),
                 std::to_string(tr.points),
                 tr.form};
    row.highlight = tr.isUserTeam;
    if (pos == 1) {
      row.hasZone = true;
      row.zoneColor = colorLeader;
    } else if (pos <= 4) {
      row.hasZone = true;
      row.zoneColor = colorEuro;
    } else if (pos >= relegationFrom) {
      row.hasZone = true;
      row.zoneColor = colorRelegated;
    }
    standingsTable->AddRow(row);
  }
  standingsTable->Build();
  frame->AddView(standingsTable);
  standingsTable->Show();
}

void CareerStandingsPage::PopulateScorersGrid() {
  auto scorers = CareerDatabase::GetInstance().GetTopScorers();

  using Align = Gui2Table::Align;
  scorersTable = new Gui2Table(windowManager, "table_scorers", 64.5f, 10.5f,
                               {{"#", 2.6f, Align::Center},
                                {TR("career_top_scorers_player"), 10.8f, Align::Left},
                                {TR("career_top_scorers_team"), 9.0f, Align::Left},
                                {TR("career_top_scorers_goals"), 4.6f, Align::Center}},
                               3.6f, 10);
  for (size_t i = 0; i < scorers.size(); i++) {
    const auto& s = scorers[i];
    Gui2Table::Row row;
    row.cells = {std::to_string(i + 1), s.playerName, s.teamName, std::to_string(s.goals)};
    row.highlight = s.isUserPlayer;
    scorersTable->AddRow(row);
  }
  scorersTable->Build();
  frame->AddView(scorersTable);
  scorersTable->Show();
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
