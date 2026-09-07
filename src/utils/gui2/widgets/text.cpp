#include "text.hpp"
#include "../textlayout.hpp"

#include "../windowmanager.hpp"
#include "SDL2/SDL_ttf.h"

namespace blunted {

Gui2Text::Gui2Text(Gui2WindowManager* windowManager, const std::string& name, float x_percent,
                   float y_percent, float width_percent, float height_percent,
                   float fontsize_percent, unsigned int maxHorizChars, const std::string& text)
    : Gui2View(windowManager, name, x_percent, y_percent, width_percent, height_percent),
      fontsize_percent(fontsize_percent),
      maxHorizChars(maxHorizChars) {
  int x, y, w, h;
  windowManager->GetCoordinates(x_percent, y_percent, width_percent, height_percent, x, y, w, h);

  color = windowManager->GetStyle()->GetColor(e_DecorationType_Bright1);
  outlineColor = windowManager->GetStyle()->GetColor(e_DecorationType_Dark1);

  if (text.length() > 0)
    AddText(text);
}

Gui2Text::~Gui2Text() {}

void Gui2Text::SetColor(const Vector3& color) {
  if (color != this->color) {
    this->color = color;
    Redraw();
  }
}

void Gui2Text::SetOutlineColor(const Vector3& outlineColor) {
  if (outlineColor != this->outlineColor) {
    this->outlineColor = outlineColor;
    Redraw();
  }
}

void Gui2Text::ClearText() {
  text.clear();

  resultText.clear();
  resultCaptions.clear();

  std::vector<Gui2View*> childrenCopy =
      children;  // need to make copy: child->Exit will remove itself from *this->children
  for (int i = (signed int)childrenCopy.size() - 1; i >= 0; i--) {  // filo
    childrenCopy.at(i)->Exit();
    delete childrenCopy.at(i);
  }
  children.clear();
}

void Gui2Text::AddEmptyLine() {
  AddText("");
}

void Gui2Text::AddText(const std::string& newText) {
  if (!text.empty())
    text += "\n";
  text += newText;
  const auto lines = WrapMenuText(newText, maxHorizChars);
  for (const auto& line : lines) {
    const size_t index = resultText.size();
    resultText.push_back(line);
    auto* caption = new Gui2Caption(windowManager, GetName() + int_to_str(index), 0,
                                    index * fontsize_percent * 1.5f, width_percent,
                                    fontsize_percent, line);
    caption->SetColor(color);
    caption->SetOutlineColor(outlineColor);
    AddView(caption);
    resultCaptions.push_back(caption);
    caption->Show();
  }
}

void Gui2Text::SetSize(float width, float height) {
  Gui2View::SetSize(width, height);
  const float lineHeight = resultCaptions.empty() ? fontsize_percent :
      std::min(fontsize_percent, height / (resultCaptions.size() * 1.5f));
  for (size_t i = 0; i < resultCaptions.size(); ++i) {
    resultCaptions[i]->SetPosition(0, i * lineHeight * 1.5f);
    resultCaptions[i]->SetSize(width, lineHeight);
    resultCaptions[i]->Redraw();
  }
}

}  // namespace blunted
