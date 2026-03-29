#include "careerpages.hpp"

#include "../../main.hpp"
#include "../pagefactory.hpp"

// ---------------------------------------------------------------------------
// CareerMenuPage
// ---------------------------------------------------------------------------

CareerMenuPage::CareerMenuPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_career", 20, 20, 60, 3, "Career Mode");
  this->AddView(title);
  title->Show();

  Gui2Button* btnCoach = new Gui2Button(windowManager, "btn_mycoach", 0, 0, 40, 3, "myCoach");
  Gui2Button* btnGM = new Gui2Button(windowManager, "btn_mygm", 0, 0, 40, 3, "myGM");
  Gui2Button* btnPlayer =
      new Gui2Button(windowManager, "btn_playercareer", 0, 0, 40, 3, "Player Career");
  Gui2Button* btnManager =
      new Gui2Button(windowManager, "btn_managercareer", 0, 0, 40, 3, "Manager Career");

  btnCoach->sig_OnClick.connect([this](...) { GoMyCoach(); });
  btnGM->sig_OnClick.connect([this](...) { GoMyGM(); });
  btnPlayer->sig_OnClick.connect([this](...) { GoPlayerCareer(); });
  btnManager->sig_OnClick.connect([this](...) { GoManagerCareer(); });

  Gui2Grid* grid = new Gui2Grid(windowManager, "career_grid", 20, 26, 60, 60);
  grid->AddView(btnCoach, 0, 0);
  grid->AddView(btnGM, 1, 0);
  grid->AddView(btnPlayer, 2, 0);
  grid->AddView(btnManager, 3, 0);
  grid->UpdateLayout(0.5);

  this->AddView(grid);
  grid->Show();

  btnCoach->SetFocus();
  this->Show();
}

CareerMenuPage::~CareerMenuPage() {}

void CareerMenuPage::GoMyCoach() {
  CreatePage(e_PageID_CareerNewGame);
}
void CareerMenuPage::GoMyGM() {
  CreatePage(e_PageID_CareerNewGame);
}
void CareerMenuPage::GoPlayerCareer() {
  CreatePage(e_PageID_CareerNewGame);
}
void CareerMenuPage::GoManagerCareer() {
  CreatePage(e_PageID_CareerNewGame);
}

// ---------------------------------------------------------------------------
// CareerNewGamePage
// ---------------------------------------------------------------------------

CareerNewGamePage::CareerNewGamePage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_newgame", 20, 20, 60, 3, "New Career");
  this->AddView(title);
  title->Show();

  Gui2Button* btnStart =
      new Gui2Button(windowManager, "btn_start_career", 30, 50, 40, 3, "Start Career");
  btnStart->sig_OnClick.connect([this](...) { StartCareer(); });
  this->AddView(btnStart);
  btnStart->Show();
  btnStart->SetFocus();

  this->Show();
}

CareerNewGamePage::~CareerNewGamePage() {}

void CareerNewGamePage::StartCareer() {
  CreatePage(e_PageID_CareerHub);
}

// ---------------------------------------------------------------------------
// CareerHubPage
// ---------------------------------------------------------------------------

CareerHubPage::CareerHubPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_careerhub", 20, 5, 60, 3, "Career Hub");
  this->AddView(title);
  title->Show();

  Gui2Button* btnTransfers =
      new Gui2Button(windowManager, "btn_transfers", 0, 0, 60, 3, "Transfer Market");
  Gui2Button* btnSquad = new Gui2Button(windowManager, "btn_squad", 0, 0, 60, 3, "My Squad");
  Gui2Button* btnPressConf =
      new Gui2Button(windowManager, "btn_pressconf", 0, 0, 60, 3, "Press Conference");
  Gui2Button* btnLeagueExp =
      new Gui2Button(windowManager, "btn_leagueexp", 0, 0, 60, 3, "League Expansion / Relegation");
  Gui2Button* btnCustomLeague =
      new Gui2Button(windowManager, "btn_customleague", 0, 0, 60, 3, "Custom League");

  btnTransfers->sig_OnClick.connect([this](...) { GoTransferMarket(); });
  btnSquad->sig_OnClick.connect([this](...) { GoSquad(); });
  btnPressConf->sig_OnClick.connect([this](...) { GoPressConference(); });
  btnLeagueExp->sig_OnClick.connect([this](...) { GoLeagueExpansion(); });
  btnCustomLeague->sig_OnClick.connect([this](...) { GoCustomLeague(); });

  Gui2Grid* grid = new Gui2Grid(windowManager, "hub_grid", 20, 12, 60, 80);
  grid->AddView(btnTransfers, 0, 0);
  grid->AddView(btnSquad, 1, 0);
  grid->AddView(btnPressConf, 2, 0);
  grid->AddView(btnLeagueExp, 3, 0);
  grid->AddView(btnCustomLeague, 4, 0);
  grid->UpdateLayout(0.5);

  this->AddView(grid);
  grid->Show();

  btnTransfers->SetFocus();
  this->Show();
}

CareerHubPage::~CareerHubPage() {}

void CareerHubPage::GoTransferMarket() {
  CreatePage(e_PageID_CareerTransferMarket);
}
void CareerHubPage::GoSquad() {}
void CareerHubPage::GoPressConference() {
  CreatePage(e_PageID_CareerPressConference);
}
void CareerHubPage::GoLeagueExpansion() {
  CreatePage(e_PageID_CareerLeagueExpansion);
}
void CareerHubPage::GoCustomLeague() {
  CreatePage(e_PageID_CareerCustomLeague);
}

// ---------------------------------------------------------------------------
// CareerTransferMarketPage
// ---------------------------------------------------------------------------

CareerTransferMarketPage::CareerTransferMarketPage(Gui2WindowManager* windowManager,
                                                   const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_transfers", 20, 20, 60, 3, "Transfer Market");
  this->AddView(title);
  title->Show();

  this->Show();
}

CareerTransferMarketPage::~CareerTransferMarketPage() {}

// ---------------------------------------------------------------------------
// CareerPressConferencePage  (6.13)
// ---------------------------------------------------------------------------

CareerPressConferencePage::CareerPressConferencePage(Gui2WindowManager* windowManager,
                                                     const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_pressconf", 10, 5, 80, 3, "Press Conference");
  this->AddView(title);
  title->Show();

  Gui2Caption* question =
      new Gui2Caption(windowManager, "caption_pc_question", 10, 12, 80, 5,
                      "How do you feel about the team's performance this week?");
  this->AddView(question);
  question->Show();

  // Answer buttons: positive / neutral / negative
  Gui2Button* btnPositive =
      new Gui2Button(windowManager, "btn_pc_positive", 0, 0, 76, 3,
                     "We gave everything – the fans should be proud. (+reputation)");
  Gui2Button* btnNeutral = new Gui2Button(windowManager, "btn_pc_neutral", 0, 0, 76, 3,
                                          "It was a decent performance; we move on.");
  Gui2Button* btnNegative = new Gui2Button(windowManager, "btn_pc_negative", 0, 0, 76, 3,
                                           "I'm disappointed. We must do better. (-reputation)");

  btnPositive->sig_OnClick.connect([this](...) { SelectAnswer(0); });
  btnNeutral->sig_OnClick.connect([this](...) { SelectAnswer(1); });
  btnNegative->sig_OnClick.connect([this](...) { SelectAnswer(2); });

  Gui2Grid* grid = new Gui2Grid(windowManager, "pc_grid", 12, 22, 76, 50);
  grid->AddView(btnPositive, 0, 0);
  grid->AddView(btnNeutral, 1, 0);
  grid->AddView(btnNegative, 2, 0);
  grid->UpdateLayout(0.5);

  this->AddView(grid);
  grid->Show();

  btnPositive->SetFocus();
  this->Show();
}

CareerPressConferencePage::~CareerPressConferencePage() {}

void CareerPressConferencePage::SelectAnswer(int answerIndex) {
  // TODO: wire into CareerDatabase::ApplyReputationDelta() once the active
  // CareerSave is accessible from menu pages.
  (void)m_reputationDeltas[answerIndex];
  CreatePage(e_PageID_CareerHub);
}

// ---------------------------------------------------------------------------
// CareerLeagueExpansionPage  (6.16)
// ---------------------------------------------------------------------------

CareerLeagueExpansionPage::CareerLeagueExpansionPage(Gui2WindowManager* windowManager,
                                                     const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Caption* title = new Gui2Caption(windowManager, "caption_leagueexp", 10, 5, 80, 3,
                                       "League Expansion & Relegation");
  this->AddView(title);
  title->Show();

  Gui2Caption* info = new Gui2Caption(windowManager, "caption_leagueexp_info", 10, 12, 80, 3,
                                      "Configure promotion and relegation for your league.");
  this->AddView(info);
  info->Show();

  Gui2Button* btnEnable = new Gui2Button(windowManager, "btn_leagueexp_enable", 0, 0, 60, 3,
                                         "Enable Promotion / Relegation");
  Gui2Button* btnDisable = new Gui2Button(windowManager, "btn_leagueexp_disable", 0, 0, 60, 3,
                                          "Disable Promotion / Relegation");
  Gui2Button* btnAddDiv =
      new Gui2Button(windowManager, "btn_leagueexp_adddiv", 0, 0, 60, 3, "Add Division");

  btnEnable->sig_OnClick.connect([this](...) { EnableRelegation(); });
  btnDisable->sig_OnClick.connect([this](...) { DisableRelegation(); });
  btnAddDiv->sig_OnClick.connect([this](...) { AddDivision(); });

  Gui2Grid* grid = new Gui2Grid(windowManager, "leagueexp_grid", 20, 20, 60, 60);
  grid->AddView(btnEnable, 0, 0);
  grid->AddView(btnDisable, 1, 0);
  grid->AddView(btnAddDiv, 2, 0);
  grid->UpdateLayout(0.5);

  this->AddView(grid);
  grid->Show();

  btnEnable->SetFocus();
  this->Show();
}

CareerLeagueExpansionPage::~CareerLeagueExpansionPage() {}

void CareerLeagueExpansionPage::EnableRelegation() {
  // Future: persist setting via CareerDatabase
  CreatePage(e_PageID_CareerHub);
}

void CareerLeagueExpansionPage::DisableRelegation() {
  // Future: persist setting via CareerDatabase
  CreatePage(e_PageID_CareerHub);
}

void CareerLeagueExpansionPage::AddDivision() {
  // Future: open a sub-page to configure the new division
  CreatePage(e_PageID_CareerHub);
}

// ---------------------------------------------------------------------------
// CareerCustomLeaguePage  (6.17)
// ---------------------------------------------------------------------------

CareerCustomLeaguePage::CareerCustomLeaguePage(Gui2WindowManager* windowManager,
                                               const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Caption* title = new Gui2Caption(windowManager, "caption_customleague", 10, 5, 80, 3,
                                       "Custom League Creation");
  this->AddView(title);
  title->Show();

  Gui2Caption* info =
      new Gui2Caption(windowManager, "caption_customleague_info", 10, 12, 80, 3,
                      "Design your own league: name, divisions, teams and cup competition.");
  this->AddView(info);
  info->Show();

  Gui2Button* btnCreate = new Gui2Button(windowManager, "btn_customleague_create", 30, 50, 40, 3,
                                         "Create Custom League");
  btnCreate->sig_OnClick.connect([this](...) { CreateCustomLeague(); });
  this->AddView(btnCreate);
  btnCreate->Show();
  btnCreate->SetFocus();

  this->Show();
}

CareerCustomLeaguePage::~CareerCustomLeaguePage() {}

void CareerCustomLeaguePage::CreateCustomLeague() {
  // Future: collect name/divisions from user input widgets and persist via CareerDatabase
  CreatePage(e_PageID_CareerHub);
}
