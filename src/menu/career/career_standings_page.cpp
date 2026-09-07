#include "career_standings_page.hpp"

#include "../../main.hpp"
#include "../pagefactory.hpp"
#include "career_common.hpp"
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

  std::string leagueName = activeSave ? activeSave->club.leagueName : "League";
  int seasonNum = activeSave ? activeSave->season.currentSeason : 1;
  int currentWeek = activeSave ? activeSave->season.currentWeek : 1;

  Gui2Caption* title = new Gui2Caption(windowManager, "cap_standings_title", 2, 1.5f, 90, 3,
                                       "🏆 " + leagueName + " — " + TR("career_standings_title"));
  title->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  frame->AddView(title);
  title->Show();

  std::string sub = "Season " + std::to_string(seasonNum) + " | Matchday " +
                    std::to_string(currentWeek) + " of 38";
  if (activeSave) {
    int pos = CareerDatabase::EstimateLeaguePosition(
        activeSave->seasonWins, activeSave->seasonDraws, activeSave->seasonLosses);
    sub += " | Current Club: " + activeSave->name + " (Rank #" + std::to_string(pos) + ")";
  }

  Gui2Caption* subtitle = new Gui2Caption(windowManager, "cap_standings_sub", 2, 4.5f, 90, 2, sub);
  frame->AddView(subtitle);
  subtitle->Show();

  // Standings table header
  Gui2Caption* tblHdr =
      new Gui2Caption(windowManager, "cap_tbl_hdr", 2, 7.5f, 62, 2.2f,
                      "Pos  Club                        P   W   D   L   GF   GA   GD  Pts  Form");
  tblHdr->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  frame->AddView(tblHdr);
  tblHdr->Show();

  // Top scorers header
  Gui2Caption* scHdr = new Gui2Caption(windowManager, "cap_sc_hdr", 66, 7.5f, 26, 2.2f,
                                       "⚽ " + TR("career_top_scorers_title"));
  scHdr->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  frame->AddView(scHdr);
  scHdr->Show();

  PopulateStandingsGrid();
  PopulateScorersGrid();

  // Bottom action bar
  Gui2Button* btnMatchday = new Gui2Button(windowManager, "btn_st_matchday", 2, 91.5f, 28, 3.2f,
                                           "⚽ " + TR("career_play_matchday"));
  btnMatchday->sig_OnClick.connect([this](...) { GoMatchday(); });
  frame->AddView(btnMatchday);
  btnMatchday->Show();

  Gui2Button* btnSeason = new Gui2Button(windowManager, "btn_st_season", 33, 91.5f, 28, 3.2f,
                                         "📅 " + TR("career_season_review_title"));
  btnSeason->sig_OnClick.connect([this](...) { GoSeason(); });
  frame->AddView(btnSeason);
  btnSeason->Show();

  Gui2Button* btnBack = new Gui2Button(windowManager, "btn_st_back", 64, 91.5f, 28, 3.2f,
                                       "🔙 " + TR("career_back_hub"));
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
    auto result = GetDB()->Query(
        "SELECT t.id, t.name FROM teams t "
        "JOIN leagues l ON t.league_id = l.id "
        "WHERE l.name = '" +
        activeSave->club.leagueName +
        "' "
        "ORDER BY t.id");
    if (result && !result->data.empty()) {
      for (unsigned int r = 0; r < result->data.size(); r++) {
        leagueClubs.emplace_back(atoi(result->data.at(r).at(0).c_str()), result->data.at(r).at(1));
      }
    }
  } catch (...) {
  }

  auto table = CareerDatabase::GetInstance().GetLeagueStandings(leagueClubs);

  standingsGrid = new Gui2Grid(windowManager, "grid_standings", 2, 10, 62, 80);

  const Vector3 colorUser(250, 215, 60);        // Gold/accent for user
  const Vector3 colorLeader(110, 230, 110);     // Green for title leader
  const Vector3 colorEuro(100, 185, 245);       // Blue for top 4
  const Vector3 colorRelegated(240, 110, 110);  // Red for relegation

  int row = 0;
  for (size_t i = 0; i < table.size(); i++) {
    const auto& tr = table[i];
    int pos = static_cast<int>(i + 1);

    char lineBuf[256];
    snprintf(lineBuf, sizeof(lineBuf), "%2d. %-24s %2d  %2d  %2d  %2d  %3d  %3d %+3d  %3d  %-5s",
             pos, tr.name.c_str(), tr.played, tr.wins, tr.draws, tr.losses, tr.goalsFor,
             tr.goalsAgainst, tr.goalDiff, tr.points, tr.form.c_str());

    Gui2Caption* rowCap =
        new Gui2Caption(windowManager, "cap_st_row_" + std::to_string(i), 0, 0, 62, 2.3f, lineBuf);

    if (tr.isUserTeam) {
      rowCap->SetColor(colorUser);
    } else if (pos == 1) {
      rowCap->SetColor(colorLeader);
    } else if (pos <= 4) {
      rowCap->SetColor(colorEuro);
    } else if (pos >= 18) {
      rowCap->SetColor(colorRelegated);
    } else {
      rowCap->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright1));
    }

    standingsGrid->AddView(rowCap, row++, 0);
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

  int row = 0;
  for (size_t i = 0; i < scorers.size(); i++) {
    const auto& s = scorers[i];
    int rank = static_cast<int>(i + 1);

    std::string line = std::to_string(rank) + ". " + s.playerName + "\n   " + s.teamName + " — " +
                       std::to_string(s.goals) + " goals";

    Gui2Caption* cap =
        new Gui2Caption(windowManager, "cap_sc_row_" + std::to_string(i), 0, 0, 26, 4.2f, line);

    if (s.isUserPlayer) {
      cap->SetColor(colorUserPlayer);
    } else {
      cap->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright1));
    }

    scorersGrid->AddView(cap, row++, 0);
  }

  scorersGrid->SetReadOnlyScrolling(true);
  scorersGrid->SetMaxVisibleRows(16);
  scorersGrid->UpdateLayout(0.2f, 0.2f, 0.2f, 0.2f);
  frame->AddView(scorersGrid);
  scorersGrid->Show();
}

void CareerStandingsPage::GoBack() {
  CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
  const int hubPage =
      (save && save->mode == CareerMode::OWNER) ? (int)e_PageID_OwnerHub : (int)e_PageID_CareerHub;
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
