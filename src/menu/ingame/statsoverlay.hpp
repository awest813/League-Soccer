// Statistics overlay HUD - shows live in-match stats below the scoreboard
#ifndef _HPP_GUI2_VIEW_STATSOVERLAY
#define _HPP_GUI2_VIEW_STATSOVERLAY

#include "utils/gui2/view.hpp"
#include "utils/gui2/widgets/caption.hpp"

using namespace blunted;
class Match;

class Gui2StatsOverlay : public Gui2View {
public:
  Gui2StatsOverlay(Gui2WindowManager* windowManager, Match* match);
  virtual ~Gui2StatsOverlay() = default;

  void UpdateStats();
  virtual void Redraw() {}

protected:
  Match* match;
  Gui2Caption* possessionCaption;
  Gui2Caption* shotsCaption;
  Gui2Caption* passCaption;
  Gui2Caption* foulsCaption;
};

#endif
