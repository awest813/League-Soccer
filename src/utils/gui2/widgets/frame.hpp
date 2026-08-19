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

protected:
};

}  // namespace blunted

#endif
