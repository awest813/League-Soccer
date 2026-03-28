#include "matchhistorypage.hpp"

#include "../../data/matchhistory.hpp"
#include "../pagefactory.hpp"
#include "main.hpp"

using namespace blunted;

MatchHistoryPage::MatchHistoryPage(Gui2WindowManager* windowManager,
                                   const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Image* bg = new Gui2Image(windowManager, "image_matchhistory_bg", 5, 5, 90, 90);
  bg->LoadImage("media/menu/backgrounds/black.png");
  this->AddView(bg);
  bg->Show();

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_matchhistory_title", 0, 8, 100, 4,
                                       "match history");
  title->SetPosition(50 - title->GetTextWidthPercent() / 2, 8);
  this->AddView(title);
  title->Show();

  std::vector<MatchHistoryEntry> entries = MatchHistory::LoadRecent(10);

  Gui2Grid* grid = new Gui2Grid(windowManager, "grid_matchhistory", 8, 16, 84, 72);

  // Header row
  grid->AddView(new Gui2Caption(windowManager, "mh_hdr_date", 0, 0, 22, 3, "date"), 0, 0);
  grid->AddView(new Gui2Caption(windowManager, "mh_hdr_match", 0, 0, 40, 3, "match"), 0, 1);
  grid->AddView(new Gui2Caption(windowManager, "mh_hdr_score", 0, 0, 12, 3, "score"), 0, 2);
  grid->AddView(new Gui2Caption(windowManager, "mh_hdr_poss", 0, 0, 20, 3, "possession"), 0, 3);

  for (int i = 0; i < (int)entries.size(); i++) {
    const MatchHistoryEntry& e = entries[i];
    int row = i + 1;
    std::string rowSfx = std::to_string(i);

    grid->AddView(
        new Gui2Caption(windowManager, "mh_date_" + rowSfx, 0, 0, 22, 3, e.timestamp.substr(0, 10)),
        row, 0);
    grid->AddView(
        new Gui2Caption(windowManager, "mh_match_" + rowSfx, 0, 0, 40, 3,
                        e.team1_name + " vs " + e.team2_name),
        row, 1);
    grid->AddView(
        new Gui2Caption(windowManager, "mh_score_" + rowSfx, 0, 0, 12, 3,
                        int_to_str(e.score1) + " - " + int_to_str(e.score2)),
        row, 2);
    grid->AddView(
        new Gui2Caption(windowManager, "mh_poss_" + rowSfx, 0, 0, 20, 3,
                        int_to_str((int)round(e.possession1_pct)) + "% / " +
                            int_to_str((int)round(e.possession2_pct)) + "%"),
        row, 3);
  }

  if (entries.empty()) {
    grid->AddView(
        new Gui2Caption(windowManager, "mh_empty", 0, 0, 84, 3, "no matches recorded yet"), 1, 0);
  }

  grid->UpdateLayout(0.5);
  this->AddView(grid);
  grid->Show();

  Gui2Button* buttonBack =
      new Gui2Button(windowManager, "button_matchhistory_back", 40, 90, 20, 3, "back");
  this->AddView(buttonBack);
  buttonBack->Show();
  buttonBack->sig_OnClick.connect([this](...) { GoBack(); });
  buttonBack->SetFocus();

  this->Show();
}

MatchHistoryPage::~MatchHistoryPage() {}
