// written by bastiaan konings schuiling 2008 - 2015
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not
// be used for anything important. i do not offer support, so don't ask. to be used for inspiration
// :)

#include "phasemenu.hpp"

#include "../gameplan.hpp"
#include "../pagefactory.hpp"
#include "main.hpp"
#include "utils/localization.hpp"

using namespace blunted;

namespace {

constexpr unsigned long kMenuSmokeAdvanceDelay_ms = 500;

bool MenuSmokeFullMatchEnabled() {
  return GetConfiguration()->GetBool("menu_smoke_test_full_match", false);
}

const char* PhaseName(e_MatchPhase phase) {
  switch (phase) {
    case e_MatchPhase_2ndHalf:
      return "second half";
    case e_MatchPhase_1stExtraTime:
      return "first extra time";
    case e_MatchPhase_2ndExtraTime:
      return "second extra time";
    case e_MatchPhase_Penalties:
      return "penalties";
    default:
      return "next phase";
  }
}

}  // namespace

MatchPhasePage::MatchPhasePage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData),
      pageCreatedTime_ms(EnvironmentManager::GetInstance().GetTime_ms()),
      autoAdvanceTriggered(false) {
  GetGameTask()->GetMatch()->Pause(true);

  nextPhase = (e_MatchPhase)pageData.properties->GetInt("nextphase");

  std::string phaseName;
  if (nextPhase == e_MatchPhase_2ndHalf)
    phaseName = Localization::GetInstance().Translate("phase_2nd_half");
  else if (nextPhase == e_MatchPhase_1stExtraTime)
    phaseName = Localization::GetInstance().Translate("phase_1st_extra_time");
  else if (nextPhase == e_MatchPhase_2ndExtraTime)
    phaseName = Localization::GetInstance().Translate("phase_2nd_extra_time");
  else if (nextPhase == e_MatchPhase_Penalties)
    phaseName = Localization::GetInstance().Translate("phase_penalties");

  std::string phaseLabel = Localization::GetInstance().Translate("phase_begin") + " " + phaseName;

  Gui2Frame* bgPanel = new Gui2Frame(windowManager, "bg_phase", 25, 25, 50, 50, true);
  this->AddView(bgPanel);
  bgPanel->Show();

  Gui2Caption* phaseTitle = new Gui2Caption(
      windowManager, "caption_phase", 2, 2, 46, 3,
      phaseName.empty() ? Localization::GetInstance().Translate("phase_match_phase") : phaseName);
  bgPanel->AddView(phaseTitle);
  phaseTitle->Show();

  buttonNext = new Gui2Button(windowManager, "button_next", 0, 0, 44, 4, phaseLabel);
  Gui2Button* button1 = new Gui2Button(windowManager, "button1", 0, 0, 44, 4,
                                       Localization::GetInstance().Translate("phase_game_plan"));

  buttonNext->sig_OnClick.connect([this](...) { ContinueGame(); });
  button1->sig_OnClick.connect([this](...) { GoGamePlan(); });

  grid = new Gui2Grid(windowManager, "grid", 2, 8, 46, 30);

  grid->AddView(buttonNext, 0, 0);
  grid->AddView(button1, 1, 0);

  grid->UpdateLayout(0.5);

  bgPanel->AddView(grid);
  grid->Show();

  buttonNext->SetFocus();

  this->Show();
}

MatchPhasePage::~MatchPhasePage() {}

void MatchPhasePage::Process() {
  Gui2Page::Process();

  if (!autoAdvanceTriggered && MenuSmokeFullMatchEnabled() &&
      EnvironmentManager::GetInstance().GetTime_ms() >=
          pageCreatedTime_ms + kMenuSmokeAdvanceDelay_ms) {
    autoAdvanceTriggered = true;
    printf("[menu-smoke] Continuing %s automatically\n", PhaseName(nextPhase));
    ContinueGame();
  }
}

void MatchPhasePage::GoGamePlan() {
  Properties properties;
  // properties.SetInt("teamID", );
  CreatePage(e_PageID_GamePlan, properties);
}

void MatchPhasePage::ContinueGame() {
  GetMenuTask()->ReleaseAllButtons();
  GetGameTask()->GetMatch()->Pause(false);
  GoBack();  // back to gamepage
}

void MatchPhasePage::ProcessWindowingEvent(WindowingEvent* event) {
  if (event->IsEscape()) {
    ContinueGame();
    event->Ignore();
  } else {
    Gui2Page::ProcessWindowingEvent(event);
  }
}
