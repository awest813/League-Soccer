#include "root.hpp"

namespace blunted {

Gui2Root::Gui2Root(Gui2WindowManager* windowManager, const std::string& name, float x_percent,
                   float y_percent, float width_percent, float height_percent)
    : Gui2View(windowManager, name, x_percent, y_percent, width_percent, height_percent) {}

Gui2Root::~Gui2Root() {}

}  // namespace blunted
