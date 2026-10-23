#ifndef _HPP_MENU_LEAGUE_CALENDAR
#define _HPP_MENU_LEAGUE_CALENDAR

#include "../pagefactory.hpp"
#include "utils/gui2/page.hpp"
#include "utils/gui2/widgets/button.hpp"
#include "utils/gui2/widgets/caption.hpp"
#include "utils/gui2/widgets/dialog.hpp"
#include "utils/gui2/widgets/frame.hpp"
#include "utils/gui2/widgets/grid.hpp"
#include "utils/gui2/widgets/pulldown.hpp"
#include "utils/gui2/widgets/table.hpp"
#include "utils/gui2/widgets/text.hpp"
#include "utils/gui2/windowmanager.hpp"

using namespace blunted;

class LeagueCalendarPage : public Gui2Page {
public:
  LeagueCalendarPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~LeagueCalendarPage();
  virtual void Process();

protected:
  void RefreshFixtures();
  void GoBack();

  Gui2Pulldown* leagueFilterPulldown;
  Gui2Frame* frame;
  Gui2Table* fixturesTable;
  Gui2Button* backButton;
  std::string m_selectedLeagueID;
  unsigned long pageCreatedTime_ms;
  bool autoAdvanceTriggered;
};

#endif
