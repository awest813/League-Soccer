#ifndef _HPP_GUI2_VIEW_PLAYERHUD
#define _HPP_GUI2_VIEW_PLAYERHUD

#include "utils/gui2/view.hpp"
#include "utils/gui2/widgets/caption.hpp"
#include "utils/gui2/widgets/image.hpp"

using namespace blunted;

class Match;

class Gui2PlayerHUD : public Gui2View {
public:
  Gui2PlayerHUD(Gui2WindowManager* windowManager, Match* match);
  virtual ~Gui2PlayerHUD();

  virtual void Put();

protected:
  Match* match;

  Gui2Caption* playerNameCaption[2];
  Gui2Caption* roleCaption[2];
  Gui2Caption* conditionCaption[2];
  Gui2Image* staminaImage[2];
  Gui2Image* powerGaugeImage[2];

  std::string lastPlayerName[2];
  std::string lastRole[2];
  std::string lastCondition[2];
  float lastStamina[2];
  float lastPowerGauge[2];
};

#endif
