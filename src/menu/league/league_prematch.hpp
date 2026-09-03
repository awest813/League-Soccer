#ifndef _HPP_MENU_LEAGUE_PREMATCH
#define _HPP_MENU_LEAGUE_PREMATCH

#include "utils/gui2/page.hpp"
#include "utils/gui2/widgets/button.hpp"
#include "utils/gui2/widgets/caption.hpp"
#include "utils/gui2/widgets/frame.hpp"
#include "utils/gui2/widgets/grid.hpp"
#include "utils/gui2/widgets/image.hpp"
#include "utils/gui2/windowmanager.hpp"

#include "../pagefactory.hpp"
#include "../../league/leaguecode.hpp"

using namespace blunted;

// Pre-match screen for the user's next season fixture: shows the pairing and
// offers to kick it off in the 3D match engine or simulate it, after which the
// whole matchday is resolved and summarised.
class LeaguePreMatchPage : public Gui2Page {
public:
  LeaguePreMatchPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~LeaguePreMatchPage();
  virtual void Process();

protected:
  void KickOff();
  void Simulate();
  void GoBackToDashboard();

  LeagueFixtureInfo fixture;
  bool hasFixture;
  bool userIsHome;
  unsigned long pageCreatedTime_ms;
  bool autoAdvanceTriggered;
};

#endif
