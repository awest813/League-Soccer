#ifndef _HPP_MENU_GAMEOVER
#define _HPP_MENU_GAMEOVER

#include "../../onthepitch/match.hpp"
#include "utils/gui2/page.hpp"
#include "utils/gui2/widgets/button.hpp"
#include "utils/gui2/widgets/frame.hpp"
#include "utils/gui2/widgets/grid.hpp"
#include "utils/gui2/widgets/root.hpp"
#include "utils/gui2/windowmanager.hpp"

using namespace blunted;

class GameOverPage : public Gui2Page {
public:
  GameOverPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~GameOverPage();

  virtual void Process();
  virtual void ProcessWindowingEvent(WindowingEvent* event);
  void GoRematch();
  void GoMainMenu();

  Gui2Button* buttonOkay;

protected:
  Match* match;
  unsigned long pageCreatedTime_ms;
  bool autoQuitTriggered;
};

#endif
