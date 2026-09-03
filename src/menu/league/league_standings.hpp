#ifndef _HPP_MENU_LEAGUE_STANDINGS
#define _HPP_MENU_LEAGUE_STANDINGS

#include "utils/gui2/page.hpp"
#include "utils/gui2/widgets/button.hpp"
#include "utils/gui2/widgets/caption.hpp"
#include "utils/gui2/widgets/frame.hpp"
#include "utils/gui2/widgets/grid.hpp"
#include "utils/gui2/windowmanager.hpp"

#include "../pagefactory.hpp"

using namespace blunted;

class LeagueStandingsPage : public Gui2Page {
public:
  LeagueStandingsPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~LeagueStandingsPage();
  virtual void Process();

protected:
  void GoPage(e_PageID pageID);
  unsigned long pageCreatedTime_ms;
  bool autoAdvanceTriggered;
};

class LeagueStandingsLeagueTablePage : public Gui2Page {
public:
  LeagueStandingsLeagueTablePage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~LeagueStandingsLeagueTablePage();
  virtual void Process();

protected:
  unsigned long pageCreatedTime_ms;
  bool autoAdvanceTriggered;
};

class LeagueStandingsLeagueStatsPage : public Gui2Page {
public:
  LeagueStandingsLeagueStatsPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~LeagueStandingsLeagueStatsPage();

protected:
};

#endif
