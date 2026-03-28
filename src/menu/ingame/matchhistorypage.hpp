#ifndef _HPP_MENU_MATCHHISTORYPAGE
#define _HPP_MENU_MATCHHISTORYPAGE

#include "utils/gui2/page.hpp"
#include "utils/gui2/windowmanager.hpp"

using namespace blunted;

class MatchHistoryPage : public Gui2Page {
 public:
  MatchHistoryPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~MatchHistoryPage();
};

#endif
