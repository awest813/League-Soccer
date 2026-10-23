#include "matchhistorypage.hpp"

#include "../../data/matchhistory.hpp"
#include "../pagefactory.hpp"
#include "main.hpp"
#include "utils/gui2/widgets/button.hpp"
#include "utils/gui2/widgets/caption.hpp"
#include "utils/gui2/widgets/grid.hpp"
#include "utils/gui2/widgets/image.hpp"
#include "utils/gui2/widgets/table.hpp"
#include "utils/localization.hpp"

using namespace blunted;

MatchHistoryPage::MatchHistoryPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* bg = new Gui2Frame(windowManager, "frame_matchhistory_bg", 5, 5, 90, 90, true);
  this->AddView(bg);
  bg->Show();

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_matchhistory_title", 0, 8, 100, 4,
                                       Localization::GetInstance().Translate("matchhistory_title"));
  title->SetPosition(50 - title->GetTextWidthPercent() / 2, 8);
  this->AddView(title);
  title->Show();

  std::vector<MatchHistoryEntry> entries = MatchHistory::LoadRecent(10);

  // A results list as football games show them: home team right-aligned
  // against the score, away team left-aligned.
  using Align = Gui2Table::Align;
  auto& loc = Localization::GetInstance();
  Gui2Table* table = new Gui2Table(windowManager, "table_matchhistory", 9, 15,
                                   {{loc.Translate("matchhistory_date"), 13, Align::Left},
                                    {"", 24, Align::Right},
                                    {loc.Translate("matchhistory_score"), 9, Align::Center},
                                    {"", 24, Align::Left},
                                    {loc.Translate("matchhistory_possession"), 12, Align::Center}},
                                   5, 10);
  for (const MatchHistoryEntry& e : entries) {
    Gui2Table::Row row;
    row.cells = {e.timestamp.substr(0, 10), e.team1_name,
                 int_to_str(e.score1) + " - " + int_to_str(e.score2), e.team2_name,
                 int_to_str((int)round(e.possession1_pct)) + "% - " +
                     int_to_str((int)round(e.possession2_pct)) + "%"};
    table->AddRow(row);
  }

  if (entries.empty()) {
    Gui2Caption* emptyCaption = new Gui2Caption(windowManager, "mh_empty", 8, 22, 84, 3,
                                                loc.Translate("matchhistory_empty"));
    this->AddView(emptyCaption);
    emptyCaption->Show();
  } else {
    table->Build();
    this->AddView(table);
    table->Show();
  }

  Gui2Button* buttonBack = new Gui2Button(windowManager, "button_matchhistory_back", 40, 88.5f, 20,
                                          4, Localization::GetInstance().Translate("action_back"));
  this->AddView(buttonBack);
  buttonBack->Show();
  buttonBack->sig_OnClick.connect([this](...) { GoBack(); });
  buttonBack->SetFocus();

  this->Show();
}

MatchHistoryPage::~MatchHistoryPage() {}
