#pragma once

#include <string>
#include <vector>
#include "../../data/careerdata.hpp"
#include "utils/gui2/page.hpp"
#include "utils/gui2/widgets/button.hpp"
#include "utils/gui2/widgets/caption.hpp"
#include "utils/gui2/widgets/grid.hpp"
#include "utils/gui2/widgets/pulldown.hpp"
#include "utils/gui2/widgets/editline.hpp"
#include "utils/gui2/widgets/frame.hpp"
#include "utils/gui2/windowmanager.hpp"

using namespace blunted;

// Mode selection: myCoach / myGM / Player / Manager / Owner Career
class CareerMenuPage : public Gui2Page {
public:
  CareerMenuPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~CareerMenuPage();

protected:
  void GoMyCoach();
  void GoMyGM();
  void GoPlayerCareer();
  void GoManagerCareer();
  void GoOwnerCareer();
  void GoContinueCareer();

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
  void RefreshTeamSelect();

private:
  std::string m_mode;
  Gui2Pulldown* teamSelectPulldown;
  Gui2EditLine* managerNameInput;
  std::string m_selectedTeamID;
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
  void GoFreeAgency();
  void GoTraining();
  void GoStrategy();
  void GoYouthAcademy();
  void GoSeason();
  void GoMatchday();
};

// Transfer market browser
class CareerTransferMarketPage : public Gui2Page {
public:
  CareerTransferMarketPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~CareerTransferMarketPage();
};

class CareerTransferBidsPage : public Gui2Page {
public:
  CareerTransferBidsPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~CareerTransferBidsPage();

protected:
  void NegotiateBid(const std::string& playerName);
};

class CareerTransferBidDetailPage : public Gui2Page {
public:
  CareerTransferBidDetailPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~CareerTransferBidDetailPage();

protected:
  void PlaceBidForPlayer(long long amount);
  std::string m_playerName;
  long long m_askingPrice;
  long long m_playerWage;
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

// Recruiting / Free Agency
class CareerFreeAgencyPage : public Gui2Page {
public:
  CareerFreeAgencyPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~CareerFreeAgencyPage();

protected:
  void RecruitPlayer(const std::string& playerName);
  std::vector<Gui2Button*> playerButtons;
};

// Squad Training
class CareerTrainingPage : public Gui2Page {
public:
  CareerTrainingPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~CareerTrainingPage();

protected:
  void TrainSquad();
  void TrainFocus(const std::string& focusArea);
};

// Youth Academy
class CareerYouthAcademyPage : public Gui2Page {
public:
  CareerYouthAcademyPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~CareerYouthAcademyPage();

protected:
  void ScoutPlayer();
  void PromotePlayer(const std::string& playerName);
};

// Strategy / Tactics
class CareerStrategyPage : public Gui2Page {
public:
  CareerStrategyPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~CareerStrategyPage();

protected:
  void SetStrategy(const std::string& strategyName);
};

// Full Squad Roster view
class CareerSquadRosterPage : public Gui2Page {
public:
  CareerSquadRosterPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~CareerSquadRosterPage();

protected:
  void ReleasePlayer(const std::string& playerName);
};

// Season End / Advance
class CareerSeasonPage : public Gui2Page {
public:
  CareerSeasonPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~CareerSeasonPage();

protected:
  void AdvanceSeason();
  void GoToHub();
};

// Matchday Simulation
class CareerMatchdayPage : public Gui2Page {
public:
  CareerMatchdayPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~CareerMatchdayPage();
  virtual void Process();

protected:
  void BuildFixtures();
  void PopulateGrid();
  void UpdateSummary();
  void SimulateMatch(int fixtureIndex);
  void SimulateAll();
  void PlayMatch();
  void PlayMatchFixture(int fixtureIndex);
  void Process3DMatchResult(int homeGoals, int awayGoals);
  void GoBack();

  Gui2Frame* frame;
  Gui2Grid* fixtureGrid;
  Gui2Caption* summaryCaption;
  std::vector<Gui2Caption*> fixtureScoreCaps;
  std::vector<std::string> m_opponents;
  std::vector<SimulatedMatch> m_results;
  int m_week;
  int m_matchesPlayed;
  int m_wins;
  int m_draws;
  int m_losses;
  int m_goalsFor;
  int m_goalsAgainst;
};

// ---------------------------------------------------------------------------
// Owner Mode Pages have been moved to ownerpages.hpp
// ---------------------------------------------------------------------------
