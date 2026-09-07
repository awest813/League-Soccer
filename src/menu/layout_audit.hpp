#ifndef GF_MENU_LAYOUT_AUDIT_HPP
#define GF_MENU_LAYOUT_AUDIT_HPP

#include <cstdio>
#include "pagefactory.hpp"
#include "utils/gui2/widgets/caption.hpp"
#include "utils/gui2/widgets/dialog.hpp"
#include "utils/gui2/widgets/pulldown.hpp"
#include "utils/gui2/widgets/slider.hpp"
#include "utils/gui2/widgets/editline.hpp"
#include "utils/gui2/widgets/text.hpp"

// Opt-in diagnostics for smoke tests; normal menu behavior is unchanged.
inline void AuditMenuLayout(blunted::Gui2View* view) {
  if (!view->IsVisible())
    return;
  float x, y, w, h;
  view->GetDerivedPosition(x, y);
  view->GetSize(w, h);
  auto* caption = dynamic_cast<blunted::Gui2Caption*>(view);
  if (caption)
    w = caption->GetTextWidthPercent();
  const auto children = view->GetChildren();
  const bool control = dynamic_cast<blunted::Gui2Button*>(view) ||
                       dynamic_cast<blunted::Gui2Slider*>(view) ||
                       dynamic_cast<blunted::Gui2EditLine*>(view) ||
                       dynamic_cast<blunted::Gui2Pulldown*>(view);
  if ((caption || control) &&
      (x < -0.1f || y < -0.1f || x + w > 100.1f || y + h > 100.1f)) {
    std::printf("[menu-layout] OUTSIDE %s x=%.2f y=%.2f w=%.2f h=%.2f\n",
                view->GetName().c_str(), x, y, w, h);
  }
  for (auto* child : children)
    AuditMenuLayout(child);
}

inline int StandaloneMenuSmokePage(const std::string& route) {
  struct Route { const char* name; int page; };
  static const Route routes[] = {
    {"settings", e_PageID_Settings}, {"gameplay", e_PageID_Gameplay},
    {"controller", e_PageID_Controller}, {"keyboard", e_PageID_Keyboard},
    {"gamepads", e_PageID_Gamepads}, {"gamepad_setup", e_PageID_GamepadSetup},
    {"gamepad_calibration", e_PageID_GamepadCalibration},
    {"gamepad_mapping", e_PageID_GamepadMapping}, {"gamepad_function", e_PageID_GamepadFunction},
    {"graphics", e_PageID_Graphics}, {"audio", e_PageID_Audio},
    {"language", e_PageID_Language}, {"credits", e_PageID_Credits},
    {"match_options", e_PageID_MatchOptions}, {"forfeit", e_PageID_PreQuit},
    {"history", e_PageID_MatchHistory}, {"career", e_PageID_CareerMenu},
    {"career_new", e_PageID_CareerNewGame}, {"career_save", e_PageID_CareerSave}
  };
  for (const auto& entry : routes)
    if (route == entry.name)
      return entry.page;
  return -1;
}

inline bool SmokeMenuWidgets(blunted::Gui2WindowManager* manager, blunted::Gui2View* parent) {
  using namespace blunted;
  bool positive = false, negative = false;
  auto* dialog = new Gui2Dialog(manager, "audit_dialog", 25, 35, 50, 25,
                                "A long confirmation title must remain inside its dialog");
  parent->AddView(dialog);
  auto* body = new Gui2Text(manager, "audit_body", 0, 0, 90, 70, 2.5f, 10, "");
  body->AddText("averylongunbrokenword");
  body->AddText("Second line");
  dialog->AddContent(body);
  auto* cancel = dialog->AddPosNegButtons("Confirm", "Cancel");
  dialog->sig_OnPositive.connect([&](auto*) { positive = true; });
  dialog->sig_OnNegative.connect([&](auto*) { negative = true; });
  dialog->Show();
  cancel->SetFocus();
  bool ok = cancel->IsVisible() && cancel->GetCaption() == "Cancel";
  float bodyWidth, bodyHeight;
  body->GetSize(bodyWidth, bodyHeight);
  ok = ok && bodyWidth <= 48 && bodyHeight <= 14 && body->GetChildren().size() == 3;
  AuditMenuLayout(dialog);
  for (int i = 0; i < 50; ++i)
    dialog->Process();
  WindowingEvent left;
  left.SetDirection(Vector3(-1, 0, 0));
  cancel->ProcessEvent(&left);
  auto* confirm = dynamic_cast<Gui2Button*>(manager->GetFocus());
  ok = ok && confirm && confirm->GetCaption() == "Confirm";
  WindowingEvent escape;
  escape.SetEscape();
  manager->GetFocus()->ProcessEvent(&escape);
  ok = ok && negative && !positive && escape.IsAccepted();
  dialog->Exit();
  delete dialog;

  auto* detail = new Gui2Dialog(manager, "audit_detail", 25, 35, 50, 25, "Details");
  parent->AddView(detail);
  auto* close = detail->AddPosNegButtons("Close", "Delete", false);
  positive = negative = false;
  detail->sig_OnPositive.connect([&](auto*) { positive = true; });
  detail->sig_OnNegative.connect([&](auto*) { negative = true; });
  detail->Show();
  close->SetFocus();
  WindowingEvent closeEvent;
  closeEvent.SetEscape();
  close->ProcessEvent(&closeEvent);
  ok = ok && positive && !negative && close->GetCaption() == "Close";
  detail->Exit();
  delete detail;

  auto* readOnly = new Gui2Grid(manager, "audit_readonly", 5, 5, 30, 10);
  parent->AddView(readOnly);
  readOnly->SetReadOnlyScrolling(true);
  readOnly->SetMaxVisibleRows(2);
  for (int i = 0; i < 4; ++i)
    readOnly->AddView(new Gui2Caption(manager, "audit_row" + std::to_string(i),
                                      0, 0, 28, 3, "Row " + std::to_string(i)), i, 0);
  readOnly->UpdateLayout();
  readOnly->Show();
  readOnly->FindView(3, 0)->SetFocus();
  ok = ok && readOnly->FindView(3, 0)->IsVisible() && !readOnly->FindView(0, 0)->IsVisible();
  for (int i = 0; i < 50; ++i)
    readOnly->Process();
  WindowingEvent up;
  up.SetDirection(Vector3(0, -1, 0));
  manager->GetFocus()->ProcessEvent(&up);
  ok = ok && manager->GetFocus() == readOnly->FindView(2, 0);
  readOnly->Exit();
  delete readOnly;

  auto* caption = new Gui2Caption(manager, "audit_long_label", 5, 5, 20, 3,
                                  "Long localized label: caf\xc3\xa9, passing and shooting assistance");
  parent->AddView(caption);
  caption->Show();
  float width, height;
  caption->GetSize(width, height);
  ok = ok && width == 20.0f && caption->GetTextWidthPercent() <= 20.1f;
  caption->SetCaption("Short");
  caption->GetSize(width, height);
  ok = ok && width == 20.0f && caption->GetCaption() == "Short";
  caption->Exit();
  delete caption;

  auto* popup = new Gui2Pulldown(manager, "audit_popup", 78, 94, 20, 3);
  parent->AddView(popup);
  popup->Show();
  popup->PullDownOrUp(); // Empty lists must not throw.
  for (int i = 0; i < 8; ++i)
    popup->AddEntry("Choice " + std::to_string(i), std::to_string(i));
  popup->SetSelected(7);
  popup->PullDownOrUp();
  ok = ok && manager->GetFocus()->IsVisible();
  AuditMenuLayout(popup);
  popup->PullDownOrUp();
  popup->Exit();
  delete popup;
  return ok;
}

#endif
