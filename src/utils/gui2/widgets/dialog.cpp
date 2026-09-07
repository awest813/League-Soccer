#include "dialog.hpp"

#include <algorithm>

#include "button.hpp"
#include "caption.hpp"
#include "grid.hpp"
#include "text.hpp"

namespace blunted {

Gui2Dialog::Gui2Dialog(Gui2WindowManager* windowManager, const std::string& name, float x_percent,
                       float y_percent, float width_percent, float height_percent,
                       const std::string& caption)
    : Gui2Frame(windowManager, name + "_frame", x_percent, y_percent, width_percent, height_percent,
                true) {
  isOverlay = true;

  grid = new Gui2Grid(windowManager, name + "_maingrid", 0, 0, width_percent, height_percent - 5);

  Gui2Caption* title = new Gui2Caption(windowManager, name + "title", 0, 0, std::max(1.0f, width_percent - 2.0f), 3, caption);
  grid->AddView(title, 0, 0);

  grid->UpdateLayout(1, 1, 1, 1);
  this->AddView(grid);
  grid->Show();
}

Gui2Dialog::~Gui2Dialog() {}

void Gui2Dialog::AddContent(Gui2View* view) {
  if (auto* text = dynamic_cast<Gui2Text*>(view))
    text->SetSize(std::max(1.0f, width_percent - 2.0f),
                  std::max(1.0f, height_percent - 11.0f));
  grid->AddView(view);
  view->Show();
  grid->UpdateLayout(1, 1, 1, 1);
}

Gui2Button* Gui2Dialog::AddPosNegButtons(const std::string& posName, const std::string& negName,
                                            bool negativeIsCancel) {
  hasNegativeAction = true;
  cancelUsesNegative = negativeIsCancel;
  const float buttonWidth = std::min(20.0f, (width_percent - 4.0f) * 0.5f);
  Gui2Grid* actions = new Gui2Grid(windowManager, name + "_actions",
                                  (width_percent - 2 * buttonWidth - 1) * 0.5f,
                                  height_percent - 4, 2 * buttonWidth + 1, 3);
  actions->SetWrapping(false, true);
  Gui2Button* posButton = new Gui2Button(windowManager, name + "_positive", 0, 0,
                                        buttonWidth, 3, posName);
  Gui2Button* negButton = new Gui2Button(windowManager, name + "_negative", 0, 0,
                                        buttonWidth, 3, negName);
  posButton->sig_OnClick.connect([this](auto*) { sig_OnPositive(this); });
  negButton->sig_OnClick.connect([this](auto*) { sig_OnNegative(this); });
  actions->AddView(posButton, 0, 0);
  actions->AddView(negButton, 0, 1);
  actions->UpdateLayout(0.25f, 0.25f, 0, 0);
  this->AddView(actions);
  actions->Show();
  // Callers focus the returned control: start on the non-destructive choice.
  return negativeIsCancel ? negButton : posButton;
}

Gui2Button* Gui2Dialog::AddSingleButton(const std::string& caption) {
  float buttonWidth = 20;
  Gui2Button* theButton = new Gui2Button(windowManager, name + "_" + caption + "_button",
                                         width_percent / 2.0 - buttonWidth / 2.0,
                                         height_percent - 4, buttonWidth, 3, caption);
  theButton->sig_OnClick.connect([this](auto*) { sig_OnPositive(this); });
  this->AddView(theButton);
  theButton->Show();
  return theButton;
}

void Gui2Dialog::ProcessWindowingEvent(WindowingEvent* event) {
  // Keep directional navigation inside the modal instead of reaching the page.
  event->Accept();
  if (event->IsEscape()) {
    if (!sig_OnCancel.empty())
      sig_OnCancel(this);
    else if (hasNegativeAction && cancelUsesNegative)
      sig_OnNegative(this);
    else
      sig_OnPositive(this);
  }
}

}  // namespace blunted
