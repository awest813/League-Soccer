#ifndef _HPP_MENU_PHASE
#define _HPP_MENU_PHASE

#include "../../gamedefines.hpp"
#include "../../onthepitch/match.hpp"
#include "utils/gui2/page.hpp"
#include "utils/gui2/widgets/button.hpp"
#include "utils/gui2/widgets/grid.hpp"
#include "utils/gui2/widgets/root.hpp"
#include "utils/gui2/windowmanager.hpp"

using namespace blunted;

class MatchPhasePage : public Gui2Page {
public:
  MatchPhasePage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~MatchPhasePage();

  virtual void Process();
  void GoGamePlan();

  Gui2Button* buttonNext;

  void ContinueGame();
  virtual void ProcessWindowingEvent(WindowingEvent* event);

protected:
  Gui2Grid* grid;
  e_MatchPhase nextPhase;
  unsigned long pageCreatedTime_ms;
  bool autoAdvanceTriggered;
};

#endif
