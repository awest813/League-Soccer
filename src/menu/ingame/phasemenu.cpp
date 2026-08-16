// written by bastiaan konings schuiling 2008 - 2015
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not
// be used for anything important. i do not offer support, so don't ask. to be used for inspiration
// :)

#include "phasemenu.hpp"

#include "../gameplan.hpp"
#include "../pagefactory.hpp"
#include "main.hpp"
#include "utils/localization.hpp"
#include "../../onthepitch/match.hpp"

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

  // Draw Match Stats
  Match* match = GetGameTask()->GetMatch();
  if (match) {
    MatchData* md = match->GetMatchData();
    Gui2Grid* statsGrid = new Gui2Grid(windowManager, "stats_grid", 2, 7, 46, 35);
    bgPanel->AddView(statsGrid);

    // Header (Team Names)
    Gui2Caption* h0 = new Gui2Caption(windowManager, "s_t1", 0, 0, 15, 3, md->GetTeamData(0)->GetShortName());
    Gui2Caption* h1 = new Gui2Caption(windowManager, "s_l1", 0, 0, 16, 3, "MATCH STATS");
    Gui2Caption* h2 = new Gui2Caption(windowManager, "s_t2", 0, 0, 15, 3, md->GetTeamData(1)->GetShortName());
    h0->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
    h1->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
    h2->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
    statsGrid->AddView(h0, 0, 0);
    statsGrid->AddView(h1, 1, 0);
    statsGrid->AddView(h2, 2, 0);

    // Score
    statsGrid->AddView(new Gui2Caption(windowManager, "s_v1", 0, 0, 15, 3, int_to_str(md->GetGoalCount(0))), 0, 1);
    statsGrid->AddView(new Gui2Caption(windowManager, "s_l2", 0, 0, 16, 3, "Score"), 1, 1);
    statsGrid->AddView(new Gui2Caption(windowManager, "s_v2", 0, 0, 15, 3, int_to_str(md->GetGoalCount(1))), 2, 1);

    // Possession
    unsigned long p0 = md->GetPossessionTime_ms(0);
    unsigned long p1 = md->GetPossessionTime_ms(1);
    int p0_pct = (p0 + p1 > 0) ? (p0 * 100) / (p0 + p1) : 50;
    statsGrid->AddView(new Gui2Caption(windowManager, "s_p1", 0, 0, 15, 3, int_to_str(p0_pct) + "%"), 0, 2);
    statsGrid->AddView(new Gui2Caption(windowManager, "s_l3", 0, 0, 16, 3, "Possession"), 1, 2);
    statsGrid->AddView(new Gui2Caption(windowManager, "s_p2", 0, 0, 15, 3, int_to_str(100 - p0_pct) + "%"), 2, 2);

    // Shots (On Target)
    std::string sh0 = int_to_str(md->GetShots(0)) + " (" + int_to_str(md->GetShotsOnTarget(0)) + ")";
    std::string sh1 = int_to_str(md->GetShots(1)) + " (" + int_to_str(md->GetShotsOnTarget(1)) + ")";
    statsGrid->AddView(new Gui2Caption(windowManager, "s_sh1", 0, 0, 15, 3, sh0), 0, 3);
    statsGrid->AddView(new Gui2Caption(windowManager, "s_l4", 0, 0, 16, 3, "Shots (On Target)"), 1, 3);
    statsGrid->AddView(new Gui2Caption(windowManager, "s_sh2", 0, 0, 15, 3, sh1), 2, 3);

    // Passes (Completed)
    std::string pa0 = int_to_str(md->GetPassesCompleted(0)) + " / " + int_to_str(md->GetPassAttempts(0));
    std::string pa1 = int_to_str(md->GetPassesCompleted(1)) + " / " + int_to_str(md->GetPassAttempts(1));
    statsGrid->AddView(new Gui2Caption(windowManager, "s_pa1", 0, 0, 15, 3, pa0), 0, 4);
    statsGrid->AddView(new Gui2Caption(windowManager, "s_l5", 0, 0, 16, 3, "Passes (Completed)"), 1, 4);
    statsGrid->AddView(new Gui2Caption(windowManager, "s_pa2", 0, 0, 15, 3, pa1), 2, 4);

    // Fouls
    statsGrid->AddView(new Gui2Caption(windowManager, "s_f1", 0, 0, 15, 3, int_to_str(md->GetFouls(0))), 0, 5);
    statsGrid->AddView(new Gui2Caption(windowManager, "s_l6", 0, 0, 16, 3, "Fouls"), 1, 5);
    statsGrid->AddView(new Gui2Caption(windowManager, "s_f2", 0, 0, 15, 3, int_to_str(md->GetFouls(1))), 2, 5);

    statsGrid->UpdateLayout(0.5);
    statsGrid->Show();
  }

  buttonNext = new Gui2Button(windowManager, "button_next", 0, 0, 44, 4, phaseLabel);
  Gui2Button* button1 = new Gui2Button(windowManager, "button1", 0, 0, 44, 4,
                                       Localization::GetInstance().Translate("phase_game_plan"));

  buttonNext->sig_OnClick.connect([this](...) { ContinueGame(); });
  button1->sig_OnClick.connect([this](...) { GoGamePlan(); });

  grid = new Gui2Grid(windowManager, "grid", 2, 44, 46, 4);

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
