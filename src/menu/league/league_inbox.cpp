#include "league_inbox.hpp"

#include "../../main.hpp"
#include "menu_smoke.hpp"
#include "../pagefactory.hpp"
#include "base/utils.hpp"

LeagueInboxPage::LeagueInboxPage(Gui2WindowManager* windowManager,
                                 const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData),
      frame(nullptr),
      messageGrid(nullptr),
      countCaption(nullptr),
      pageCreatedTime_ms(league_menu_smoke::Now_ms()),
      autoAdvanceTriggered(false) {
  frame = new Gui2Frame(windowManager, "frame_league_inbox", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_league_inbox", 2, 2, 66, 3, "Inbox");
  frame->AddView(title);
  title->Show();

  countCaption =
      new Gui2Caption(windowManager, "caption_inbox_count", 2, 6, 66, 2, "");
  frame->AddView(countCaption);
  countCaption->Show();

  RefreshMessages();

  Gui2Button* btnBack = new Gui2Button(windowManager, "btn_inbox_back", 25, 92, 50, 3, "Back to Dashboard");
  btnBack->sig_OnClick.connect([this](...) { GoBack(); });
  frame->AddView(btnBack);
  btnBack->Show();

  btnBack->SetFocus();

  this->Show();
}

LeagueInboxPage::~LeagueInboxPage() {}

void LeagueInboxPage::RefreshMessages() {
  if (messageGrid) {
    messageGrid->Exit();
    delete messageGrid;
    messageGrid = nullptr;
  }

  auto result = GetDB()->Query(
      "SELECT id, sender, subject, timestamp, read FROM inbox_messages ORDER BY id DESC LIMIT 20");

  int total = 0;
  auto countResult = GetDB()->Query("SELECT COUNT(*) FROM inbox_messages WHERE read = 0");
  if (!countResult->data.empty()) {
    total = atoi(countResult->data.at(0).at(0).c_str());
  }
  countCaption->SetCaption(std::to_string(total) + " unread message" + (total != 1 ? "s" : ""));

  messageGrid = new Gui2Grid(windowManager, "grid_inbox", 2, 9, 66, 78);

  if (result->data.empty()) {
    Gui2Caption* emptyCap = new Gui2Caption(windowManager, "caption_inbox_empty", 0, 0, 86, 3,
                                            "No messages yet. Messages will appear as you play matches.");
    messageGrid->AddView(emptyCap, 0, 0);
  } else {
    int row = 0;
    for (const auto& r : result->data) {
      std::string msgID = r.at(0);
      std::string sender = r.at(1);
      std::string subject = r.at(2);
      std::string timestamp = r.at(3);
      int isRead = atoi(r.at(4).c_str());
      std::string prefix = isRead ? "  " : "* ";
      std::string label = prefix + "[" + timestamp.substr(0, 10) + "] " + sender + ": " + subject;

      Gui2Button* btn = new Gui2Button(windowManager, "btn_msg_" + msgID, 0, 0, 86, 2.5, label);
      btn->sig_OnClick.connect([this, msgID, sender, subject, timestamp](...) {
        GetDB()->Query("UPDATE inbox_messages SET read = 1 WHERE id = " + msgID);

        auto bodyResult = GetDB()->Query(
            "SELECT body FROM inbox_messages WHERE id = " + msgID);
        std::string body = "No content.";
        if (!bodyResult->data.empty() && !bodyResult->data.at(0).at(0).empty()) {
          body = bodyResult->data.at(0).at(0);
        }

        Gui2Dialog* dlg = new Gui2Dialog(windowManager, "dialog_msg_" + msgID, 20, 15, 60, 70, subject);
        Gui2Text* txt = new Gui2Text(windowManager, "text_msg_" + msgID, 5, 5, 90, 70, 2.5, 50, "");
        txt->AddText("From: " + sender);
        txt->AddText("Date: " + timestamp);
        txt->AddEmptyLine();
        txt->AddText(body);
        dlg->AddContent(txt);

        (dlg->AddSingleButton("Close"))->SetFocus();
        dlg->sig_OnPositive.connect([this, dlg](...) {
          dlg->Exit();
          delete dlg;
          RefreshMessages();
        });
        this->AddView(dlg);
        dlg->Show();
      });
      messageGrid->AddView(btn, row++, 0);
    }
  }

  messageGrid->UpdateLayout(0.5);
  frame->AddView(messageGrid);
  messageGrid->Show();
}

void LeagueInboxPage::DeleteMessage(int msgID) {
  GetDB()->Query("DELETE FROM inbox_messages WHERE id = " + std::to_string(msgID));
  RefreshMessages();
}

void LeagueInboxPage::GoBack() {
  this->Exit();
  Properties properties;
  windowManager->GetPageFactory()->CreatePage(static_cast<int>(e_PageID_League_Forward), properties, 0);
  delete this;
}

void LeagueInboxPage::Process() {
  Gui2Page::Process();

  if (!league_menu_smoke::RouteEnabled("inbox") || autoAdvanceTriggered ||
      league_menu_smoke::Now_ms() <
          pageCreatedTime_ms + league_menu_smoke::kQuitDelay_ms) {
    return;
  }

  autoAdvanceTriggered = true;
  printf("[menu-smoke] League inbox reached successfully\n");
  GetMenuTask()->QuitGame();
}
