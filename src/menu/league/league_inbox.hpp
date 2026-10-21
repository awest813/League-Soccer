#ifndef _HPP_MENU_LEAGUE_INBOX
#define _HPP_MENU_LEAGUE_INBOX

#include "../pagefactory.hpp"
#include "utils/gui2/page.hpp"
#include "utils/gui2/widgets/button.hpp"
#include "utils/gui2/widgets/caption.hpp"
#include "utils/gui2/widgets/dialog.hpp"
#include "utils/gui2/widgets/frame.hpp"
#include "utils/gui2/widgets/grid.hpp"
#include "utils/gui2/widgets/text.hpp"
#include "utils/gui2/windowmanager.hpp"

using namespace blunted;

class LeagueInboxPage : public Gui2Page {
public:
  LeagueInboxPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~LeagueInboxPage();
  virtual void Process();

protected:
  void RefreshMessages();
  void GoBack();
  void DeleteMessage(int msgID);

  Gui2Frame* frame;
  Gui2Grid* messageGrid;
  Gui2Caption* countCaption;
  std::string m_selectedLeagueID;
  unsigned long pageCreatedTime_ms;
  bool autoAdvanceTriggered;
};

#endif
