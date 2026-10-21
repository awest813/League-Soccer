#ifndef GF_MENU_LAYOUT_AUDIT_HPP
#define GF_MENU_LAYOUT_AUDIT_HPP

#include <cstdio>

#include "pagefactory.hpp"
#include "career/career_database.hpp"
#include "career/career_hub_model.hpp"
#include "utils/gui2/widgets/caption.hpp"
#include "utils/gui2/widgets/dialog.hpp"
#include "utils/gui2/widgets/editline.hpp"
#include "utils/gui2/widgets/pulldown.hpp"
#include "utils/gui2/widgets/slider.hpp"
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
  const bool control =
      dynamic_cast<blunted::Gui2Button*>(view) || dynamic_cast<blunted::Gui2Slider*>(view) ||
      dynamic_cast<blunted::Gui2EditLine*>(view) || dynamic_cast<blunted::Gui2Pulldown*>(view);
  if ((caption || control) && (x < -0.1f || y < -0.1f || x + w > 100.1f || y + h > 100.1f)) {
    std::printf("[menu-layout] OUTSIDE %s x=%.2f y=%.2f w=%.2f h=%.2f\n", view->GetName().c_str(),
                x, y, w, h);
  }
  for (auto* child : children)
    AuditMenuLayout(child);
}

inline int StandaloneMenuSmokePage(const std::string& route) {
  struct Route {
    const char* name;
    int page;
  };
  static const Route routes[] = {{"settings", e_PageID_Settings},
                                 {"gameplay", e_PageID_Gameplay},
                                 {"controller", e_PageID_Controller},
                                 {"keyboard", e_PageID_Keyboard},
                                 {"gamepads", e_PageID_Gamepads},
                                 {"gamepad_setup", e_PageID_GamepadSetup},
                                 {"gamepad_setup_virtual", e_PageID_GamepadSetup},
                                 {"gamepad_calibration", e_PageID_GamepadCalibration},
                                 {"gamepad_mapping", e_PageID_GamepadMapping},
                                 {"gamepad_function", e_PageID_GamepadFunction},
                                 {"graphics", e_PageID_Graphics},
                                 {"audio", e_PageID_Audio},
                                 {"language", e_PageID_Language},
                                 {"credits", e_PageID_Credits},
                                 {"match_options", e_PageID_MatchOptions},
                                 {"forfeit", e_PageID_PreQuit},
                                 {"history", e_PageID_MatchHistory},
                                 {"career", e_PageID_CareerMenu},
                                 {"career_training", e_PageID_CareerTraining},
                                 {"career_tactics", e_PageID_CareerStrategy},
                                 {"career_tactics_delegated", e_PageID_CareerStrategy},
                                 {"career_press", e_PageID_CareerPressConference},
                                 {"career_roster", e_PageID_CareerSquadRoster},
                                 {"career_roster_player", e_PageID_CareerSquadRoster},
                                 {"career_new", e_PageID_CareerNewGame},
                                 {"career_owner_gm", e_PageID_CareerHub},
                                 {"career_owner_legacy", e_PageID_OwnerHub},
                                 {"career_owner_delegated", e_PageID_CareerHub},
                                 {"career_player_missing", e_PageID_CareerHub},
                                 {"career_player_training", e_PageID_CareerTraining},
                                 {"career_coach", e_PageID_CareerHub},
                                 {"career_player", e_PageID_CareerHub},
                                 {"career_season", e_PageID_CareerSeason},
                                 {"career_season_pending", e_PageID_CareerSeason},
                                 {"career_matchday", e_PageID_CareerMatchday},
                                 {"career_save", e_PageID_CareerSave}};
  for (const auto& entry : routes)
    if (route == entry.name)
      return entry.page;
  return -1;
}

inline blunted::Gui2View* FindCareerHubView(blunted::Gui2View* view, const std::string& name) {
  if (view->GetName() == name) return view;
  for (auto* child : view->GetChildren())
    if (auto* found = FindCareerHubView(child, name)) return found;
  return nullptr;
}

inline bool SmokeCareerHub(blunted::Gui2WindowManager* manager, blunted::Gui2View* page, int section) {
  using namespace blunted;
  const auto* save = CareerDatabase::GetInstance().GetActiveSave();
  auto* nav = dynamic_cast<Gui2Grid*>(FindCareerHubView(page, "hub_navigation"));
  if (!save || !FindCareerHubView(page, "career_shared_hub") || !nav) return false;
  const char* ids[] = {"hub_continue", "hub_nav_overview", "hub_nav_team", "hub_nav_role",
                       "hub_nav_competitions", "hub_nav_inbox", "hub_save", "hub_exit"};
  if (!manager->GetFocus() || manager->GetFocus()->GetName() != ids[section + 1]) return false;
  nav->SetFocus();
  for (int i = 0; i < 8; ++i) {
    auto* focus = manager->GetFocus();
    if (!focus || focus->GetName() != ids[i] || !focus->IsVisible()) return false;
    for (int tick = 0; tick < 50; ++tick) nav->Process();
    if (i < 7) {
      WindowingEvent down;
      down.SetDirection(Vector3(0, 1, 0));
      focus->ProcessEvent(&down);
    }
  }
  const auto tools = CareerHubTools(*save, static_cast<CareerHubSection>(section));
  auto* actions = dynamic_cast<Gui2Grid*>(FindCareerHubView(page, section == 4 ? "hub_messages" : "hub_actions"));
  if (section > 0 && section < 4 && (!actions || actions->GetChildren().size() != tools.size())) return false;
  for (const auto& tool : tools)
    if (!FindCareerHubView(page, std::string("hub_action_") + tool.id)) return false;
  if (actions && actions->IsSelectable()) {
    WindowingEvent right;
    right.SetDirection(Vector3(1, 0, 0));
    manager->GetFocus()->ProcessEvent(&right);
    if (!actions->IsInFocusPath()) return false;
    actions->SetFocus();
    for (size_t i = 0; i < actions->GetChildren().size(); ++i) {
      auto* child = actions->GetChildren()[i];
      if (!child->IsInFocusPath() || !child->IsVisible()) return false;
      AuditMenuLayout(page);
      for (int tick = 0; tick < 50; ++tick) actions->Process();
      if (i + 1 < actions->GetChildren().size()) {
        WindowingEvent down;
        down.SetDirection(Vector3(0, 1, 0));
        manager->GetFocus()->ProcessEvent(&down);
      }
    }
    WindowingEvent left;
    left.SetDirection(Vector3(-1, 0, 0));
    manager->GetFocus()->ProcessEvent(&left);
    if (!nav->IsInFocusPath()) return false;
  }
  if (section == 4 && !save->inbox.empty()) {
    for (size_t index : {size_t(0), save->inbox.size() - 1}) {
      auto* message = dynamic_cast<Gui2Button*>(FindCareerHubView(page, "hub_message_" + std::to_string(index)));
      if (!message) return false;
      message->SetFocus();
      WindowingEvent activate;
      activate.SetActivate();
      message->ProcessEvent(&activate);
      auto* dialog = FindCareerHubView(page, "hub_inbox_dialog_frame");
      if (!dialog || !dialog->IsInFocusPath() || !save->inbox[index].read) return false;
      AuditMenuLayout(page);
      WindowingEvent escape;
      escape.SetEscape();
      manager->GetFocus()->ProcessEvent(&escape);
      if (FindCareerHubView(page, "hub_inbox_dialog_frame") || manager->GetFocus() != message ||
          !message->IsVisible() || message->GetCaption() != save->inbox[index].subject) return false;
      AuditMenuLayout(page);
    }
  }
  return true;
}

inline bool SmokeCareerRoster(blunted::Gui2WindowManager* manager, blunted::Gui2View* page) {
  using namespace blunted;
  auto* roster = dynamic_cast<Gui2Grid*>(FindCareerHubView(page, "squad_grid"));
  if (!roster || roster->GetChildren().size() != 24) return false;
  roster->SetFocus();
  for (int i = 0; i < 24; ++i) {
    auto* row = roster->FindView(i, 0);
    if (!row || manager->GetFocus() != row || !row->IsVisible()) return false;
    AuditMenuLayout(page);
    for (int tick = 0; tick < 50; ++tick) roster->Process();
    if (i < 23) {
      WindowingEvent down;
      down.SetDirection(Vector3(0, 1, 0));
      row->ProcessEvent(&down);
    }
  }
  auto* previous = manager->GetFocus();
  WindowingEvent activate;
  activate.SetActivate();
  previous->ProcessEvent(&activate);
  auto* profile = FindCareerHubView(page, "cap_player_detail");
  if (!profile || !profile->IsVisible()) return false;
  AuditMenuLayout(page);
  WindowingEvent escape;
  escape.SetEscape();
  manager->GetFocus()->ProcessEvent(&escape);
  if (FindCareerHubView(page, "cap_player_detail") || manager->GetFocus() != previous || !previous->IsVisible()) return false;
  const auto* save = CareerDatabase::GetInstance().GetActiveSave();
  if (save && CanManageClub(*save)) {
    WindowingEvent reopen;
    reopen.SetActivate();
    previous->ProcessEvent(&reopen);
    auto* release = FindCareerHubView(page, "btn_release_action");
    if (!release) return false;
    release->SetFocus();
    WindowingEvent request;
    request.SetActivate();
    release->ProcessEvent(&request);
    if (!FindCareerHubView(page, "dialog_release_player_frame")) return false;
    WindowingEvent cancel;
    cancel.SetEscape();
    manager->GetFocus()->ProcessEvent(&cancel);
    if (FindCareerHubView(page, "dialog_release_player_frame") || manager->GetFocus() != previous ||
        !previous->IsVisible() || save->roster.size() != 24) return false;
  }
  return true;
}

inline bool SmokeCareerTraining(blunted::Gui2WindowManager* manager, blunted::Gui2View* page) {
  using namespace blunted;
  Gui2Grid* players = nullptr;
  for (auto* child : page->GetChildren())
    if (child->GetName() == "development_players")
      players = dynamic_cast<Gui2Grid*>(child);
  if (!players || players->GetChildren().size() != 32) {
    printf("[menu-smoke] Training roster size: %d\n",
           players ? static_cast<int>(players->GetChildren().size()) : -1);
    return false;
  }
  players->SetFocus();
  for (int row = 0; row < 32; ++row) {
    auto* focused = manager->GetFocus();
    auto* card = players->GetChildren().at(row);
    if (!focused || !card->IsInFocusPath() || !card->IsVisible() || !focused->IsVisible()) {
      printf("[menu-smoke] Training row %d focus %s visible %d\n", row,
             focused ? focused->GetName().c_str() : "none", focused ? focused->IsVisible() : false);
      return false;
    }
    AuditMenuLayout(page);
    for (int tick = 0; tick < 50; ++tick)
      players->Process();
    if (row < 31) {
      WindowingEvent down;
      down.SetDirection(Vector3(0, 1, 0));
      focused->ProcessEvent(&down);
    }
  }
  return true;
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
    readOnly->AddView(new Gui2Caption(manager, "audit_row" + std::to_string(i), 0, 0, 28, 3,
                                      "Row " + std::to_string(i)),
                      i, 0);
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

  auto* caption =
      new Gui2Caption(manager, "audit_long_label", 5, 5, 20, 3,
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
  popup->PullDownOrUp();  // Empty lists must not throw.
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
