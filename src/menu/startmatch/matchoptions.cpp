// written by bastiaan konings schuiling 2008 - 2015
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not
// be used for anything important. i do not offer support, so don't ask. to be used for inspiration
// :)

#include "matchoptions.hpp"

#include <cmath>

#include "../pagefactory.hpp"
#include "main.hpp"
#include "onthepitch/matchduration.hpp"
#include "utils/localization.hpp"

using namespace blunted;

namespace {

constexpr unsigned long kMenuSmokeAdvanceDelay_ms = 250;

bool MenuSmokeQuickMatchEnabled() {
  return GetConfiguration()->GetBool("menu_smoke_test_quick_match", false);
}

bool MenuSmokeFullMatchEnabled() {
  return GetConfiguration()->GetBool("menu_smoke_test_full_match", false);
}

bool MenuSmokeAutoQuickMatchEnabled() {
  return MenuSmokeQuickMatchEnabled() || MenuSmokeFullMatchEnabled();
}

}  // namespace

MatchOptionsPage::MatchOptionsPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData),
      buttonStart(nullptr),
      pageCreatedTime_ms(EnvironmentManager::GetInstance().GetTime_ms()),
      autoAdvanceTriggered(false) {
  Gui2Frame* frame = new Gui2Frame(windowManager, "matchoptions_frame", 25, 15, 50, 75, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* header = new Gui2Caption(windowManager, "matchoptions_caption", 2, 2, 46, 3,
                                        TR("match_options_title"));
  frame->AddView(header);
  header->Show();

  Gui2Grid* grid = new Gui2Grid(windowManager, "matchoptions_grid", 2, 10, 46, 60);

  difficultySlider = new Gui2Slider(windowManager, "matchoptions_slider_difficulty", 0, 0, 29, 6,
                                    TR("match_difficulty"));
  matchDurationSlider = new Gui2Slider(windowManager, "matchoptions_slider_matchduration", 0, 0, 29,
                                       6, TR("match_duration"));
  matchDurationSlider->SetQuantization(kMatchDurationSliderSteps);
  buttonStart =
      new Gui2Button(windowManager, "matchoptions_button_start", 0, 0, 29, 3, TR("match_start"));
  Gui2Button* buttonBack = new Gui2Button(windowManager, "matchoptions_button_back", 0, 0, 29, 3,
                                          Localization::GetInstance().Translate("action_back"));

  float difficulty = GetConfiguration()->GetReal("match_difficulty", _default_Difficulty);
  float matchDurationMinutes = kDefaultMatchDurationMinutes;
  if (GetConfiguration()->Exists("match_duration_minutes")) {
    matchDurationMinutes =
        GetConfiguration()->GetReal("match_duration_minutes", kDefaultMatchDurationMinutes);
  } else {
    matchDurationMinutes = MatchDurationMinutesFromLegacySlider(
        GetConfiguration()->GetReal("match_duration", _default_MatchDuration));
  }
  difficultySlider->SetValue(difficulty);
  matchDurationSlider->SetValue(MatchDurationSliderFromMinutes(matchDurationMinutes));
  UpdateMatchDurationCaption();
  matchDurationSlider->sig_OnChange.connect([this](Gui2Slider*) { UpdateMatchDurationCaption(); });

  grid->AddView(difficultySlider, 0, 0);
  grid->AddView(matchDurationSlider, 1, 0);
  grid->AddView(buttonStart, 2, 0);
  grid->AddView(buttonBack, 3, 0);
  grid->UpdateLayout(0.5);

  frame->AddView(grid);
  grid->Show();

  buttonStart->sig_OnClick.connect([this](...) { GoLoadingMatchPage(); });
  buttonBack->sig_OnClick.connect([this](...) { GoBack(); });

  buttonStart->SetFocus();

  this->Show();
}

MatchOptionsPage::~MatchOptionsPage() {}

void MatchOptionsPage::UpdateMatchDurationCaption() {
  const int minutes =
      static_cast<int>(std::round(MatchDurationMinutesFromSlider(matchDurationSlider->GetValue())));
  matchDurationSlider->SetCaption(TRF("match_duration_minutes", {std::to_string(minutes)}));
}

void MatchOptionsPage::Process() {
  Gui2Page::Process();

  if (!autoAdvanceTriggered && MenuSmokeAutoQuickMatchEnabled() &&
      EnvironmentManager::GetInstance().GetTime_ms() >=
          pageCreatedTime_ms + kMenuSmokeAdvanceDelay_ms) {
    autoAdvanceTriggered = true;
    printf("[menu-smoke] Match options confirmed, starting match load\n");
    GoLoadingMatchPage();
  }
}

void MatchOptionsPage::GoLoadingMatchPage() {
  GetConfiguration()->Set("match_difficulty", difficultySlider->GetValue());
  GetConfiguration()->Set("match_duration_minutes",
                          MatchDurationMinutesFromSlider(matchDurationSlider->GetValue()));
  GetConfiguration()->SaveFile(GetConfigFilename());

  this->Exit();

  Properties properties;
  windowManager->GetPageFactory()->CreatePage((int)e_PageID_LoadingMatch, properties, 0);

  delete this;
}
