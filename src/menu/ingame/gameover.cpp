// written by bastiaan konings schuiling 2008 - 2015
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not
// be used for anything important. i do not offer support, so don't ask. to be used for inspiration
// :)

#include "gameover.hpp"

#include <ctime>

#include "../../data/matchhistory.hpp"
#include "../career/career_database.hpp"
#include "../pagefactory.hpp"
#include "main.hpp"
#include "utils/gui2/events.hpp"
#include "utils/localization.hpp"

using namespace blunted;

namespace {

constexpr unsigned long kMenuSmokeQuitDelay_ms = 1000;

bool MenuSmokeFullMatchEnabled() {
  return GetConfiguration()->GetBool("menu_smoke_test_full_match", false);
}

}  // namespace

GameOverPage::GameOverPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData),
      pageCreatedTime_ms(EnvironmentManager::GetInstance().GetTime_ms()),
      autoQuitTriggered(false),
      match(nullptr) {
  match = GetGameTask()->GetMatch();
  if (!match) {
    return;
  }
  match->Pause(true);

  Gui2Frame* frame = new Gui2Frame(windowManager, "gameover_frame", 10, 10, 80, 80, true);
  this->AddView(frame);
  frame->Show();

  std::string scoreStr = match->GetTeam(0)->GetTeamData()->GetName() + " " +
                         int_to_str(match->GetMatchData()->GetGoalCount(0)) + " - " +
                         int_to_str(match->GetMatchData()->GetGoalCount(1)) + " " +
                         match->GetTeam(1)->GetTeamData()->GetName();
  Gui2Caption* header =
      new Gui2Caption(windowManager, "caption_gameover_header", 0, 15, 80, 4, scoreStr);
  header->SetPosition(50 - header->GetTextWidthPercent() / 2, 15);
  frame->AddView(header);
  header->Show();

  buttonOkay = new Gui2Button(windowManager, "button_gameover_ok", 55, 82, 20, 3,
                              Localization::GetInstance().Translate("gameover_continue"));
  frame->AddView(buttonOkay);
  buttonOkay->Show();
  buttonOkay->sig_OnClick.connect([this](...) { GoMainMenu(); });

  float possession1 = match->GetMatchData()->GetPossessionTime_ms(0);
  float possession2 = match->GetMatchData()->GetPossessionTime_ms(1);
  int shots1 = match->GetMatchData()->GetShots(0);
  int shots2 = match->GetMatchData()->GetShots(1);
  int shotsOnTarget1 = match->GetMatchData()->GetShotsOnTarget(0);
  int shotsOnTarget2 = match->GetMatchData()->GetShotsOnTarget(1);
  int passes1 = match->GetMatchData()->GetPassAttempts(0);
  int passes2 = match->GetMatchData()->GetPassAttempts(1);
  int passComp1 = match->GetMatchData()->GetPassesCompleted(0);
  int passComp2 = match->GetMatchData()->GetPassesCompleted(1);
  int fouls1 = match->GetMatchData()->GetFouls(0);
  int fouls2 = match->GetMatchData()->GetFouls(1);

  Gui2Grid* grid = new Gui2Grid(windowManager, "grid_gameover_stats", 15, 25, 70, 50);

  float totalPossession = possession1 + possession2;
  int possession1Pct = (totalPossession > 0) ? round(possession1 / totalPossession * 100) : 50;
  int possession2Pct = (totalPossession > 0) ? round(possession2 / totalPossession * 100) : 50;

  grid->AddView(new Gui2Caption(windowManager, "caption_possession_t1", 0, 0, 25, 3,
                                int_to_str(possession1Pct) + "%"),
                0, 0);
  grid->AddView(
      new Gui2Caption(windowManager, "caption_possession_header", 0, 0, 35, 3,
                      Localization::GetInstance().Translate("gameover_possession")),
      0, 1);
  grid->AddView(new Gui2Caption(windowManager, "caption_possession_t2", 0, 0, 10, 3,
                                int_to_str(possession2Pct) + "%"),
                0, 2);

  grid->AddView(new Gui2Caption(windowManager, "caption_shots_t1", 0, 0, 25, 3, int_to_str(shots1)),
                1, 0);
  grid->AddView(new Gui2Caption(windowManager, "caption_shots_header", 0, 0, 35, 3,
                                Localization::GetInstance().Translate("gameover_shots")),
                1, 1);
  grid->AddView(new Gui2Caption(windowManager, "caption_shots_t2", 0, 0, 10, 3, int_to_str(shots2)),
                1, 2);

  auto shotOnTargetStr = [](int onTarget, int total) -> std::string {
    if (total == 0)
      return "0";
    return int_to_str(onTarget) + "/" + int_to_str(total);
  };
  grid->AddView(new Gui2Caption(windowManager, "caption_sot_t1", 0, 0, 25, 3,
                                shotOnTargetStr(shotsOnTarget1, shots1)),
                2, 0);
  grid->AddView(new Gui2Caption(windowManager, "caption_sot_header", 0, 0, 35, 3,
                                Localization::GetInstance().Translate("gameover_shots_on_target")),
                2, 1);
  grid->AddView(new Gui2Caption(windowManager, "caption_sot_t2", 0, 0, 10, 3,
                                shotOnTargetStr(shotsOnTarget2, shots2)),
                2, 2);

  auto passAccStr = [](int completed, int attempted) -> std::string {
    if (attempted == 0)
      return "0%";
    return int_to_str(int(round(completed * 100.0f / attempted))) + "% (" + int_to_str(completed) +
           "/" + int_to_str(attempted) + ")";
  };
  grid->AddView(new Gui2Caption(windowManager, "caption_passacc_t1", 0, 0, 25, 3,
                                passAccStr(passComp1, passes1)),
                3, 0);
  grid->AddView(new Gui2Caption(windowManager, "caption_passacc_header", 0, 0, 35, 3,
                                Localization::GetInstance().Translate("gameover_pass_accuracy")),
                3, 1);
  grid->AddView(new Gui2Caption(windowManager, "caption_passacc_t2", 0, 0, 10, 3,
                                passAccStr(passComp2, passes2)),
                3, 2);

  grid->AddView(new Gui2Caption(windowManager, "caption_fouls_t1", 0, 0, 25, 3, int_to_str(fouls1)),
                4, 0);
  grid->AddView(new Gui2Caption(windowManager, "caption_fouls_header", 0, 0, 35, 3,
                                Localization::GetInstance().Translate("gameover_fouls")),
                4, 1);
  grid->AddView(new Gui2Caption(windowManager, "caption_fouls_t2", 0, 0, 10, 3, int_to_str(fouls2)),
                4, 2);

  grid->UpdateLayout(0.5);

  frame->AddView(grid);
  grid->Show();

  buttonOkay->SetFocus();

  // Auto-save match result to history. Guarded so re-entering this page (e.g.
  // returning from Match History) does not append a duplicate record.
  if (!match->GetMatchData()->IsHistorySaved()) {
    match->GetMatchData()->SetHistorySaved(true);

    float poss1 = match->GetMatchData()->GetPossessionTime_ms(0);
    float poss2 = match->GetMatchData()->GetPossessionTime_ms(1);
    float totalPoss = poss1 + poss2;

    MatchHistoryEntry entry;
    entry.id = 0;

    time_t now = time(nullptr);
    char tsbuf[32];
    struct tm* tminfo = localtime(&now);
    strftime(tsbuf, sizeof(tsbuf), "%Y-%m-%d %H:%M:%S", tminfo);
    entry.timestamp = tsbuf;

    entry.team1_name = match->GetTeam(0)->GetTeamData()->GetName();
    entry.team2_name = match->GetTeam(1)->GetTeamData()->GetName();
    entry.score1 = match->GetMatchData()->GetGoalCount(0);
    entry.score2 = match->GetMatchData()->GetGoalCount(1);
    entry.match_time_ms = (int)match->GetMatchTime_ms();
    entry.possession1_pct = (totalPoss > 0) ? poss1 / totalPoss * 100.0f : 50.0f;
    entry.possession2_pct = (totalPoss > 0) ? poss2 / totalPoss * 100.0f : 50.0f;
    entry.shots1 = match->GetMatchData()->GetShots(0);
    entry.shots2 = match->GetMatchData()->GetShots(1);
    entry.shots_on_target1 = match->GetMatchData()->GetShotsOnTarget(0);
    entry.shots_on_target2 = match->GetMatchData()->GetShotsOnTarget(1);
    entry.passes1 = match->GetMatchData()->GetPassAttempts(0);
    entry.passes2 = match->GetMatchData()->GetPassAttempts(1);
    entry.passes_completed1 = match->GetMatchData()->GetPassesCompleted(0);
    entry.passes_completed2 = match->GetMatchData()->GetPassesCompleted(1);
    entry.fouls1 = match->GetMatchData()->GetFouls(0);
    entry.fouls2 = match->GetMatchData()->GetFouls(1);

    MatchHistory::EnsureTable();
    MatchHistory::SaveMatch(entry);
  }

  Gui2Button* buttonHistory =
      new Gui2Button(windowManager, "button_gameover_history", 25, 82, 20, 3,
                     Localization::GetInstance().Translate("gameover_match_history"));
  frame->AddView(buttonHistory);
  buttonHistory->Show();
  buttonHistory->sig_OnClick.connect([this](...) {
    Properties props;
    CreatePage((int)e_PageID_MatchHistory, props);
  });

  this->Show();
}

GameOverPage::~GameOverPage() {}

void GameOverPage::Process() {
  Gui2Page::Process();

  if (!autoQuitTriggered && MenuSmokeFullMatchEnabled() &&
      EnvironmentManager::GetInstance().GetTime_ms() >=
          pageCreatedTime_ms + kMenuSmokeQuitDelay_ms) {
    autoQuitTriggered = true;
    printf("[menu-smoke] Full match complete: %s %i - %i %s\n",
           match->GetTeam(0)->GetTeamData()->GetName().c_str(),
           match->GetMatchData()->GetGoalCount(0), match->GetMatchData()->GetGoalCount(1),
           match->GetTeam(1)->GetTeamData()->GetName().c_str());
    printf("[menu-smoke] Full-match verification succeeded, quitting test run\n");
    EnvironmentManager::GetInstance().SignalQuit();
  }
}

void GameOverPage::ProcessWindowingEvent(WindowingEvent* event) {
  if (event->IsEscape()) {
    // The match is over; ESC should return to the main menu like "Continue"
    // rather than walking back into the (now finished) match/game flow.
    GoMainMenu();
    return;
  } else {
    event->Ignore();
  }
}

void GameOverPage::GoRematch() {
  windowManager->GetPagePath()->Clear();

  GetGameTask()->Action(e_GameTaskMessage_StopMatch);
  GetGameTask()->Action(e_GameTaskMessage_StartMatch);

  this->Exit();
  Properties properties;
  windowManager->GetPageFactory()->CreatePage((int)e_PageID_Game, properties, 0);
  delete this;
}

void GameOverPage::GoMainMenu() {
  // Preserve the finished 3D match in career bookkeeping before leaving the game flow.
  bool resumeCareer = false;
  if (match && CareerDatabase::GetInstance().GetActiveSave()) {
    auto* matchData = match->GetMatchData();
    if (matchData) {
      CareerDatabase::GetInstance().Process3DMatchResult(matchData->GetGoalCount(0),
                                                         matchData->GetGoalCount(1));
      CareerDatabase::GetInstance().SaveCareerData();
      resumeCareer = true;
    }
  }
  if (resumeCareer) {
    GetConfiguration()->SetBool("career_resume_hub", true);
  }
  this->Exit();
  GetMenuTask()->SetMenuAction(e_MenuAction_Menu);
  delete this;
}
