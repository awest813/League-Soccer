#ifndef _HPP_MENU_LEAGUE_TEAM
#define _HPP_MENU_LEAGUE_TEAM

#include "utils/gui2/page.hpp"
#include "utils/gui2/widgets/button.hpp"
#include "utils/gui2/widgets/caption.hpp"
#include "utils/gui2/widgets/dialog.hpp"
#include "utils/gui2/widgets/frame.hpp"
#include "utils/gui2/widgets/grid.hpp"
#include "utils/gui2/widgets/slider.hpp"
#include "utils/gui2/widgets/text.hpp"
#include "utils/gui2/windowmanager.hpp"

#include "../pagefactory.hpp"

using namespace blunted;

class LeagueTeamPage : public Gui2Page {
public:
  LeagueTeamPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~LeagueTeamPage();
  virtual void Process();

protected:
  void GoPage(e_PageID pageID);
  unsigned long pageCreatedTime_ms;
  bool autoAdvanceTriggered;
};

class LeagueTeamFormationPage : public Gui2Page {
public:
  LeagueTeamFormationPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~LeagueTeamFormationPage();

protected:
  void ApplyFormation(const std::string& formationXML);

  Gui2Frame* frame;
};

class LeagueTeamPlayerSelectionPage : public Gui2Page {
public:
  LeagueTeamPlayerSelectionPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~LeagueTeamPlayerSelectionPage();

protected:
  void RefreshSquad();
  void SwapPlayers(int idA, int idB);

  Gui2Frame* frame;
  Gui2Grid* squadGrid;
  Gui2Caption* feedbackCaption;
  int selectedPlayerDBID;
  bool selectedIsGoalkeeper;
};

class LeagueTeamTacticsPage : public Gui2Page {
public:
  LeagueTeamTacticsPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~LeagueTeamTacticsPage();

protected:
  void SaveTactics();

  Gui2Frame* frame;
  Gui2Caption* feedbackCaption;
  std::vector<std::pair<std::string, Gui2Slider*>> tacticSliders;
};

class LeagueTeamPlayerOverviewPage : public Gui2Page {
public:
  LeagueTeamPlayerOverviewPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~LeagueTeamPlayerOverviewPage();
  virtual void Process();

protected:
  unsigned long pageCreatedTime_ms;
  bool autoAdvanceTriggered;
};

class LeagueTeamPlayerDevelopmentPage : public Gui2Page {
public:
  LeagueTeamPlayerDevelopmentPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~LeagueTeamPlayerDevelopmentPage();

protected:
  void RefreshSquad();
  void TrainSelected(const std::string& attributeName, const std::string& attributeLabel);

  Gui2Frame* frame;
  Gui2Grid* squadGrid;
  Gui2Caption* feedbackCaption;
  int selectedPlayerDBID;
};

class LeagueTeamSetupPage : public Gui2Page {
public:
  LeagueTeamSetupPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~LeagueTeamSetupPage();

protected:
};

#endif
