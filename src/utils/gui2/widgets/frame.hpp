#ifndef _HPP_GUI2_VIEW_FRAME
#define _HPP_GUI2_VIEW_FRAME

#include "../view.hpp"
#include "image.hpp"

namespace blunted {

class Gui2Frame : public Gui2View {
public:
  Gui2Frame(Gui2WindowManager* windowManager, const std::string& name, float x_percent,
            float y_percent, float width_percent, float height_percent, bool background = false);
  virtual ~Gui2Frame();

  // Resizes the background too, so a frame can be fitted to its content
  // after the content has been laid out.
  virtual void SetSize(float new_width_percent, float new_height_percent);

protected:
  void DrawBackground();

  Gui2Image* background = nullptr;
};

}  // namespace blunted

#endif
