#include "league_prematch.hpp"

#include "../../main.hpp"
#include "../pagefactory.hpp"
#include "menu_smoke.hpp"
#include "utils/localization.hpp"

LeaguePreMatchPage::LeaguePreMatchPage(Gui2WindowManager* windowManager,
                                       const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData),
      hasFixture(false),
      userIsHome(true),
      pageCreatedTime_ms(league_menu_smoke::Now_ms()),
      autoAdvanceTriggered(false) {
  auto& loc = Localization::GetInstance();
  int userTeamID = 0;
  LeagueGetUserTeamID(userTeamID);
  hasFixture = LeagueGetUserNextFixture(fixture);
  if (hasFixture) {
    userIsHome = (fixture.homeTeamID == userTeamID);
  }

  Gui2Frame* frame = new Gui2Frame(windowManager, "frame_league_prematch", 20, 15, 60, 70, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_league_prematch", 2, 2, 56, 3,
                      loc.Translate("league_prematch_title"));
  frame->AddView(title);
  title->Show();

  if (hasFixture) {
    Gui2Caption* matchInfo =
        new Gui2Caption(windowManager, "caption_league_prematch_info", 2, 7, 56, 2.5,
                        fixture.date + "  |  " +
                            (fixture.competitionName.empty() ? loc.Translate("league_standings")
                                                             : fixture.competitionName));
    frame->AddView(matchInfo);
    matchInfo->Show();

    // Club crests flanking the pairing.
    const float logoHeight = 10.5f;
    const float logoWidth = windowManager->GetWidthPercentForHeight(logoHeight, 1.0f);
    const char* fallbackLogo = "media/menu/league.png";
    std::string homeLogo = LeagueResolveLogoPath(fixture.homeTeamID);
    std::string awayLogo = LeagueResolveLogoPath(fixture.awayTeamID);

    Gui2Image* homeLogoView =
        new Gui2Image(windowManager, "image_league_prematch_home", 15.0f - logoWidth * 0.5f, 11,
                      logoWidth, logoHeight);
    frame->AddView(homeLogoView);
    homeLogoView->LoadImage(homeLogo.empty() ? fallbackLogo : homeLogo);
    homeLogoView->Show();

    Gui2Image* awayLogoView =
        new Gui2Image(windowManager, "image_league_prematch_away", 45.0f - logoWidth * 0.5f, 11,
                      logoWidth, logoHeight);
    frame->AddView(awayLogoView);
    awayLogoView->LoadImage(awayLogo.empty() ? fallbackLogo : awayLogo);
    awayLogoView->Show();

    std::string homeLabel = fixture.homeTeamName;
    std::string awayLabel = fixture.awayTeamName;
    if (userIsHome) {
      homeLabel = "> " + homeLabel;
    } else {
      awayLabel = "> " + awayLabel;
    }
    Gui2Caption* pairing = new Gui2Caption(windowManager, "caption_league_prematch_pairing", 2, 23,
                                           56, 3, homeLabel + "  vs  " + awayLabel);
    frame->AddView(pairing);
    pairing->Show();

    Gui2Caption* venue = new Gui2Caption(
        windowManager, "caption_league_prematch_venue", 2, 27, 56, 2.5,
        userIsHome ? loc.Translate("league_prematch_home")
                   : loc.TranslateAndFormat("league_prematch_away", {fixture.homeTeamName}));
    frame->AddView(venue);
    venue->Show();
  } else {
    Gui2Caption* noFixture = new Gui2Caption(
        windowManager, "caption_league_prematch_nofixture", 2, 9, 56, 3,
        loc.Translate("league_prematch_no_fixture"));
    frame->AddView(noFixture);
    noFixture->Show();
  }

  Gui2Grid* grid = new Gui2Grid(windowManager, "grid_league_prematch", 2, 33, 56, 24);

  int row = 0;
  if (hasFixture) {
    Gui2Button* buttonKickOff =
        new Gui2Button(windowManager, "button_league_prematch_kickoff", 0, 0, 52, 4,
                       loc.Translate("league_prematch_kickoff"));
    buttonKickOff->sig_OnClick.connect([this](...) { KickOff(); });
    grid->AddView(buttonKickOff, row++, 0);

    Gui2Button* buttonSimulate =
        new Gui2Button(windowManager, "button_league_prematch_simulate", 0, 0, 52, 4,
                       loc.Translate("league_prematch_simulate"));
    buttonSimulate->sig_OnClick.connect([this](...) { Simulate(); });
    grid->AddView(buttonSimulate, row++, 0);
  }

  Gui2Button* buttonBack =
      new Gui2Button(windowManager, "button_league_prematch_back", 0, 0, 52, 4,
                     loc.Translate("action_back"));
  buttonBack->sig_OnClick.connect([this](...) { GoBackToDashboard(); });
  grid->AddView(buttonBack, row, 0);

  grid->UpdateLayout(0.25);
  frame->AddView(grid);
  grid->Show();

  if (grid->IsSelectable()) {
    grid->SetFocus();
  }

  this->Show();
}

LeaguePreMatchPage::~LeaguePreMatchPage() {}

void LeaguePreMatchPage::Process() {
  Gui2Page::Process();

  if (!league_menu_smoke::RouteEnabled("prematch") || autoAdvanceTriggered ||
      league_menu_smoke::Now_ms() < pageCreatedTime_ms + league_menu_smoke::kQuitDelay_ms) {
    return;
  }

  autoAdvanceTriggered = true;
  printf("[menu-smoke] League pre-match page reached successfully\n");
  GetMenuTask()->QuitGame();
}

void LeaguePreMatchPage::KickOff() {
  if (!hasFixture) {
    return;
  }

  // Arm the pending fixture before leaving the menu flow; GameOverPage picks
  // it up after full-time and writes the score back into match_results.
  LeagueSetPendingFixture(fixture);

  std::vector<SideSelection> sides(1);
  sides[0].controllerID = 0;
  // side -1 controls team 1 (home), side 1 controls team 2 (away).
  sides[0].side = userIsHome ? -1 : 1;
  GetMenuTask()->SetControllerSetup(sides);
  GetMenuTask()->SetTeamIDs(int_to_str(fixture.homeTeamID), int_to_str(fixture.awayTeamID));

  CreatePage((int)e_PageID_MatchOptions);
}

void LeaguePreMatchPage::Simulate() {
  if (!hasFixture) {
    return;
  }

  int homeGoals = 0;
  int awayGoals = 0;
  LeagueSimulateFixture(fixture, homeGoals, awayGoals);
  LeagueRecordResult(fixture, homeGoals, awayGoals);
  LeagueResolveMatchday(fixture.date);

  CreatePage((int)e_PageID_League_Matchday);
}

void LeaguePreMatchPage::GoBackToDashboard() {
  this->Exit();
  Properties properties;
  windowManager->GetPageFactory()->CreatePage((int)e_PageID_League_Forward, properties, 0);
  delete this;
}
