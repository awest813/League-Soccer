#ifndef _HPP_MENU_INGAME_REPLAY
#define _HPP_MENU_INGAME_REPLAY

#include "../../onthepitch/match.hpp"
#include "utils/gui2/page.hpp"
#include "utils/gui2/widgets/caption.hpp"
#include "utils/gui2/windowmanager.hpp"

using namespace blunted;

class ReplayPage : public Gui2Page {
public:
  ReplayPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~ReplayPage();

  void OnClose();
  void Autorun(int replayHistoryOffset_ms, bool stayInReplay);

protected:
  Match* match;

  virtual void Process();
  virtual void ProcessKeyboardEvent(KeyboardEvent* event);
  virtual void ProcessJoystickEvent(JoystickEvent* event);
  void ProcessInput(const Vector3& direction, bool button1, bool button2, bool slowMotion);
  void UpdateTimeLabel();

  signed long actualTime_ms;
  unsigned long minTime_ms;
  unsigned long maxTime_ms;

  int cam;
  int replayCamCount;
  float modifierValue;

  bool autoRun;
  bool stayInReplay;
  bool slowMotion;
  bool closeWhenAutorunCompletes;

  Gui2Caption* timeLabel;
};

#endif
