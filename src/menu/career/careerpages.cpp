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

  Gui2Button* btnCoach =
      new Gui2Button(windowManager, "btn_mycoach", 0, 0, 40, 3, "myCoach");
  Gui2Button* btnGM =
      new Gui2Button(windowManager, "btn_mygm", 0, 0, 40, 3, "myGM");
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

void CareerMenuPage::GoMyCoach() { CreatePage(e_PageID_CareerNewGame); }
void CareerMenuPage::GoMyGM() { CreatePage(e_PageID_CareerNewGame); }
void CareerMenuPage::GoPlayerCareer() { CreatePage(e_PageID_CareerNewGame); }
void CareerMenuPage::GoManagerCareer() { CreatePage(e_PageID_CareerNewGame); }

// ---------------------------------------------------------------------------
// CareerNewGamePage
// ---------------------------------------------------------------------------

CareerNewGamePage::CareerNewGamePage(Gui2WindowManager* windowManager,
                                     const Gui2PageData& pageData)
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

void CareerNewGamePage::StartCareer() { CreatePage(e_PageID_CareerHub); }

// ---------------------------------------------------------------------------
// CareerHubPage
// ---------------------------------------------------------------------------

CareerHubPage::CareerHubPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_careerhub", 20, 20, 60, 3, "Career Hub");
  this->AddView(title);
  title->Show();

  Gui2Button* btnTransfers =
      new Gui2Button(windowManager, "btn_transfers", 0, 0, 40, 3, "Transfer Market");
  Gui2Button* btnSquad =
      new Gui2Button(windowManager, "btn_squad", 0, 0, 40, 3, "My Squad");

  btnTransfers->sig_OnClick.connect([this](...) { GoTransferMarket(); });
  btnSquad->sig_OnClick.connect([this](...) { GoSquad(); });

  Gui2Grid* grid = new Gui2Grid(windowManager, "hub_grid", 20, 26, 60, 60);
  grid->AddView(btnTransfers, 0, 0);
  grid->AddView(btnSquad, 1, 0);
  grid->UpdateLayout(0.5);

  this->AddView(grid);
  grid->Show();

  btnTransfers->SetFocus();
  this->Show();
}

CareerHubPage::~CareerHubPage() {}

void CareerHubPage::GoTransferMarket() { CreatePage(e_PageID_CareerTransferMarket); }
void CareerHubPage::GoSquad() {}

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
