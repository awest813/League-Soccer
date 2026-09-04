#include "career_save_page.hpp"

#include "../pagefactory.hpp"
#include "career_common.hpp"
#include "utils/gui2/widgets/dialog.hpp"
#include "utils/localization.hpp"

namespace blunted {

namespace {

std::string ModeToString(CareerMode mode) {
  switch (mode) {
    case CareerMode::PLAYER:
      return TR("career_mode_player");
    case CareerMode::COACH:
      return TR("career_mode_coach");
    case CareerMode::GM:
      return TR("career_mode_gm");
    case CareerMode::OWNER:
      return TR("career_mode_owner");
    case CareerMode::MANAGER:
    default:
      return TR("career_mode_manager");
  }
}

}  // namespace

CareerSavePage::CareerSavePage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData), slotsGrid(nullptr), feedbackCaption(nullptr) {
  m_fromMenu =
      pageData.properties ? (pageData.properties->Get("fromMenu", "false") == "true") : false;

  frame = new Gui2Frame(windowManager, "frame_career_save", 8, 4, 84, 92, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_career_save_title", 2, 2, 80, 3,
                                       TR("career_save_title"));
  title->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  frame->AddView(title);
  title->Show();

  Gui2Caption* info = new Gui2Caption(windowManager, "caption_career_save_info", 2, 5.5f, 80, 2.5f,
                                      TR("career_save_info"));
  frame->AddView(info);
  info->Show();

  CareerSave* activeSave = CareerDatabase::GetInstance().GetActiveSave();
  Gui2Button* btnSaveActive =
      new Gui2Button(windowManager, "btn_save_active_career", 2, 8.5f, 44, 3,
                     activeSave ? (TR("career_save_active_now") + " (" + activeSave->name + ")")
                                : TR("career_save_no_active"));
  btnSaveActive->SetActive(activeSave != nullptr);
  btnSaveActive->sig_OnClick.connect([this](...) {
    if (CareerDatabase::GetInstance().SaveCareerData()) {
      SetFeedback(TR("career_save_success"));
      RefreshSlots();
    } else {
      SetFeedback(TR("career_save_failed"));
    }
  });
  frame->AddView(btnSaveActive);
  btnSaveActive->Show();

  bool hasAutoSave = CareerDatabase::GetInstance().HasSaveSlot(-1);
  Gui2Button* btnRestoreAutoSave = new Gui2Button(windowManager, "btn_restore_autosave", 48, 8.5f,
                                                  34, 3, TR("career_load_autosave_btn"));
  btnRestoreAutoSave->SetActive(hasAutoSave);
  btnRestoreAutoSave->sig_OnClick.connect([this](...) {
    Gui2Dialog* dlg = new Gui2Dialog(this->windowManager, "dialog_restore_autosave", 20, 30, 60, 30,
                                     TR("career_load_autosave_confirm"));
    (dlg->AddPosNegButtons(TR("league_yes"), TR("league_no")))->SetFocus();
    dlg->sig_OnPositive.connect([this, dlg](...) {
      dlg->Exit();
      delete dlg;
      if (CareerDatabase::GetInstance().LoadCareerSlot(-1)) {
        SetFeedback(TR("career_load_autosave_success"));
        CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
        const int hubPage = (save && save->mode == CareerMode::OWNER) ? (int)e_PageID_OwnerHub
                                                                      : (int)e_PageID_CareerHub;
        Properties props;
        CreatePage(hubPage, props);
      } else {
        SetFeedback(TR("career_load_autosave_failed"));
      }
    });
    dlg->sig_OnNegative.connect([this, dlg](...) {
      dlg->Exit();
      delete dlg;
    });
    this->AddView(dlg);
    dlg->Show();
  });
  frame->AddView(btnRestoreAutoSave);
  btnRestoreAutoSave->Show();

  feedbackCaption =
      new Gui2Caption(windowManager, "caption_career_save_feedback", 2, 84, 80, 2.5f, "");
  feedbackCaption->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright1));
  frame->AddView(feedbackCaption);
  feedbackCaption->Show();

  RefreshSlots();

  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_career_save_back", 2, 87, 80, 3, TR("action_back"));
  btnBack->sig_OnClick.connect([this](...) { GoBack(); });
  frame->AddView(btnBack);
  btnBack->Show();

  btnBack->SetFocus();
  this->Show();
}

CareerSavePage::~CareerSavePage() {}

void CareerSavePage::SetFeedback(const std::string& message) {
  if (feedbackCaption) {
    feedbackCaption->SetCaption(message);
  }
}

void CareerSavePage::RefreshSlots() {
  if (slotsGrid) {
    slotsGrid->Exit();
    delete slotsGrid;
    slotsGrid = nullptr;
  }

  slotsGrid = new Gui2Grid(windowManager, "grid_career_save_slots", 2, 12.5f, 80, 71);
  CareerSave* activeSave = CareerDatabase::GetInstance().GetActiveSave();

  int row = 0;
  for (int slot = 1; slot <= 5; slot++) {
    CareerPersistence::CareerSaveSummary summary;
    bool hasSave = CareerDatabase::GetInstance().GetSlotSummary(slot, summary) && summary.isValid;

    std::string slotLabel;
    if (hasSave) {
      slotLabel = "SLOT " + std::to_string(slot) + ": " + summary.clubName + " (" +
                  ModeToString(summary.mode) + ")\n" + "Season " + std::to_string(summary.season) +
                  " (Week " + std::to_string(summary.week) +
                  ") | Trust: " + std::to_string(summary.boardConfidence) +
                  "% | Budget: " + FormatCareerMoney(summary.transferBudget);
      if (!summary.timestamp.empty()) {
        slotLabel += " | " + summary.timestamp;
      }
    } else {
      slotLabel = "SLOT " + std::to_string(slot) + ": [" + TR("career_slot_empty") + "]";
    }

    Gui2Caption* slotCap = new Gui2Caption(windowManager, "cap_slot_" + std::to_string(slot), 0, 0,
                                           52, 4.5f, slotLabel);
    slotCap->SetColor(hasSave ? windowManager->GetStyle()->GetColor(e_DecorationType_Bright1)
                              : windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
    slotsGrid->AddView(slotCap, row, 0);

    Gui2Button* btnSave = new Gui2Button(windowManager, "btn_slot_save_" + std::to_string(slot), 0,
                                         0, 8, 3.5f, TR("career_save_btn"));
    btnSave->SetActive(activeSave != nullptr);
    btnSave->sig_OnClick.connect([this, slot](...) { OnSaveSlot(slot); });
    slotsGrid->AddView(btnSave, row, 1);

    Gui2Button* btnLoad = new Gui2Button(windowManager, "btn_slot_load_" + std::to_string(slot), 0,
                                         0, 8, 3.5f, TR("career_load_btn"));
    btnLoad->SetActive(hasSave);
    btnLoad->sig_OnClick.connect([this, slot](...) { OnLoadSlot(slot); });
    slotsGrid->AddView(btnLoad, row, 2);

    Gui2Button* btnDelete = new Gui2Button(windowManager, "btn_slot_del_" + std::to_string(slot), 0,
                                           0, 8, 3.5f, TR("career_delete_btn"));
    btnDelete->SetActive(hasSave);
    btnDelete->sig_OnClick.connect([this, slot](...) { OnDeleteSlot(slot); });
    slotsGrid->AddView(btnDelete, row, 3);

    row++;
  }

  slotsGrid->UpdateLayout(0.5f, 0.5f, 0.5f, 0.5f);
  frame->AddView(slotsGrid);
  slotsGrid->Show();
}

void CareerSavePage::OnSaveSlot(int slotIndex) {
  CareerPersistence::CareerSaveSummary summary;
  bool occupied =
      CareerDatabase::GetInstance().GetSlotSummary(slotIndex, summary) && summary.isValid;
  if (occupied) {
    Gui2Dialog* dlg = new Gui2Dialog(this->windowManager, "dialog_career_slot_overwrite", 20, 30,
                                     60, 30, TR("career_slot_overwrite_confirm"));
    (dlg->AddPosNegButtons(TR("league_yes"), TR("league_no")))->SetFocus();
    dlg->sig_OnPositive.connect([this, dlg, slotIndex](...) {
      dlg->Exit();
      delete dlg;
      if (CareerDatabase::GetInstance().SaveCareerSlot(slotIndex)) {
        SetFeedback(TRF("career_save_slot_success", {std::to_string(slotIndex)}));
        RefreshSlots();
      } else {
        SetFeedback(TR("career_save_failed"));
      }
    });
    dlg->sig_OnNegative.connect([this, dlg](...) {
      dlg->Exit();
      delete dlg;
    });
    this->AddView(dlg);
    dlg->Show();
  } else {
    if (CareerDatabase::GetInstance().SaveCareerSlot(slotIndex)) {
      SetFeedback(TRF("career_save_slot_success", {std::to_string(slotIndex)}));
      RefreshSlots();
    } else {
      SetFeedback(TR("career_save_failed"));
    }
  }
}

void CareerSavePage::OnLoadSlot(int slotIndex) {
  CareerSave* activeSave = CareerDatabase::GetInstance().GetActiveSave();
  if (activeSave) {
    Gui2Dialog* dlg = new Gui2Dialog(this->windowManager, "dialog_career_slot_load", 20, 30, 60, 30,
                                     TR("career_slot_load_confirm"));
    (dlg->AddPosNegButtons(TR("league_yes"), TR("league_no")))->SetFocus();
    dlg->sig_OnPositive.connect([this, dlg, slotIndex](...) {
      dlg->Exit();
      delete dlg;
      PerformLoadSlot(slotIndex);
    });
    dlg->sig_OnNegative.connect([this, dlg](...) {
      dlg->Exit();
      delete dlg;
    });
    this->AddView(dlg);
    dlg->Show();
  } else {
    PerformLoadSlot(slotIndex);
  }
}

void CareerSavePage::PerformLoadSlot(int slotIndex) {
  if (CareerDatabase::GetInstance().LoadCareerSlot(slotIndex)) {
    SetFeedback(TRF("career_load_slot_success", {std::to_string(slotIndex)}));
    CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
    const int hubPage = (save && save->mode == CareerMode::OWNER) ? (int)e_PageID_OwnerHub
                                                                  : (int)e_PageID_CareerHub;
    Properties props;
    CreatePage(hubPage, props);
  } else {
    SetFeedback(TR("career_load_failed"));
  }
}

void CareerSavePage::OnDeleteSlot(int slotIndex) {
  Gui2Dialog* dlg = new Gui2Dialog(this->windowManager, "dialog_career_slot_delete", 20, 30, 60, 30,
                                   TR("career_slot_delete_confirm"));
  (dlg->AddPosNegButtons(TR("league_yes"), TR("league_no")))->SetFocus();
  dlg->sig_OnPositive.connect([this, dlg, slotIndex](...) {
    dlg->Exit();
    delete dlg;
    if (CareerDatabase::GetInstance().DeleteCareerSlot(slotIndex)) {
      SetFeedback(TRF("career_delete_slot_success", {std::to_string(slotIndex)}));
      RefreshSlots();
    } else {
      SetFeedback(TR("career_delete_failed"));
    }
  });
  dlg->sig_OnNegative.connect([this, dlg](...) {
    dlg->Exit();
    delete dlg;
  });
  this->AddView(dlg);
  dlg->Show();
}

void CareerSavePage::GoBack() {
  if (m_fromMenu) {
    CreatePage(e_PageID_CareerMenu);
  } else {
    CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
    const int hubPage = (save && save->mode == CareerMode::OWNER) ? (int)e_PageID_OwnerHub
                                                                  : (int)e_PageID_CareerHub;
    Properties props;
    CreatePage(hubPage, props);
  }
}

}  // namespace blunted
