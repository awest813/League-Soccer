#ifndef _HPP_MENU_SETPIECEEDITOR
#define _HPP_MENU_SETPIECEEDITOR

#include "../../data/setpiececonfig.hpp"
#include "utils/gui2/page.hpp"
#include "utils/gui2/widgets/button.hpp"
#include "utils/gui2/widgets/caption.hpp"
#include "utils/gui2/widgets/grid.hpp"
#include "utils/gui2/windowmanager.hpp"

using namespace blunted;

class SetPieceEditorPage : public Gui2Page {
 public:
  SetPieceEditorPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData);
  virtual ~SetPieceEditorPage();

  void Save();

 protected:
  int teamDatabaseID;

  // One entry per editable set-piece type (FreeKick, Corner, GoalKick)
  static const int kNumPieces = 3;
  e_SetPiece pieceTypes[kNumPieces];
  SetPieceParams params[kNumPieces];
  Gui2Caption* depthCaption[kNumPieces];
  Gui2Caption* widthCaption[kNumPieces];

  void UpdateCaptions();
};

#endif
