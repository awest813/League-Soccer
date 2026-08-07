#include "league_team.hpp"

#include <string>

#include "../../main.hpp"
#include "../pagefactory.hpp"
#include "base/utils.hpp"
#include "menu_smoke.hpp"
#include "utils/gui2/widgets/dialog.hpp"
#include "utils/gui2/widgets/text.hpp"
#include "utils/localization.hpp"

LeagueTeamPage::LeagueTeamPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData),
      pageCreatedTime_ms(league_menu_smoke::Now_ms()),
      autoAdvanceTriggered(false) {
  auto teamResult = GetDB()->Query(
      "SELECT t.id, t.name, t.formation_xml, t.tactics_xml "
      "FROM teams t "
      "JOIN settings s ON t.id = s.team_id "
      "LIMIT 1");
  const std::string teamID = teamResult->data.empty() ? "0" : teamResult->data.at(0).at(0);
  const std::string teamName = teamResult->data.empty() ? "Club" : teamResult->data.at(0).at(1);
  const bool hasFormation = !teamResult->data.empty() && teamResult->data.at(0).size() > 2 &&
                            !teamResult->data.at(0).at(2).empty();
  const bool hasTactics = !teamResult->data.empty() && teamResult->data.at(0).size() > 3 &&
                          !teamResult->data.at(0).at(3).empty();

  auto squadResult = GetDB()->Query(
      "SELECT COUNT(*), AVG(base_stat), MIN(age), MAX(age) FROM players WHERE team_id = " + teamID);
  const std::string squadSize = squadResult->data.empty() ? "0" : squadResult->data.at(0).at(0);
  const std::string avgStat = squadResult->data.empty() ? "-" : squadResult->data.at(0).at(1);
  const std::string youngestAge = squadResult->data.empty() ? "-" : squadResult->data.at(0).at(2);
  const std::string oldestAge = squadResult->data.empty() ? "-" : squadResult->data.at(0).at(3);

  Gui2Frame* frame = new Gui2Frame(windowManager, "frame_league_team", 8, 5, 84, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_league_team", 3, 2, 36, 3, "Team Management");
  frame->AddView(title);
  title->Show();

  Gui2Caption* subtitle =
      new Gui2Caption(windowManager, "caption_league_team_subtitle", 3, 6, 36, 3, teamName);
  frame->AddView(subtitle);
  subtitle->Show();

  Gui2Frame* actionPanel = new Gui2Frame(windowManager, "frame_team_actions", 3, 14, 38, 68, true);
  frame->AddView(actionPanel);
  actionPanel->Show();

  Gui2Caption* actionTitle =
      new Gui2Caption(windowManager, "caption_team_actions", 2, 2, 32, 2, "Squad Tools");
  actionPanel->AddView(actionTitle);
  actionTitle->Show();

  Gui2Button* btnFormation =
      new Gui2Button(windowManager, "btn_team_formation", 0, 0, 60, 3, "Formation");
  Gui2Button* btnPlayerSel =
      new Gui2Button(windowManager, "btn_team_playersel", 0, 0, 60, 3, "Player Selection");
  Gui2Button* btnTactics =
      new Gui2Button(windowManager, "btn_team_tactics", 0, 0, 60, 3, "Tactics");
  Gui2Button* btnPlayerOvr =
      new Gui2Button(windowManager, "btn_team_playerovr", 0, 0, 60, 3, "Player Overview");
  Gui2Button* btnPlayerDev =
      new Gui2Button(windowManager, "btn_team_playerdev", 0, 0, 60, 3, "Player Development");
  Gui2Button* btnSetup = new Gui2Button(windowManager, "btn_team_setup", 0, 0, 60, 3, "Team Setup");
  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_team_back", 0, 0, 60, 3, "Back to Dashboard");

  btnFormation->sig_OnClick.connect([this](...) { GoPage(e_PageID_League_Team_Formation); });
  btnPlayerSel->sig_OnClick.connect([this](...) { GoPage(e_PageID_League_Team_PlayerSelection); });
  btnTactics->sig_OnClick.connect([this](...) { GoPage(e_PageID_League_Team_Tactics); });
  btnPlayerOvr->sig_OnClick.connect([this](...) { GoPage(e_PageID_League_Team_PlayerOverview); });
  btnPlayerDev->sig_OnClick.connect(
      [this](...) { GoPage(e_PageID_League_Team_PlayerDevelopment); });
  btnSetup->sig_OnClick.connect([this](...) { GoPage(e_PageID_League_Team_Setup); });
  btnBack->sig_OnClick.connect([this](...) { GoPage(e_PageID_League_Forward); });

  Gui2Grid* grid = new Gui2Grid(windowManager, "grid_team", 2, 8, 32, 54);
  grid->AddView(btnFormation, 0, 0);
  grid->AddView(btnPlayerSel, 1, 0);
  grid->AddView(btnTactics, 2, 0);
  grid->AddView(btnPlayerOvr, 3, 0);
  grid->AddView(btnPlayerDev, 4, 0);
  grid->AddView(btnSetup, 5, 0);
  grid->AddView(btnBack, 6, 0);
  grid->UpdateLayout(0.25, 0.25, 0.3, 0.3);
  actionPanel->AddView(grid);
  grid->Show();

  Gui2Frame* squadPanel = new Gui2Frame(windowManager, "frame_team_summary", 45, 14, 36, 25, true);
  frame->AddView(squadPanel);
  squadPanel->Show();

  Gui2Caption* squadTitle =
      new Gui2Caption(windowManager, "caption_team_summary", 2, 2, 30, 2, "Squad Snapshot");
  squadPanel->AddView(squadTitle);
  squadTitle->Show();

  Gui2Caption* squadBody = new Gui2Caption(windowManager, "caption_team_summary_body", 2, 6, 30, 10,
                                           squadSize + " players\nAverage base stat: " + avgStat +
                                               "\nAge range: " + youngestAge + " to " + oldestAge);
  squadPanel->AddView(squadBody);
  squadBody->Show();

  Gui2Frame* structurePanel =
      new Gui2Frame(windowManager, "frame_team_structure", 45, 43, 36, 18, true);
  frame->AddView(structurePanel);
  structurePanel->Show();

  Gui2Caption* structureTitle =
      new Gui2Caption(windowManager, "caption_team_structure", 2, 2, 30, 2, "Structure");
  structurePanel->AddView(structureTitle);
  structureTitle->Show();

  Gui2Caption* structureBody =
      new Gui2Caption(windowManager, "caption_team_structure_body", 2, 6, 30, 6,
                      std::string("Formation: ") + (hasFormation ? "Configured" : "Needs setup") +
                          "\nTactics: " + (hasTactics ? "Configured" : "Needs setup"));
  structurePanel->AddView(structureBody);
  structureBody->Show();

  Gui2Frame* guidancePanel =
      new Gui2Frame(windowManager, "frame_team_guidance", 45, 65, 36, 17, true);
  frame->AddView(guidancePanel);
  guidancePanel->Show();

  Gui2Caption* guidanceTitle =
      new Gui2Caption(windowManager, "caption_team_guidance", 2, 2, 30, 2, "Recommended Flow");
  guidancePanel->AddView(guidanceTitle);
  guidanceTitle->Show();

  Gui2Caption* guidanceBody =
      new Gui2Caption(windowManager, "caption_team_guidance_body", 2, 6, 30, 6,
                      "Check the squad list, tune formation, then sharpen tactics before kickoff.");
  guidancePanel->AddView(guidanceBody);
  guidanceBody->Show();

  btnFormation->SetFocus();
  this->Show();
}

LeagueTeamPage::~LeagueTeamPage() {}

void LeagueTeamPage::Process() {
  Gui2Page::Process();

  if (!league_menu_smoke::RouteEnabled("team_overview") || autoAdvanceTriggered ||
      league_menu_smoke::Now_ms() < pageCreatedTime_ms + league_menu_smoke::kAdvanceDelay_ms) {
    return;
  }

  autoAdvanceTriggered = true;
  printf("[menu-smoke] Team management opening player overview\n");
  GoPage(e_PageID_League_Team_PlayerOverview);
}

void LeagueTeamPage::GoPage(e_PageID pageID) {
  this->Exit();
  Properties properties;
  windowManager->GetPageFactory()->CreatePage(static_cast<int>(pageID), properties, 0);
  delete this;
}

LeagueTeamFormationPage::LeagueTeamFormationPage(Gui2WindowManager* windowManager,
                                                 const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* frame =
      new Gui2Frame(windowManager, "frame_league_team_formation", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_league_team_formation", 2, 2, 66, 3, "Formation");
  frame->AddView(title);
  title->Show();

  auto result = GetDB()->Query(
      "SELECT t.name, t.formation_xml FROM teams t, settings s WHERE t.id = s.team_id LIMIT 1");
  if (!result->data.empty()) {
    std::string teamName = result->data.at(0).at(0);
    std::string formationXML = result->data.at(0).at(1);
    Gui2Caption* info =
        new Gui2Caption(windowManager, "caption_formation_team", 2, 6, 66, 3, "Team: " + teamName);
    frame->AddView(info);
    info->Show();

    Gui2Text* formationText =
        new Gui2Text(windowManager, "text_formation", 2, 12, 66, 70, 2.5, 60, "");
    if (formationXML.empty()) {
      formationText->AddText(
          "No formation data available. Use the match engine to set a formation.");
    } else {
      formationText->AddText(formationXML);
    }
    frame->AddView(formationText);
    formationText->Show();
  }

  Gui2Button* btnBack = new Gui2Button(windowManager, "btn_formation_back", 30, 90, 40, 3,
                                       Localization::GetInstance().Translate("action_back"));
  btnBack->sig_OnClick.connect([this, windowManager](...) {
    this->Exit();
    Properties properties;
    windowManager->GetPageFactory()->CreatePage(static_cast<int>(e_PageID_League_Team), properties,
                                                0);
    delete this;
  });
  frame->AddView(btnBack);
  btnBack->Show();
  btnBack->SetFocus();
  this->Show();
}

LeagueTeamFormationPage::~LeagueTeamFormationPage() {}

LeagueTeamPlayerSelectionPage::LeagueTeamPlayerSelectionPage(Gui2WindowManager* windowManager,
                                                             const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* frame =
      new Gui2Frame(windowManager, "frame_league_team_playersel", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_league_team_playerselection", 2, 2,
                                       66, 3, "Player Selection");
  frame->AddView(title);
  title->Show();

  auto result = GetDB()->Query(
      "SELECT p.id, p.firstname, p.lastname, p.role FROM players p, teams t, settings s "
      "WHERE p.team_id = t.id AND t.id = s.team_id ORDER BY p.formationorder");
  Gui2Grid* grid = new Gui2Grid(windowManager, "grid_playersel", 2, 8, 66, 80);
  int row = 0;
  if (!result->data.empty()) {
    for (const auto& r : result->data) {
      std::string playerID = r.at(0);
      std::string label = r.at(1) + " " + r.at(2) + " (" + r.at(3) + ")";
      Gui2Button* btn =
          new Gui2Button(windowManager, "btn_player_" + r.at(0), 0, 0, 86, 2.5, label);
      btn->sig_OnClick.connect([this, windowManager, playerID, label](...) {
        auto detail = GetDB()->Query(
            "SELECT firstname, lastname, role, age, base_stat FROM players WHERE id = " + playerID);
        Gui2Dialog* dlg =
            new Gui2Dialog(windowManager, "dialog_player_detail", 25, 20, 50, 60, label);
        if (!detail->data.empty()) {
          Gui2Text* txt =
              new Gui2Text(windowManager, "text_player_detail", 5, 5, 90, 80, 2.5, 40, "");
          const auto& d = detail->data.at(0);
          txt->AddText("Name: " + d.at(0) + " " + d.at(1));
          txt->AddText("Role: " + d.at(2));
          txt->AddText("Age: " + d.at(3));
          txt->AddText("Base Stat: " + d.at(4));
          dlg->AddContent(txt);
        }
        (dlg->AddSingleButton("Close"))->SetFocus();
        dlg->sig_OnPositive.connect([this, dlg](...) {
          dlg->Exit();
          delete dlg;
        });
        this->AddView(dlg);
        dlg->Show();
      });
      grid->AddView(btn, row++, 0);
    }
  } else {
    Gui2Caption* emptyCap = new Gui2Caption(windowManager, "caption_playersel_empty", 0, 0, 86, 3,
                                            "No players in the squad.");
    grid->AddView(emptyCap, row++, 0);
  }

  // Back lives in the same grid so keyboard/gamepad can reach it too.
  Gui2Button* btnBack = new Gui2Button(windowManager, "btn_playersel_back", 0, 0, 86, 2.5,
                                       Localization::GetInstance().Translate("action_back"));
  btnBack->sig_OnClick.connect([this, windowManager](...) {
    this->Exit();
    Properties properties;
    windowManager->GetPageFactory()->CreatePage(static_cast<int>(e_PageID_League_Team), properties,
                                                0);
    delete this;
  });
  grid->AddView(btnBack, row, 0);

  grid->UpdateLayout(0.5);
  frame->AddView(grid);
  grid->Show();

  if (grid->IsSelectable()) {
    grid->SetFocus();
  }

  this->Show();
}

LeagueTeamPlayerSelectionPage::~LeagueTeamPlayerSelectionPage() {}

LeagueTeamTacticsPage::LeagueTeamTacticsPage(Gui2WindowManager* windowManager,
                                             const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* frame = new Gui2Frame(windowManager, "frame_league_team_tactics", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_league_team_tactics", 2, 2, 66, 3, "Team Tactics");
  frame->AddView(title);
  title->Show();

  auto result = GetDB()->Query(
      "SELECT t.name, t.tactics_xml FROM teams t, settings s WHERE t.id = s.team_id LIMIT 1");
  if (!result->data.empty()) {
    std::string teamName = result->data.at(0).at(0);
    std::string tacticsXML = result->data.at(0).at(1);
    Gui2Caption* info =
        new Gui2Caption(windowManager, "caption_tactics_team", 2, 6, 66, 3, "Team: " + teamName);
    frame->AddView(info);
    info->Show();

    Gui2Text* tacticsText = new Gui2Text(windowManager, "text_tactics", 2, 12, 66, 70, 2.5, 60, "");
    if (tacticsXML.empty()) {
      tacticsText->AddText("No tactics data available. Use the match engine to set tactics.");
    } else {
      tacticsText->AddText(tacticsXML);
    }
    frame->AddView(tacticsText);
    tacticsText->Show();
  }

  Gui2Button* btnBack = new Gui2Button(windowManager, "btn_tactics_back", 30, 90, 40, 3,
                                       Localization::GetInstance().Translate("action_back"));
  btnBack->sig_OnClick.connect([this, windowManager](...) {
    this->Exit();
    Properties properties;
    windowManager->GetPageFactory()->CreatePage(static_cast<int>(e_PageID_League_Team), properties,
                                                0);
    delete this;
  });
  frame->AddView(btnBack);
  btnBack->Show();
  btnBack->SetFocus();
  this->Show();
}

LeagueTeamTacticsPage::~LeagueTeamTacticsPage() {}

LeagueTeamPlayerOverviewPage::LeagueTeamPlayerOverviewPage(Gui2WindowManager* windowManager,
                                                           const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData),
      pageCreatedTime_ms(league_menu_smoke::Now_ms()),
      autoAdvanceTriggered(false) {
  Gui2Frame* frame =
      new Gui2Frame(windowManager, "frame_league_team_playerovr", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_league_team_playeroverview", 2, 2,
                                       66, 3, "Player Overview");
  frame->AddView(title);
  title->Show();

  Gui2Caption* header =
      new Gui2Caption(windowManager, "caption_playerovr_header", 2, 6, 66, 2,
                      "Name                  | Role                | Age | Base Stat");
  frame->AddView(header);
  header->Show();

  auto result = GetDB()->Query(
      "SELECT p.firstname, p.lastname, p.role, p.age, p.base_stat FROM players p "
      "JOIN teams t ON p.team_id = t.id JOIN settings s ON t.id = s.team_id "
      "ORDER BY p.formationorder");
  if (!result->data.empty()) {
    Gui2Grid* grid = new Gui2Grid(windowManager, "grid_playerovr", 2, 10, 66, 75);
    int row = 0;
    for (const auto& r : result->data) {
      std::string fullName = r.at(0) + " " + r.at(1);
      char buf[256];
      snprintf(buf, sizeof(buf), "%-20s | %-19s | %2s  | %s", fullName.c_str(), r.at(2).c_str(),
               r.at(3).c_str(), r.at(4).c_str());
      std::string btnLabel(buf);
      Gui2Button* btn =
          new Gui2Button(windowManager, "btn_povr_" + std::to_string(row), 0, 0, 86, 2.5, btnLabel);
      btn->sig_OnClick.connect([this, windowManager, fullName](...) {
        auto detail = GetDB()->Query(
            "SELECT p.firstname, p.lastname, p.role, p.age, p.base_stat, t.name "
            "FROM players p JOIN teams t ON p.team_id = t.id "
            "WHERE p.firstname || ' ' || p.lastname = '" +
            fullName + "' LIMIT 1");
        Gui2Dialog* dlg =
            new Gui2Dialog(windowManager, "dialog_povr_detail", 25, 20, 50, 60, fullName);
        if (!detail->data.empty()) {
          Gui2Text* txt =
              new Gui2Text(windowManager, "text_povr_detail", 5, 5, 90, 80, 2.5, 40, "");
          const auto& d = detail->data.at(0);
          txt->AddText("Name: " + d.at(0) + " " + d.at(1));
          txt->AddText("Role: " + d.at(2));
          txt->AddText("Age: " + d.at(3));
          txt->AddText("Base Stat: " + d.at(4));
          txt->AddText("Team: " + d.at(5));
          dlg->AddContent(txt);
        }
        (dlg->AddSingleButton("Close"))->SetFocus();
        dlg->sig_OnPositive.connect([this, dlg](...) {
          dlg->Exit();
          delete dlg;
        });
        this->AddView(dlg);
        dlg->Show();
      });
      grid->AddView(btn, row++, 0);
    }
    grid->UpdateLayout(0.5);
    frame->AddView(grid);
    grid->Show();
  }

  Gui2Button* btnBack = new Gui2Button(windowManager, "btn_playerovr_back", 30, 90, 40, 3,
                                       Localization::GetInstance().Translate("action_back"));
  btnBack->sig_OnClick.connect([this, windowManager](...) {
    this->Exit();
    Properties properties;
    windowManager->GetPageFactory()->CreatePage(static_cast<int>(e_PageID_League_Team), properties,
                                                0);
    delete this;
  });
  frame->AddView(btnBack);
  btnBack->Show();
  btnBack->SetFocus();
  this->Show();
}

LeagueTeamPlayerOverviewPage::~LeagueTeamPlayerOverviewPage() {}

void LeagueTeamPlayerOverviewPage::Process() {
  Gui2Page::Process();

  if (!league_menu_smoke::RouteEnabled("team_overview") || autoAdvanceTriggered ||
      league_menu_smoke::Now_ms() < pageCreatedTime_ms + league_menu_smoke::kQuitDelay_ms) {
    return;
  }

  autoAdvanceTriggered = true;
  printf("[menu-smoke] Team player overview reached successfully\n");
  GetMenuTask()->QuitGame();
}

LeagueTeamPlayerDevelopmentPage::LeagueTeamPlayerDevelopmentPage(Gui2WindowManager* windowManager,
                                                                 const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* frame =
      new Gui2Frame(windowManager, "frame_league_team_playerdev", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_league_team_playerdevelopment", 2, 2,
                                       66, 3, "Player Development");
  frame->AddView(title);
  title->Show();

  Gui2Caption* header =
      new Gui2Caption(windowManager, "caption_playerdev_header", 2, 6, 66, 2,
                      "Name                  | Role                | Age | Base Stat");
  frame->AddView(header);
  header->Show();

  auto result = GetDB()->Query(
      "SELECT p.id, p.firstname, p.lastname, p.role, p.age, p.base_stat FROM players p "
      "JOIN teams t ON p.team_id = t.id JOIN settings s ON t.id = s.team_id "
      "ORDER BY p.age ASC, p.base_stat DESC");

  if (!result->data.empty()) {
    Gui2Grid* grid = new Gui2Grid(windowManager, "grid_playerdev", 2, 9, 66, 72);
    int row = 0;
    for (const auto& r : result->data) {
      std::string playerID = r.at(0);
      std::string fullName = r.at(1) + " " + r.at(2);
      char buf[256];
      snprintf(buf, sizeof(buf), "%-20s | %-19s | %2s | %s", fullName.c_str(), r.at(3).c_str(),
               r.at(4).c_str(), r.at(5).c_str());
      std::string btnLabel(buf);
      Gui2Button* btn =
          new Gui2Button(windowManager, "btn_pdev_" + std::to_string(row), 0, 0, 86, 2.5, btnLabel);
      btn->sig_OnClick.connect([this, windowManager, playerID, fullName](...) {
        auto detail = GetDB()->Query(
            "SELECT p.firstname, p.lastname, p.role, p.age, p.base_stat FROM players "
            "WHERE id = " +
            playerID);
        Gui2Dialog* dlg =
            new Gui2Dialog(windowManager, "dialog_pdev_" + playerID, 20, 15, 60, 70, fullName);
        if (!detail->data.empty()) {
          Gui2Text* txt =
              new Gui2Text(windowManager, "text_pdev_" + playerID, 5, 5, 90, 70, 2.5, 40, "");
          const auto& d = detail->data.at(0);
          int age = atoi(d.at(3).c_str());
          txt->AddText("Name: " + d.at(0) + " " + d.at(1));
          txt->AddText("Role: " + d.at(2));
          txt->AddText("Age: " + d.at(3));
          txt->AddText("Base Stat: " + d.at(4));
          txt->AddEmptyLine();
          if (age < 24) {
            txt->AddText("Growth potential: High - young players develop faster.");
          } else if (age < 30) {
            txt->AddText("Growth potential: Moderate - prime years.");
          } else {
            txt->AddText("Growth potential: Low - likely to decline.");
          }
          txt->AddEmptyLine();
          txt->AddText("Training programs will be available in a future update.");
          dlg->AddContent(txt);
        }
        (dlg->AddSingleButton("Close"))->SetFocus();
        dlg->sig_OnPositive.connect([this, dlg](...) {
          dlg->Exit();
          delete dlg;
        });
        this->AddView(dlg);
        dlg->Show();
      });
      grid->AddView(btn, row++, 0);
    }
    grid->UpdateLayout(0.5);
    frame->AddView(grid);
    grid->Show();
  } else {
    Gui2Caption* info = new Gui2Caption(windowManager, "caption_playerdev_info", 2, 10, 66, 4,
                                        "No players found on your squad.");
    frame->AddView(info);
    info->Show();
  }

  Gui2Button* btnBack = new Gui2Button(windowManager, "btn_playerdev_back", 25, 92, 50, 3,
                                       Localization::GetInstance().Translate("action_back"));
  btnBack->sig_OnClick.connect([this, windowManager](...) {
    this->Exit();
    Properties properties;
    windowManager->GetPageFactory()->CreatePage(static_cast<int>(e_PageID_League_Team), properties,
                                                0);
    delete this;
  });
  frame->AddView(btnBack);
  btnBack->Show();
  btnBack->SetFocus();
  this->Show();
}

LeagueTeamPlayerDevelopmentPage::~LeagueTeamPlayerDevelopmentPage() {}

LeagueTeamSetupPage::LeagueTeamSetupPage(Gui2WindowManager* windowManager,
                                         const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* frame = new Gui2Frame(windowManager, "frame_league_team_setup", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_league_team_setup", 2, 2, 66, 3, "Team Setup");
  frame->AddView(title);
  title->Show();

  auto result = GetDB()->Query(
      "SELECT t.name, l.name FROM teams t JOIN leagues l ON t.league_id = l.id "
      "JOIN settings s ON t.id = s.team_id LIMIT 1");

  std::string teamName = "Unknown";
  std::string leagueName = "Unknown";
  if (!result->data.empty()) {
    teamName = result->data.at(0).at(0);
    leagueName = result->data.at(0).at(1);
  }

  Gui2Caption* infoTeam =
      new Gui2Caption(windowManager, "caption_setup_team", 2, 6, 66, 2.5, "Team: " + teamName);
  frame->AddView(infoTeam);
  infoTeam->Show();

  Gui2Caption* infoLeague = new Gui2Caption(windowManager, "caption_setup_league", 2, 9, 66, 2.5,
                                            "League: " + leagueName);
  frame->AddView(infoLeague);
  infoLeague->Show();

  Gui2Caption* sectionLabel =
      new Gui2Caption(windowManager, "caption_setup_section", 2, 14, 66, 2, "-- Squad Overview --");
  frame->AddView(sectionLabel);
  sectionLabel->Show();

  Gui2Caption* squadHeader =
      new Gui2Caption(windowManager, "caption_setup_squad_header", 2, 17, 66, 2,
                      "Name                  | Role                | Age");
  frame->AddView(squadHeader);
  squadHeader->Show();

  auto playersResult = GetDB()->Query(
      "SELECT p.firstname, p.lastname, p.role, p.age FROM players p "
      "JOIN teams t ON p.team_id = t.id JOIN settings s ON t.id = s.team_id "
      "ORDER BY p.formationorder");

  int squadCount = 0;
  int avgAge = 0;
  if (!playersResult->data.empty()) {
    Gui2Grid* grid = new Gui2Grid(windowManager, "grid_setup_players", 2, 20, 66, 60);
    int row = 0;
    for (const auto& r : playersResult->data) {
      squadCount++;
      avgAge += atoi(r.at(3).c_str());
      char buf[256];
      snprintf(buf, sizeof(buf), "%-20s | %-19s | %2s", (r.at(0) + " " + r.at(1)).c_str(),
               r.at(2).c_str(), r.at(3).c_str());
      Gui2Caption* cap = new Gui2Caption(windowManager, "caption_setup_p_" + std::to_string(row), 0,
                                         0, 86, 2.5, buf);
      grid->AddView(cap, row++, 0);
    }
    grid->UpdateLayout(0.5);
    frame->AddView(grid);
    grid->Show();
  }

  if (squadCount > 0)
    avgAge /= squadCount;
  char summaryBuf[256];
  snprintf(summaryBuf, sizeof(summaryBuf), "Squad: %d players | Average age: %d", squadCount,
           avgAge);
  Gui2Caption* summaryCap =
      new Gui2Caption(windowManager, "caption_setup_summary", 2, 84, 66, 2.5, summaryBuf);
  frame->AddView(summaryCap);
  summaryCap->Show();

  Gui2Button* btnBack = new Gui2Button(windowManager, "btn_setup_back", 25, 92, 50, 3,
                                       Localization::GetInstance().Translate("action_back"));
  btnBack->sig_OnClick.connect([this, windowManager](...) {
    this->Exit();
    Properties properties;
    windowManager->GetPageFactory()->CreatePage(static_cast<int>(e_PageID_League_Team), properties,
                                                0);
    delete this;
  });
  frame->AddView(btnBack);
  btnBack->Show();
  btnBack->SetFocus();
  this->Show();
}

LeagueTeamSetupPage::~LeagueTeamSetupPage() {}
