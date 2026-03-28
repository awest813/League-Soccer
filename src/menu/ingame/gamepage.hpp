// written by bastiaan konings schuiling 2008 - 2015
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not
// be used for anything important. i do not offer support, so don't ask. to be used for inspiration
// :)

#ifndef _HPP_MENU_GAME
#define _HPP_MENU_GAME

#include <boost/signals2.hpp>

#include "utils/gui2/page.hpp"
#include "utils/gui2/windowmanager.hpp"

class Match;

using namespace blunted;

class GamePage : public Gui2Page {
public:
  GamePage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~GamePage();

  virtual void Process();

  virtual void ProcessWindowingEvent(WindowingEvent* event);
  virtual void ProcessKeyboardEvent(KeyboardEvent* event);
  virtual void ProcessJoystickEvent(JoystickEvent* event);

  void GoShortReplayPage();
  void GoExtendedReplayPage();
  void GoMatchPhasePage();
  void GoGameOverPage();
  void OnCreatedMatch();

protected:
  Match* match;

  boost::signals2::connection conn_MatchPhaseChange;
  boost::signals2::connection conn_ShortReplayMoment;
  boost::signals2::connection conn_ExtendedReplayMoment;
  boost::signals2::connection conn_GameOver;
};

#endif
