#ifndef _HPP_MENU_LEAGUE
#define _HPP_MENU_LEAGUE

#include "utils/gui2/page.hpp"
#include "utils/gui2/widgets/button.hpp"
#include "utils/gui2/widgets/dialog.hpp"
#include "utils/gui2/widgets/editline.hpp"
#include "utils/gui2/widgets/filebrowser.hpp"
#include "utils/gui2/widgets/frame.hpp"
#include "utils/gui2/widgets/grid.hpp"
#include "utils/gui2/widgets/pulldown.hpp"
#include "utils/gui2/widgets/slider.hpp"
#include "utils/gui2/windowmanager.hpp"

using namespace blunted;

class LeaguePage : public Gui2Page {
public:
  LeaguePage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~LeaguePage();
  virtual void Process();

protected:
  void GoForward();
  void GoMainMenu();
  void StepTime();
  void SetTimeCaption();

  Gui2Caption* captionTime;
  unsigned long pageCreatedTime_ms;
  bool autoAdvanceTriggered;
  bool autoStepTriggered;
};

class LeagueStartPage : public Gui2Page {
public:
  LeagueStartPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~LeagueStartPage();
  virtual void Process();

protected:
  void GoLoad();
  void GoNew();

  unsigned long pageCreatedTime_ms;
  bool autoAdvanceTriggered;
};

class LeagueStartLoadPage : public Gui2Page {
public:
  LeagueStartLoadPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~LeagueStartLoadPage();

protected:
  void GoLoadSave();

  Gui2FileBrowser* browser;
};

class LeagueStartNewPage : public Gui2Page {
public:
  LeagueStartNewPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~LeagueStartNewPage();
  virtual void Process();

protected:
  void GoDatabaseSelectDialog();
  void CloseDatabaseSelectDialog();
  void GoProceed();
  void CloseCreateSaveDialog();
  void RefreshTeamSelect();

  Gui2Button* databaseSelectButton;
  Gui2Pulldown* currencySelectPulldown;
  Gui2Slider* difficultySlider;
  Gui2EditLine* saveNameInput;
  Gui2EditLine* managerNameInput;
  Gui2Pulldown* teamSelectPulldown;

  Gui2Dialog* databaseSelectDialog;
  Gui2FileBrowser* databaseSelectBrowser;
  Gui2View* previousFocus;

  std::string data_SelectedDatabase;
  std::string data_SelectedTeamID;

  Gui2Dialog* createSaveDialog;

  bool success;
  unsigned long pageCreatedTime_ms;
  unsigned long dialogShownTime_ms;
  bool autoAdvanceTriggered;
  bool autoCloseDialogTriggered;
};

#endif
