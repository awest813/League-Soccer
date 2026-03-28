#pragma once

#include "utils/gui2/page.hpp"
#include "utils/gui2/widgets/button.hpp"
#include "utils/gui2/widgets/caption.hpp"
#include "utils/gui2/widgets/grid.hpp"
#include "utils/gui2/windowmanager.hpp"

using namespace blunted;

// Mode selection: myCoach / myGM / Player Career / Manager Career
class CareerMenuPage : public Gui2Page {
 public:
  CareerMenuPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~CareerMenuPage();

 protected:
  void GoMyCoach();
  void GoMyGM();
  void GoPlayerCareer();
  void GoManagerCareer();
};

// New career setup: pick team and difficulty
class CareerNewGamePage : public Gui2Page {
 public:
  CareerNewGamePage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~CareerNewGamePage();

 protected:
  void StartCareer();
};

// Career hub: season info, budget, objectives
class CareerHubPage : public Gui2Page {
 public:
  CareerHubPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~CareerHubPage();

 protected:
  void GoTransferMarket();
  void GoSquad();
};

// Transfer market browser
class CareerTransferMarketPage : public Gui2Page {
 public:
  CareerTransferMarketPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~CareerTransferMarketPage();
};
