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

private:
  void GoCareerMode(const std::string& mode);
};

// New career setup: pick team and difficulty
class CareerNewGamePage : public Gui2Page {
public:
  CareerNewGamePage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~CareerNewGamePage();

protected:
  void StartCareer();

private:
  std::string m_mode;  // career mode passed from CareerMenuPage via properties
};

// Career hub: season info, budget, objectives
class CareerHubPage : public Gui2Page {
public:
  CareerHubPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~CareerHubPage();

protected:
  void GoTransferMarket();
  void GoSquad();
  void GoPressConference();
  void GoLeagueExpansion();
  void GoCustomLeague();
};

// Transfer market browser
class CareerTransferMarketPage : public Gui2Page {
public:
  CareerTransferMarketPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~CareerTransferMarketPage();
};

// 6.13 – Press conference / media interactions
class CareerPressConferencePage : public Gui2Page {
public:
  CareerPressConferencePage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~CareerPressConferencePage();

protected:
  // Called when the user selects an answer (index 0-2)
  void SelectAnswer(int answerIndex);

  // Reputation delta for each answer of the current question
  int m_reputationDeltas[3] = {5, 0, -5};
};

// 6.16 – League expansion / relegation configuration
class CareerLeagueExpansionPage : public Gui2Page {
public:
  CareerLeagueExpansionPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~CareerLeagueExpansionPage();

protected:
  void EnableRelegation();
  void DisableRelegation();
  void AddDivision();
};

// 6.17 – Custom league creation
class CareerCustomLeaguePage : public Gui2Page {
public:
  CareerCustomLeaguePage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~CareerCustomLeaguePage();

protected:
  void CreateCustomLeague();
};
