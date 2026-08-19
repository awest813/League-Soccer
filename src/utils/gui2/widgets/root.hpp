#ifndef _HPP_GUI2_VIEW_ROOT
#define _HPP_GUI2_VIEW_ROOT

#include "../view.hpp"

namespace blunted {

class Gui2Root : public Gui2View {
public:
  Gui2Root(Gui2WindowManager* windowManager, const std::string& name, float x_percent,
           float y_percent, float width_percent, float height_percent);
  virtual ~Gui2Root();

protected:
};

}  // namespace blunted

#endif
