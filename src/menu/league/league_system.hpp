#ifndef _HPP_MENU_LEAGUE_SYSTEM
#define _HPP_MENU_LEAGUE_SYSTEM

#include "utils/gui2/page.hpp"
#include "utils/gui2/widgets/button.hpp"
#include "utils/gui2/widgets/caption.hpp"
#include "utils/gui2/widgets/dialog.hpp"
#include "utils/gui2/widgets/editline.hpp"
#include "utils/gui2/widgets/frame.hpp"
#include "utils/gui2/widgets/grid.hpp"
#include "utils/gui2/widgets/pulldown.hpp"
#include "utils/gui2/widgets/slider.hpp"
#include "utils/gui2/windowmanager.hpp"

#include "../pagefactory.hpp"

using namespace blunted;

class LeagueSystemPage : public Gui2Page {
public:
  LeagueSystemPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~LeagueSystemPage();
  virtual void Process();

protected:
  void GoPage(e_PageID pageID);
  unsigned long pageCreatedTime_ms;
  bool autoAdvanceTriggered;
};

class LeagueSystemSavePage : public Gui2Page {
public:
  LeagueSystemSavePage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~LeagueSystemSavePage();

protected:
  void RefreshSlots();
  void SaveToSlot(int slotIndex);
  void LoadSlot(int slotIndex);
  void DeleteSlot(int slotIndex);
  void SetFeedback(const std::string& message);

  Gui2Frame* frame;
  Gui2Caption* feedbackCaption;
  Gui2Grid* slotsGrid;
};

class LeagueSystemSettingsPage : public Gui2Page {
public:
  LeagueSystemSettingsPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~LeagueSystemSettingsPage();
  virtual void Process();

protected:
  void SaveSettings();

  Gui2EditLine* editManagerName;
  Gui2Pulldown* pulldownCurrency;
  Gui2Slider* sliderDifficulty;
  Gui2EditLine* editSeasonYear;
  Gui2Frame* frame;

  unsigned long pageCreatedTime_ms;
  bool autoAdvanceTriggered;
};

#endif
