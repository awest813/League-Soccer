#ifndef _HPP_MENU_LEAGUE_FORWARD
#define _HPP_MENU_LEAGUE_FORWARD

#include "utils/gui2/page.hpp"
#include "utils/gui2/widgets/button.hpp"
#include "utils/gui2/widgets/caption.hpp"
#include "utils/gui2/widgets/frame.hpp"
#include "utils/gui2/widgets/grid.hpp"
#include "utils/gui2/widgets/image.hpp"
#include "utils/gui2/windowmanager.hpp"

#include "../pagefactory.hpp"

using namespace blunted;

class LeagueForwardPage : public Gui2Page {
public:
  LeagueForwardPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~LeagueForwardPage();
  virtual void Process();

protected:
  void GoPage(e_PageID pageID);
  void GoMainMenu();

  unsigned long pageCreatedTime_ms;
  bool autoAdvanceTriggered;
};

#endif
