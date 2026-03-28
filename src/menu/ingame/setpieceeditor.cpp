#include "setpieceeditor.hpp"

#include "../../onthepitch/match.hpp"
#include "../pagefactory.hpp"
#include "main.hpp"

using namespace blunted;

static const char* kPieceNames[3] = {"freekick", "corner", "goalkick"};

SetPieceEditorPage::SetPieceEditorPage(Gui2WindowManager* windowManager,
                                       const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  teamDatabaseID = pageData.properties->GetInt("teamDatabaseID", 0);

  pieceTypes[0] = e_SetPiece_FreeKick;
  pieceTypes[1] = e_SetPiece_Corner;
  pieceTypes[2] = e_SetPiece_GoalKick;

  SetPieceConfig::EnsureTable(teamDatabaseID);
  for (int i = 0; i < kNumPieces; i++) {
    params[i] = SetPieceConfig::Load(teamDatabaseID, pieceTypes[i]);
  }

  Gui2Image* bg = new Gui2Image(windowManager, "image_spe_bg", 5, 5, 90, 90);
  bg->LoadImage("media/menu/backgrounds/black.png");
  this->AddView(bg);
  bg->Show();

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_spe_title", 0, 8, 100, 4, "set-piece editor");
  title->SetPosition(50 - title->GetTextWidthPercent() / 2, 8);
  this->AddView(title);
  title->Show();

  Gui2Grid* grid = new Gui2Grid(windowManager, "grid_spe", 8, 18, 84, 60);

  // Header
  grid->AddView(new Gui2Caption(windowManager, "spe_h0", 0, 0, 20, 3, "set-piece"), 0, 0);
  grid->AddView(new Gui2Caption(windowManager, "spe_h1", 0, 0, 6, 3, "depth-"), 0, 1);
  grid->AddView(new Gui2Caption(windowManager, "spe_h2", 0, 0, 10, 3, "depth"), 0, 2);
  grid->AddView(new Gui2Caption(windowManager, "spe_h3", 0, 0, 6, 3, "depth+"), 0, 3);
  grid->AddView(new Gui2Caption(windowManager, "spe_h4", 0, 0, 6, 3, "width-"), 0, 4);
  grid->AddView(new Gui2Caption(windowManager, "spe_h5", 0, 0, 10, 3, "width"), 0, 5);
  grid->AddView(new Gui2Caption(windowManager, "spe_h6", 0, 0, 6, 3, "width+"), 0, 6);

  for (int i = 0; i < kNumPieces; i++) {
    int row = i + 1;
    std::string sfx = std::to_string(i);

    grid->AddView(
        new Gui2Caption(windowManager, "spe_name_" + sfx, 0, 0, 20, 3, kPieceNames[i]),
        row, 0);

    // Depth - button
    Gui2Button* btnDm =
        new Gui2Button(windowManager, "spe_dm_" + sfx, 0, 0, 6, 3, "-");
    btnDm->sig_OnClick.connect([this, i](...) {
      params[i].formation_depth = std::max(0.05f, params[i].formation_depth - 0.05f);
      UpdateCaptions();
    });
    grid->AddView(btnDm, row, 1);

    depthCaption[i] = new Gui2Caption(windowManager, "spe_dv_" + sfx, 0, 0, 10, 3, "");
    grid->AddView(depthCaption[i], row, 2);

    // Depth + button
    Gui2Button* btnDp =
        new Gui2Button(windowManager, "spe_dp_" + sfx, 0, 0, 6, 3, "+");
    btnDp->sig_OnClick.connect([this, i](...) {
      params[i].formation_depth = std::min(1.0f, params[i].formation_depth + 0.05f);
      UpdateCaptions();
    });
    grid->AddView(btnDp, row, 3);

    // Width - button
    Gui2Button* btnWm =
        new Gui2Button(windowManager, "spe_wm_" + sfx, 0, 0, 6, 3, "-");
    btnWm->sig_OnClick.connect([this, i](...) {
      params[i].formation_width = std::max(0.05f, params[i].formation_width - 0.05f);
      UpdateCaptions();
    });
    grid->AddView(btnWm, row, 4);

    widthCaption[i] = new Gui2Caption(windowManager, "spe_wv_" + sfx, 0, 0, 10, 3, "");
    grid->AddView(widthCaption[i], row, 5);

    // Width + button
    Gui2Button* btnWp =
        new Gui2Button(windowManager, "spe_wp_" + sfx, 0, 0, 6, 3, "+");
    btnWp->sig_OnClick.connect([this, i](...) {
      params[i].formation_width = std::min(1.0f, params[i].formation_width + 0.05f);
      UpdateCaptions();
    });
    grid->AddView(btnWp, row, 6);
  }

  grid->UpdateLayout(0.5);
  this->AddView(grid);
  grid->Show();

  UpdateCaptions();

  Gui2Button* btnSave =
      new Gui2Button(windowManager, "spe_save", 30, 82, 20, 3, "save");
  this->AddView(btnSave);
  btnSave->Show();
  btnSave->sig_OnClick.connect([this](...) { Save(); });
  btnSave->SetFocus();

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "spe_back", 55, 82, 20, 3, "back");
  this->AddView(btnBack);
  btnBack->Show();
  btnBack->sig_OnClick.connect([this](...) { GoBack(); });

  this->Show();
}

SetPieceEditorPage::~SetPieceEditorPage() {}

void SetPieceEditorPage::UpdateCaptions() {
  for (int i = 0; i < kNumPieces; i++) {
    // Format as e.g. "0.45"
    char buf[16];
    snprintf(buf, sizeof(buf), "%.2f", params[i].formation_depth);
    depthCaption[i]->SetCaption(buf);
    snprintf(buf, sizeof(buf), "%.2f", params[i].formation_width);
    widthCaption[i]->SetCaption(buf);
  }
}

void SetPieceEditorPage::Save() {
  SetPieceConfig::EnsureTable(teamDatabaseID);
  for (int i = 0; i < kNumPieces; i++) {
    SetPieceConfig::Save(teamDatabaseID, pieceTypes[i], params[i]);
  }
  GoBack();
}
