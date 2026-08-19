#ifndef _HPP_GUI2_STYLE
#define _HPP_GUI2_STYLE

#include "SDL2/SDL_ttf.h"
#include "base/math/vector3.hpp"

namespace blunted {

enum e_TextType {
  e_TextType_Default,
  e_TextType_DefaultOutline,
  e_TextType_Caption,
  e_TextType_Title,
  e_TextType_ToolTip
};

enum e_DecorationType {
  e_DecorationType_Dark1,
  e_DecorationType_Dark2,
  e_DecorationType_Bright1,
  e_DecorationType_Bright2,
  e_DecorationType_Toggled
};

class Gui2Style {
public:
  Gui2Style();
  virtual ~Gui2Style();

  void SetFont(e_TextType textType, TTF_Font* font);
  void SetColor(e_DecorationType decorationType, const Vector3& color);

  TTF_Font* GetFont(e_TextType textType) const;
  TTF_Font* GetOutlineFont(e_TextType textType) const;
  Vector3 GetColor(e_DecorationType decorationType) const;

protected:
  std::map<e_TextType, TTF_Font*> fonts;
  std::map<e_DecorationType, Vector3> colors;
};

}  // namespace blunted

#endif
