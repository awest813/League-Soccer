#ifndef _HPP_MENU_INGAME
#define _HPP_MENU_INGAME

#include "../../onthepitch/match.hpp"
#include "../cameramenu.hpp"
#include "utils/gui2/widgets/button.hpp"
#include "utils/gui2/widgets/frame.hpp"
#include "utils/gui2/widgets/grid.hpp"
#include "utils/gui2/widgets/menu.hpp"
#include "utils/gui2/widgets/root.hpp"
#include "utils/gui2/windowmanager.hpp"

using namespace blunted;

class IngamePage : public Gui2Page {
public:
  IngamePage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~IngamePage();

  void GoGamePlan();
  void GoControllerSelect();
  void GoControllerRemap();
  void GoCameraSettings();
  void GoVisualOptions();
  void GoSystemSettings();
  void GoReplay();
  void GoPreQuit();
  void GoSetPieceEditor();

  virtual void ProcessWindowingEvent(WindowingEvent* event);

protected:
  int teamID;  // team that activated the ingame menu
};

class PreQuitPage : public Gui2Page {
public:
  PreQuitPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~PreQuitPage();

  void GoMenu();

protected:
};

#endif
