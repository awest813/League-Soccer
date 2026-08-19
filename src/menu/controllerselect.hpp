#ifndef _HPP_MENU_CONTROLLERSELECT
#define _HPP_MENU_CONTROLLERSELECT

#include "../onthepitch/match.hpp"
#include "utils/gui2/widgets/button.hpp"
#include "utils/gui2/widgets/grid.hpp"
#include "utils/gui2/widgets/image.hpp"
#include "utils/gui2/widgets/frame.hpp"
#include "utils/gui2/widgets/menu.hpp"
#include "utils/gui2/widgets/root.hpp"
#include "utils/gui2/windowmanager.hpp"

using namespace blunted;

class ControllerSelectPage : public Gui2Page {
public:
  ControllerSelectPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~ControllerSelectPage();

  void ConfirmSelection();
  void SetImagePositions();

  virtual void Process();
  virtual void ProcessKeyboardEvent(KeyboardEvent* event);
  virtual void ProcessJoystickEvent(JoystickEvent* event);
  virtual void ProcessWindowingEvent(WindowingEvent* event);

protected:
  std::vector<SideSelection> sides;
  std::vector<unsigned long> delay;
  bool inGame;
  bool autoAssignedPlayerOne = false;
  unsigned long pageCreatedTime_ms;
  bool autoAdvanceTriggered;
};

#endif
