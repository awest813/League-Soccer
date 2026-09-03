#ifndef _HPP_MENU_LEAGUE_MATCHDAY
#define _HPP_MENU_LEAGUE_MATCHDAY

#include "utils/gui2/page.hpp"
#include "utils/gui2/widgets/button.hpp"
#include "utils/gui2/widgets/caption.hpp"
#include "utils/gui2/widgets/frame.hpp"
#include "utils/gui2/widgets/grid.hpp"
#include "utils/gui2/windowmanager.hpp"

#include "../pagefactory.hpp"

using namespace blunted;

// Lists every result of the most recently resolved matchday, highlighting the
// user's club. Reached after simulating/playing a matchday.
class LeagueMatchdayPage : public Gui2Page {
public:
  LeagueMatchdayPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~LeagueMatchdayPage();
  virtual void Process();

protected:
  void RefreshResults();
  void StartNewSeason();
  void GoBackToDashboard();

  Gui2Frame* frame;
  Gui2Caption* matchdayHeader;
  Gui2Grid* resultsGrid;
  unsigned long pageCreatedTime_ms;
  bool autoAdvanceTriggered;
};

#endif
