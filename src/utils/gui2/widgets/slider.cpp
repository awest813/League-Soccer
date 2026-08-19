#include "slider.hpp"

#include <cmath>

#include "../windowmanager.hpp"
#include "SDL2/SDL2_rotozoom.h"

namespace blunted {

Gui2Slider::Gui2Slider(Gui2WindowManager* windowManager, const std::string& name, float x_percent,
                       float y_percent, float width_percent, float height_percent,
                       const std::string& caption)
    : Gui2View(windowManager, name, x_percent, y_percent, width_percent, height_percent),
      quantizationSteps(51),
      caption(caption) {
  isSelectable = true;

  int x, y, w, h;
  windowManager->GetCoordinates(x_percent, y_percent, width_percent, height_percent, x, y, w, h);
  image = windowManager->CreateImage2D(name, w, h, true);

  fadeOutTime_ms = 200;
  fadeOut_ms = fadeOutTime_ms;
  switchHelperDescriptionTime_ms = -1;
  switchHelperDescription_ms = 0;
  activeDescription = -1;

  value = 0.5f;
  quantizedValue = value;

  titleCaption =
      new Gui2Caption(windowManager, name + "caption", 1.0, 0.2, width_percent, 2.4, caption);
  this->AddView(titleCaption);
  titleCaption->Show();

  valueCaption =
      new Gui2Caption(windowManager, name + "valuecaption", 1.0, 0.2, width_percent, 2.4, "");
  this->AddView(valueCaption);
  valueCaption->Show();

  customValueText = "";

  Redraw();
}

Gui2Slider::~Gui2Slider() {}

void Gui2Slider::GetImages(std::vector<boost::intrusive_ptr<Image2D>>& target) {
  target.push_back(image);
  Gui2View::GetImages(target);
}

void Gui2Slider::Process() {
  // printf("gui2slider %s :: processing\n", name.c_str());
  if (fadeOut_ms <= fadeOutTime_ms) {
    fadeOut_ms += windowManager->GetTimeStep_ms();
    if (!IsFocussed() && fadeOut_ms <= fadeOutTime_ms) {  // cool fadeout effect!
      Redraw();
    }
  }

  if (IsFocussed())
    switchHelperDescription_ms += windowManager->GetTimeStep_ms();
  if (switchHelperDescriptionTime_ms != -1 &&
      switchHelperDescription_ms > switchHelperDescriptionTime_ms) {
    switchHelperDescription_ms = 0;
    activeDescription++;
    if (activeDescription >= (signed int)helperValues.size())
      activeDescription = -1;  // reset loop
    if (!IsFocussed())
      activeDescription = -1;
    for (int i = -1; i < (signed int)helperValues.size(); i++) {
      if (i != activeDescription) {
        if (i == -1) {
          titleCaption->Hide();
        } else {
          helperValues.at((unsigned int)i).descriptionCaption->Hide();
        }

      } else {
        if (i == -1) {
          titleCaption->Show();
        } else {
          helperValues.at((unsigned int)i).descriptionCaption->Show();
        }
      }
    }
  }

  Gui2View::Process();
}

void Gui2Slider::Redraw() {
  int x, y, w, h;
  windowManager->GetCoordinates(x_percent, y_percent, width_percent, height_percent, x, y, w, h);
  float x_ratio = w / width_percent;   // 1% width in px
  float y_ratio = h / height_percent;  // 1% height in px
  int x_margin = std::max(1, int(round(x_ratio * 0.4f)));

  Vector3 darkColor = windowManager->GetStyle()->GetColor(e_DecorationType_Dark1);
  Vector3 brightColor = windowManager->GetStyle()->GetColor(e_DecorationType_Bright2);
  Vector3 accentColor = windowManager->GetStyle()->GetColor(e_DecorationType_Bright1);
  Vector3 trackColor = windowManager->GetStyle()->GetColor(e_DecorationType_Dark2);

  float bias = IsFocussed() ? 0.0f : (fadeOut_ms / (float)fadeOutTime_ms);
  bias = std::max(0.0f, std::min(1.0f, bias));
  Vector3 bgColor = brightColor * (1.0f - bias) + darkColor * bias;
  int bgAlpha = 180 + int(75.0f * (1.0f - bias));

  // Background frame
  image->DrawRectangle(0, 0, w, h, bgColor, bgAlpha);

  // Focus accent top & bottom borders
  if (IsFocussed()) {
    image->DrawRectangle(0, 0, w, 2, brightColor, 255);
    image->DrawRectangle(0, h - 2, w, 2, brightColor, 255);
  }

  // Groove / Track geometry
  int trackMarginX = int(round(x_ratio * 0.8f));
  int trackX = trackMarginX;
  int trackY = int(h * 0.65f);
  int trackW = w - trackMarginX * 2;
  int trackH = std::max(3, int(h * 0.12f));

  // Track background (unfilled)
  image->DrawRectangle(trackX, trackY, trackW, trackH, trackColor, 220);

  // Track border / outline
  image->DrawRectangle(trackX, trackY, trackW, 1, darkColor, 255);
  image->DrawRectangle(trackX, trackY + trackH - 1, trackW, 1, darkColor, 255);

  // Filled progress bar (from start to slider thumb)
  int fillW = int(round(quantizedValue * trackW));
  if (fillW > 0) {
    Vector3 fillCol = IsFocussed() ? accentColor : (accentColor * 0.7f + brightColor * 0.3f);
    image->DrawRectangle(trackX, trackY, fillW, trackH, fillCol, 230);
  }

  // Helper value markers (e.g. factory default ticks)
  for (unsigned int i = 0; i < helperValues.size(); i++) {
    float helperVal = helperValues.at(i).value;
    int tickX = trackX + int(round(helperVal * (trackW - 2)));
    Vector3 helperCol = helperValues.at(i).color;
    image->DrawRectangle(tickX, trackY - 2, 2, trackH + 4, helperCol, 255);
  }

  // Slider thumb / knob
  int thumbW = std::max(4, int(round(x_ratio * 0.5f)));
  int thumbH = std::max(8, int(h * 0.42f));
  int thumbY = trackY - (thumbH - trackH) / 2;
  int thumbX = trackX + int(round(quantizedValue * (trackW - thumbW)));

  // Thumb body and border
  Vector3 thumbCol = IsFocussed() ? accentColor : brightColor;
  image->DrawRectangle(thumbX, thumbY, thumbW, thumbH, thumbCol, 255);
  image->DrawRectangle(thumbX, thumbY, thumbW, 1, Vector3(255, 255, 255), 255);
  image->DrawRectangle(thumbX, thumbY + thumbH - 1, thumbW, 1, darkColor, 255);

  image->OnChange();
  UpdateValueText();
}

void Gui2Slider::SetValueText(const std::string& text) {
  customValueText = text;
  UpdateValueText();
}

void Gui2Slider::UpdateValueText() {
  if (!valueCaption)
    return;

  std::string text = customValueText;
  if (text.empty()) {
    text = std::to_string(static_cast<int>(std::round(quantizedValue * 100.0f))) + "%";
  }
  valueCaption->SetCaption(text);
  UpdateValuePosition();
}

void Gui2Slider::UpdateValuePosition() {
  if (!valueCaption)
    return;
  const float textWidth = valueCaption->GetTextWidthPercent();
  const float x = std::max(1.0f, width_percent - textWidth - 1.0f);
  valueCaption->SetPosition(x, 0.2f);
}

void Gui2Slider::ProcessWindowingEvent(WindowingEvent* event) {
  Vector3 direction = event->GetDirection();

  float xoffset = 0;
  if (direction.coords[0] < -0.3f)
    xoffset = -pow(-direction.coords[0], 2.0f);
  if (direction.coords[0] > 0.3f)
    xoffset = pow(direction.coords[0], 2.0f);

  // Digital input reaches either extreme. Analog input should remain
  // proportional in both directions instead of making every left movement a
  // full quantized step.
  const bool fullStep = std::fabs(direction.coords[0]) >= 0.99f;

  if (xoffset != 0) {
    if (!fullStep) {
      value += xoffset * 0.01f;
    } else {
      value += xoffset * (1.0f / (quantizationSteps - 1.0f));
    }
    if (value > 1.0f)
      value = 1.0f;
    if (value < 0.0f)
      value = 0.0f;
    quantizedValue = round(value * (quantizationSteps - 1)) / (quantizationSteps - 1.0f);
    sig_OnChange(this);
    Redraw();
  } else {
    event->Ignore();
  }
}

void Gui2Slider::OnGainFocus() {
  Redraw();
}

void Gui2Slider::OnLoseFocus() {
  fadeOut_ms = 0;
  switchHelperDescription_ms = 0;
  activeDescription = -1;
  for (unsigned int i = 0; i < helperValues.size(); i++) {
    helperValues.at(i).descriptionCaption->Hide();
  }
  titleCaption->Show();
}

void Gui2Slider::SetValue(float newValue) {
  value = clamp(newValue, 0.0f, 1.0f);
  quantizedValue = round(value * (quantizationSteps - 1)) / (quantizationSteps - 1.0f);
  Redraw();
}

int Gui2Slider::AddHelperValue(const Vector3& color, const std::string& description,
                               float initialValue) {
  Gui2Slider_HelperValue helper;
  helper.index = helperValues.empty() ? 1 : helperValues.at(helperValues.size() - 1).index + 1;
  helper.color = color;
  helper.descriptionCaption = new Gui2Caption(
      windowManager, "gui2sliderhelper_" + int_to_str(helper.index) + "_description_caption", 1.0,
      0.2, width_percent, 2.4, description);
  helper.descriptionCaption->SetColor(helper.color);
  helper.descriptionCaption->SetTransparency(0.3f);
  this->AddView(helper.descriptionCaption);
  helper.descriptionCaption->Hide();
  helper.value = clamp(initialValue, 0.0f, 1.0f);
  helperValues.push_back(helper);
  return helper.index;
  // Redraw();
}

void Gui2Slider::SetHelperValue(int index, float value) {
  for (unsigned int i = 0; i < helperValues.size(); i++) {
    if (helperValues.at(i).index == index) {
      helperValues.at(i).value = clamp(value, 0.0f, 1.0f);
    }
  }
}

void Gui2Slider::DeleteHelperValue(int index) {
  auto iter = helperValues.begin();
  while (iter != helperValues.end()) {
    if ((*iter).index == index) {
      (*iter).descriptionCaption->Exit();
      delete (*iter).descriptionCaption;
      iter = helperValues.erase(iter);
      break;
    }
    iter++;
  }
}

}  // namespace blunted
