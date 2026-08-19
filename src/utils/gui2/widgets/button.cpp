#include "button.hpp"

#include <cmath>
#include "SDL2/SDL2_rotozoom.h"

namespace blunted {

Gui2Button::Gui2Button(Gui2WindowManager* windowManager, const std::string& name, float x_percent,
                       float y_percent, float width_percent, float height_percent,
                       const std::string& caption)
    : Gui2View(windowManager, name, x_percent, y_percent, width_percent, height_percent) {
  isSelectable = true;

  color = windowManager->GetStyle()->GetColor(e_DecorationType_Bright1);

  int x, y, w, h;
  windowManager->GetCoordinates(x_percent, y_percent, width_percent, height_percent, x, y, w, h);
  image = windowManager->CreateImage2D(name, w, h, true);

  captionView = new Gui2Caption(windowManager, name + "caption", 1.2, 0.3, width_percent,
                                height_percent - 0.6, caption);
  this->AddView(captionView);
  captionView->Show();

  fadeOutTime_ms = 200;
  fadeOut_ms = fadeOutTime_ms;

  toggleable = false;
  toggled = false;
  active = true;

  Redraw();
}

Gui2Button::~Gui2Button() {}

void Gui2Button::GetImages(std::vector<boost::intrusive_ptr<Image2D>>& target) {
  target.push_back(image);
  Gui2View::GetImages(target);
}

void Gui2Button::Process() {
  // printf("gui2button %s :: processing\n", name.c_str());
  if (fadeOut_ms <= fadeOutTime_ms) {
    fadeOut_ms += windowManager->GetTimeStep_ms();
    if (!IsFocussed() && fadeOut_ms <= fadeOutTime_ms) {  // cool fadeout effect!
      Redraw();
    }
  }

  Gui2View::Process();
}

void Gui2Button::SetColor(const Vector3& color) {
  this->color = color;
  captionView->SetColor(color);
  Redraw();
}

void Gui2Button::Redraw() {
  int x, y, w, h;
  windowManager->GetCoordinates(x_percent, y_percent, width_percent, height_percent, x, y, w, h);
  
  // Base background fill (always slightly transparent dark)
  Vector3 baseColor = windowManager->GetStyle()->GetColor(e_DecorationType_Dark1);
  if (!active) {
    baseColor = windowManager->GetStyle()->GetColor(e_DecorationType_Dark2);
    image->DrawRectangle(0, 0, w, h, baseColor, 120); // Disabled transparency
  } else {
    // Dynamic full-button highlight overlay (Modern flat style)
    float bias = IsFocussed() ? 0.0f : (fadeOut_ms / (float)fadeOutTime_ms);
    Vector3 highlightColor = windowManager->GetStyle()->GetColor(e_DecorationType_Bright2);
    
    if (toggleable && toggled) {
      highlightColor = windowManager->GetStyle()->GetColor(e_DecorationType_Toggled);
      bias = 0.0f; // Always highlight if toggled
    }
    
    // Blend base and highlight based on focus/fade
    Vector3 finalColor = highlightColor * (1.0f - bias) + baseColor * bias;
    int finalAlpha = 200 + int(55.0f * (1.0f - bias)); // Fully opaque on hover
    
    image->DrawRectangle(0, 0, w, h, finalColor, finalAlpha);
    
    // Sleek border highlight for extra polish
    if (IsFocussed() || (toggleable && toggled)) {
      image->DrawRectangle(0, 0, w, 2, highlightColor, 255); // Top border
      image->DrawRectangle(0, h - 2, w, 2, highlightColor, 255); // Bottom border
    }
  }

  image->OnChange();
}

void Gui2Button::ProcessWindowingEvent(WindowingEvent* event) {
  if (event->IsActivate() && active) {
    windowManager->PlayClickSound();
    if (toggleable) {
      if (toggled) {
        toggled = false;
      } else {
        toggled = true;
      }
      Redraw();
    }
    sig_OnClick(this);
  } else {
    event->Ignore();
  }
}

void Gui2Button::OnGainFocus() {
  windowManager->PlayHoverSound();
  Redraw();
  sig_OnGainFocus(this);
}

void Gui2Button::OnLoseFocus() {
  fadeOut_ms = 0;
  sig_OnLoseFocus(this);
}

}  // namespace blunted
