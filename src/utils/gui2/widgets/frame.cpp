#include "frame.hpp"

#include "../windowmanager.hpp"

namespace blunted {

Gui2Frame::Gui2Frame(Gui2WindowManager* windowManager, const std::string& name, float x_percent,
                     float y_percent, float width_percent, float height_percent, bool background)
    : Gui2View(windowManager, name, x_percent, y_percent, width_percent, height_percent) {
  if (background) {
    this->background =
        new Gui2Image(windowManager, name + "_frame", 0, 0, width_percent, height_percent);
    this->AddView(this->background);
    DrawBackground();
    this->background->Show();
  }
}

Gui2Frame::~Gui2Frame() {}

void Gui2Frame::DrawBackground() {
  int x, y, w, h;
  windowManager->GetCoordinates(x_percent, y_percent, width_percent, height_percent, x, y, w, h);

  // Modern sleek modal/frame background matching the global theme
  Vector3 bgColor = windowManager->GetStyle()->GetColor(e_DecorationType_Dark1);
  background->GetImage2D()->DrawRectangle(
      0, 0, w, h, bgColor, 220);  // Solid translucent flat color instead of an old static image
  background->GetImage2D()->DrawRectangle(
      0, 0, w, 2, windowManager->GetStyle()->GetColor(e_DecorationType_Bright2),
      255);  // Top accent border
  background->GetImage2D()->OnChange();
}

void Gui2Frame::SetSize(float new_width_percent, float new_height_percent) {
  Gui2View::SetSize(new_width_percent, new_height_percent);
  if (background) {
    background->SetSize(new_width_percent, new_height_percent);
    DrawBackground();
  }
}

}  // namespace blunted
