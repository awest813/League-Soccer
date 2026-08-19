#ifndef _HPP_MENU_LOADINGMATCH
#define _HPP_MENU_LOADINGMATCH

#include "../../onthepitch/match.hpp"
#include "utils/gui2/widgets/grid.hpp"
#include "utils/gui2/widgets/image.hpp"
#include "utils/gui2/widgets/menu.hpp"
#include "utils/gui2/widgets/root.hpp"
#include "utils/gui2/windowmanager.hpp"

using namespace blunted;

class LoadingMatchPage : public Gui2Page {
public:
  LoadingMatchPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~LoadingMatchPage();

  virtual void Process();

  void Close();

protected:
  bool sentStartGameSignal;
};

#endif
