#ifndef _HPP_MENU_INGAME_CAMERA
#define _HPP_MENU_INGAME_CAMERA

#include "utils/gui2/page.hpp"
#include "utils/gui2/widgets/button.hpp"
#include "utils/gui2/widgets/frame.hpp"
#include "utils/gui2/widgets/grid.hpp"
#include "utils/gui2/widgets/root.hpp"
#include "utils/gui2/widgets/slider.hpp"
#include "utils/gui2/windowmanager.hpp"

using namespace blunted;

class CameraPage : public Gui2Page {
public:
  CameraPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~CameraPage();

  void OnClose();

  void UpdateCamera();

  void ApplyPreset(float zoom, float height, float fov, float angleFactor);

protected:
  Gui2Slider* sliderZoom;
  Gui2Slider* sliderHeight;
  Gui2Slider* sliderFOV;
  Gui2Slider* sliderAngleFactor;
};

#endif
