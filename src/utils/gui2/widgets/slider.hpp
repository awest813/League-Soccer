#ifndef _HPP_GUI2_VIEW_SLIDER
#define _HPP_GUI2_VIEW_SLIDER

#include "../view.hpp"
#include "SDL2/SDL_ttf.h"
#include "caption.hpp"
#include "scene/objects/image2d.hpp"

namespace blunted {

struct Gui2Slider_HelperValue {
  int index;
  Vector3 color;
  float value;
  Gui2Caption* descriptionCaption;
};

class Gui2Slider : public Gui2View {
public:
  Gui2Slider(Gui2WindowManager* windowManager, const std::string& name, float x_percent,
             float y_percent, float width_percent, float height_percent,
             const std::string& caption);
  virtual ~Gui2Slider();

  virtual void GetImages(std::vector<boost::intrusive_ptr<Image2D>>& target);

  virtual void Process();
  virtual void Redraw();

  virtual void ProcessWindowingEvent(WindowingEvent* event);

  virtual void Show() {
    titleCaption->Show();
    Gui2View::Show();
  }  // ignore helper descriptions

  virtual void OnGainFocus();
  virtual void OnLoseFocus();

  void SetValue(float newValue);
  float GetValue() { return quantizedValue; }
  void SetCaption(const std::string& newCaption) {
    caption = newCaption;
    titleCaption->SetCaption(newCaption);
  }

  void SetQuantization(int steps) { quantizationSteps = std::max(steps, 2); }

  // Replaces the automatic numeric readout on the right side of the slider
  // with a custom label. Passing an empty string restores the default
  // percentage readout.
  void SetValueText(const std::string& text);

  int AddHelperValue(const Vector3& color, const std::string& description,
                     float initialValue = 0.0f);
  void SetHelperValue(int index, float value);
  void DeleteHelperValue(int index);

  boost::signals2::signal<void(Gui2Slider*)> sig_OnChange;

protected:
  boost::intrusive_ptr<Image2D> image;

  int fadeOut_ms;
  int fadeOutTime_ms;
  int switchHelperDescription_ms;
  int switchHelperDescriptionTime_ms;
  int activeDescription;
  int quantizationSteps;

  std::string caption;

  Gui2Caption* titleCaption;

  std::vector<Gui2Slider_HelperValue> helperValues;

  float value;
  float quantizedValue;

  Gui2Caption* valueCaption;
  std::string customValueText;

  void UpdateValueText();
  void UpdateValuePosition();
};

}  // namespace blunted

#endif
