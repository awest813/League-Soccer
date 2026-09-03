#include "league_team.hpp"

#include <string>

#include "../../league/leaguecode.hpp"
#include "../../main.hpp"
#include "../pagefactory.hpp"
#include "base/utils.hpp"
#include "menu_smoke.hpp"
#include "utils/gui2/widgets/dialog.hpp"
#include "utils/gui2/widgets/text.hpp"
#include "utils/localization.hpp"
#include "utils/xmlloader.hpp"

namespace {

struct FormationSlot {
  const char* role;
  float x, y;
};

// Standard presets in the pitch-coordinate space the match engine consumes
// (x: own goal -1 .. opponent goal 1, y: +left / -right).
const std::vector<FormationSlot>& GetFormationPreset(const std::string& name) {
  static const std::map<std::string, std::vector<FormationSlot>> presets = {
      {"4-4-2",
       {{"GK", -1.0f, 0.0f},
        {"LB", -0.7f, 0.75f},
        {"CB", -1.0f, 0.25f},
        {"CB", -1.0f, -0.25f},
        {"RB", -0.7f, -0.75f},
        {"LM", -0.2f, 0.75f},
        {"CM", -0.2f, 0.25f},
        {"CM", -0.2f, -0.25f},
        {"RM", -0.2f, -0.75f},
        {"CF", 0.8f, 0.3f},
        {"CF", 0.8f, -0.3f}}},
      {"4-3-3",
       {{"GK", -1.0f, 0.0f},
        {"LB", -0.7f, 0.8f},
        {"CB", -1.0f, 0.28f},
        {"CB", -1.0f, -0.28f},
        {"RB", -0.7f, -0.8f},
        {"CM", -0.3f, 0.45f},
        {"DM", -0.45f, 0.0f},
        {"CM", -0.3f, -0.45f},
        {"LM", 0.7f, 0.7f},
        {"CF", 0.95f, 0.0f},
        {"RM", 0.7f, -0.7f}}},
      {"4-2-3-1",
       {{"GK", -1.0f, 0.0f},
        {"LB", -0.7f, 0.8f},
        {"CB", -1.0f, 0.28f},
        {"CB", -1.0f, -0.28f},
        {"RB", -0.7f, -0.8f},
        {"DM", -0.35f, 0.25f},
        {"DM", -0.35f, -0.25f},
        {"LM", 0.15f, 0.7f},
        {"AM", 0.25f, 0.0f},
        {"RM", 0.15f, -0.7f},
        {"CF", 0.9f, 0.0f}}},
      {"3-5-2",
       {{"GK", -1.0f, 0.0f},
        {"CB", -1.0f, 0.4f},
        {"CB", -1.0f, 0.0f},
        {"CB", -1.0f, -0.4f},
        {"LM", -0.4f, 0.9f},
        {"CM", -0.3f, 0.4f},
        {"DM", -0.5f, 0.0f},
        {"CM", -0.3f, -0.4f},
        {"RM", -0.4f, -0.9f},
        {"CF", 0.85f, 0.25f},
        {"CF", 0.85f, -0.25f}}},
      {"5-3-2",
       {{"GK", -1.0f, 0.0f},
        {"LB", -0.65f, 0.9f},
        {"CB", -1.0f, 0.45f},
        {"CB", -1.0f, 0.0f},
        {"CB", -1.0f, -0.45f},
        {"RB", -0.65f, -0.9f},
        {"CM", -0.25f, 0.4f},
        {"DM", -0.4f, 0.0f},
        {"CM", -0.25f, -0.4f},
        {"CF", 0.85f, 0.25f},
        {"CF", 0.85f, -0.25f}}},
  };
  return presets.at(name);
}

std::string SerializeFormation(const std::vector<FormationSlot>& slots) {
  std::string xml;
  for (unsigned int i = 0; i < slots.size(); i++) {
    char buf[128];
    snprintf(buf, sizeof(buf), "<p%u><position>%.2f, %.2f</position><role>%s</role></p%u>", i + 1,
             slots.at(i).x, slots.at(i).y, slots.at(i).role, i + 1);
    xml += buf;
  }
  return xml;
}

bool IsGoalkeeperRole(const std::string& role) { return role.compare(0, 2, "GK") == 0; }

std::string FormationSummary(const std::string& formationXML) {
  XMLLoader loader;
  XMLTree tree = loader.Load(formationXML);
  std::string summary;
  for (int num = 1; num <= 11; num++) {
    auto it = tree.children.find("p" + int_to_str(num));
    if (it == tree.children.end()) continue;
    auto roleIt = it->second.children.find("role");
    if (roleIt == it->second.children.end()) continue;
    if (!summary.empty()) summary += " ";
    summary += roleIt->second.value;
  }
  return summary;
}

}  // namespace

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
      new Gui2Caption(windowManager, "caption_league_team", 3, 2, 36, 3,
                      Localization::GetInstance().Translate("league_team_management"));
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
      new Gui2Caption(windowManager, "caption_team_actions", 2, 2, 32, 2,
                      Localization::GetInstance().Translate("league_squad_tools"));
  actionPanel->AddView(actionTitle);
  actionTitle->Show();

  Gui2Button* btnFormation =
      new Gui2Button(windowManager, "btn_team_formation", 0, 0, 34, 3,
                     Localization::GetInstance().Translate("league_formation"));
  Gui2Button* btnPlayerSel =
      new Gui2Button(windowManager, "btn_team_playersel", 0, 0, 34, 3,
                     Localization::GetInstance().Translate("league_player_selection"));
  Gui2Button* btnTactics =
      new Gui2Button(windowManager, "btn_team_tactics", 0, 0, 34, 3,
                     Localization::GetInstance().Translate("league_tactics"));
  Gui2Button* btnPlayerOvr =
      new Gui2Button(windowManager, "btn_team_playerovr", 0, 0, 34, 3,
                     Localization::GetInstance().Translate("league_player_overview"));
  Gui2Button* btnPlayerDev =
      new Gui2Button(windowManager, "btn_team_playerdev", 0, 0, 34, 3,
                     Localization::GetInstance().Translate("league_player_development"));
  Gui2Button* btnSetup = new Gui2Button(
      windowManager, "btn_team_setup", 0, 0, 34, 3,
      Localization::GetInstance().Translate("league_team_setup"));
  Gui2Button* btnBack =
      new Gui2Button(windowManager, "btn_team_back", 0, 0, 34, 3,
                     Localization::GetInstance().Translate("league_back_dashboard"));

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
      new Gui2Caption(windowManager, "caption_team_summary", 2, 2, 30, 2,
                      Localization::GetInstance().Translate("league_squad_snapshot"));
  squadPanel->AddView(squadTitle);
  squadTitle->Show();

  Gui2Caption* squadBody = new Gui2Caption(
      windowManager, "caption_team_summary_body", 2, 6, 30, 10,
      TRF("league_squad_body", {squadSize, avgStat, youngestAge, oldestAge}));
  squadPanel->AddView(squadBody);
  squadBody->Show();

  Gui2Frame* structurePanel =
      new Gui2Frame(windowManager, "frame_team_structure", 45, 43, 36, 18, true);
  frame->AddView(structurePanel);
  structurePanel->Show();

  Gui2Caption* structureTitle =
      new Gui2Caption(windowManager, "caption_team_structure", 2, 2, 30, 2,
                      Localization::GetInstance().Translate("league_structure"));
  structurePanel->AddView(structureTitle);
  structureTitle->Show();

  std::string formationStatus = hasFormation ? TR("league_configured") : TR("league_needs_setup");
  std::string tacticsStatus = hasTactics ? TR("league_configured") : TR("league_needs_setup");
  Gui2Caption* structureBody = new Gui2Caption(
      windowManager, "caption_team_structure_body", 2, 6, 30, 6,
      TRF("league_structure_body", {formationStatus, tacticsStatus}));
  structurePanel->AddView(structureBody);
  structureBody->Show();

  Gui2Frame* guidancePanel =
      new Gui2Frame(windowManager, "frame_team_guidance", 45, 65, 36, 17, true);
  frame->AddView(guidancePanel);
  guidancePanel->Show();

  Gui2Caption* guidanceTitle =
      new Gui2Caption(windowManager, "caption_team_guidance", 2, 2, 30, 2,
                      Localization::GetInstance().Translate("league_recommended_flow"));
  guidancePanel->AddView(guidanceTitle);
  guidanceTitle->Show();

  Gui2Caption* guidanceBody =
      new Gui2Caption(windowManager, "caption_team_guidance_body", 2, 6, 30, 6,
                      Localization::GetInstance().Translate("league_guidance_body"));
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
    : Gui2Page(windowManager, pageData),
      frame(nullptr) {
  frame = new Gui2Frame(windowManager, "frame_league_team_formation", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  auto& loc = Localization::GetInstance();

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_league_team_formation", 2, 2, 66, 3,
                      loc.Translate("league_formation"));
  frame->AddView(title);
  title->Show();

  auto result = GetDB()->Query(
      "SELECT t.name, t.formation_xml, t.formation_factory_xml "
      "FROM teams t, settings s WHERE t.id = s.team_id LIMIT 1");
  if (!result->data.empty()) {
    std::string teamName = result->data.at(0).at(0);
    std::string formationXML = result->data.at(0).at(1);
    std::string factoryXML =
        (result->data.at(0).size() > 2) ? result->data.at(0).at(2) : std::string();

    Gui2Caption* info =
        new Gui2Caption(windowManager, "caption_formation_team", 2, 6, 66, 3,
                        TRF("league_team_line", {teamName}));
    frame->AddView(info);
    info->Show();

    std::string summary = FormationSummary(formationXML);
    if (summary.empty()) summary = loc.Translate("league_needs_setup");
    Gui2Caption* current = new Gui2Caption(
        windowManager, "caption_formation_current", 2, 10, 66, 2.5,
        loc.TranslateAndFormat("league_formation_current", {summary}));
    frame->AddView(current);
    current->Show();

    // One button per standard preset, plus a factory reset.
    std::vector<std::string> presetNames = {"4-4-2", "4-3-3", "4-2-3-1", "3-5-2", "5-3-2"};
    Gui2Grid* presetGrid = new Gui2Grid(windowManager, "grid_formation_presets", 2, 15, 66, 50);
    int row = 0;
    for (const auto& name : presetNames) {
      Gui2Button* btn = new Gui2Button(windowManager, "btn_formation_" + name, 0, 0, 64, 3, name);
      std::string presetXML = SerializeFormation(GetFormationPreset(name));
      btn->sig_OnClick.connect([this, windowManager, presetXML](...) {
        ApplyFormation(presetXML);
      });
      presetGrid->AddView(btn, row++, 0);
    }

    Gui2Button* btnFactory = new Gui2Button(windowManager, "btn_formation_factory", 0, 0, 64, 3,
                                            loc.Translate("league_formation_factory"));
    btnFactory->sig_OnClick.connect([this, windowManager, factoryXML](...) {
      if (!factoryXML.empty()) {
        ApplyFormation(factoryXML);
      }
    });
    presetGrid->AddView(btnFactory, row++, 0);

    presetGrid->UpdateLayout(0.4);
    frame->AddView(presetGrid);
    presetGrid->Show();
    presetGrid->SetFocus();
  }

  Gui2Button* btnBack = new Gui2Button(windowManager, "btn_formation_back", 15, 86, 40, 3,
                                       loc.Translate("action_back"));
  btnBack->sig_OnClick.connect([this, windowManager](...) {
    this->Exit();
    Properties properties;
    windowManager->GetPageFactory()->CreatePage(static_cast<int>(e_PageID_League_Team), properties,
                                                0);
    delete this;
  });
  frame->AddView(btnBack);
  btnBack->Show();
  this->Show();
}

void LeagueTeamFormationPage::ApplyFormation(const std::string& formationXML) {
  int userTeamID = 0;
  if (LeagueGetUserTeamID(userTeamID)) {
    GetDB()->Query("UPDATE teams SET formation_xml = '" + formationXML +
                   "' WHERE id = " + int_to_str(userTeamID));
    GetDB()->Query(
        "INSERT INTO inbox_messages (sender, subject, body) VALUES "
        "('Kit Man', 'Formation changed', 'The gaffer has set a new formation. It takes effect in "
        "the next match.')");
  }

  // Recreate so the summary reflects the new formation.
  this->Exit();
  Properties properties;
  windowManager->GetPageFactory()->CreatePage(static_cast<int>(e_PageID_League_Team_Formation),
                                              properties, 0);
  delete this;
}

LeagueTeamFormationPage::~LeagueTeamFormationPage() {}

LeagueTeamPlayerSelectionPage::LeagueTeamPlayerSelectionPage(Gui2WindowManager* windowManager,
                                                             const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData),
      frame(nullptr),
      squadGrid(nullptr),
      feedbackCaption(nullptr),
      selectedPlayerDBID(-1),
      selectedIsGoalkeeper(false) {
  frame = new Gui2Frame(windowManager, "frame_league_team_playersel", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  feedbackCaption = new Gui2Caption(windowManager, "caption_playersel_feedback", 2, 6, 66, 2.5,
                                    Localization::GetInstance().Translate("league_lineup_hint"));
  frame->AddView(feedbackCaption);
  feedbackCaption->Show();

  RefreshSquad();

  this->Show();
}

void LeagueTeamPlayerSelectionPage::RefreshSquad() {
  if (squadGrid) {
    squadGrid->Exit();
    delete squadGrid;
    squadGrid = nullptr;
  }

  auto& loc = Localization::GetInstance();

  squadGrid = new Gui2Grid(windowManager, "grid_playersel", 2, 9, 66, 74);
  int row = 0;

  auto result = GetDB()->Query(
      "SELECT p.id, p.firstname, p.lastname, p.role, p.base_stat, p.formationorder "
      "FROM players p, teams t, settings s "
      "WHERE p.team_id = t.id AND t.id = s.team_id ORDER BY p.formationorder");

  if (!result->data.empty()) {
    for (const auto& r : result->data) {
      int playerDBID = atoi(r.at(0).c_str());
      int order = atoi(r.at(5).c_str());
      bool starter = order < 11;
      std::string slotLabel =
          starter ? int_to_str(order + 1) : loc.Translate("league_lineup_bench");
      std::string label = "[" + slotLabel + "] " + r.at(1) + " " + r.at(2) + " (" + r.at(3) + ")";

      Gui2Button* btn = new Gui2Button(windowManager, "btn_player_" + r.at(0), 0, 0, 65, 2.5,
                                       label);
      if (playerDBID == selectedPlayerDBID) {
        btn->SetColor(Vector3(250, 210, 60));
      } else if (!starter) {
        btn->SetColor(Vector3(170, 170, 170));
      }
      btn->sig_OnClick.connect([this, playerDBID, r](...) {
        bool isGK = IsGoalkeeperRole(r.at(3));
        int order = atoi(r.at(5).c_str());

        if (selectedPlayerDBID == -1) {
          selectedPlayerDBID = playerDBID;
          selectedIsGoalkeeper = isGK;
          if (feedbackCaption) {
            feedbackCaption->SetCaption(Localization::GetInstance().TranslateAndFormat(
                "league_lineup_selected",
                {r.at(1) + " " + r.at(2)}));
          }
          RefreshSquad();
          return;
        }

        if (selectedPlayerDBID == playerDBID) {
          // Clicking again deselects.
          selectedPlayerDBID = -1;
          if (feedbackCaption) {
            feedbackCaption->SetCaption(
                Localization::GetInstance().Translate("league_lineup_hint"));
          }
          RefreshSquad();
          return;
        }

        if (isGK != selectedIsGoalkeeper) {
          if (feedbackCaption) {
            feedbackCaption->SetCaption(
                Localization::GetInstance().Translate("league_lineup_gk_rule"));
          }
          return;
        }
        if (isGK && order >= 11) {
          // Both are goalkeepers; the second GK on the bench stays there.
          if (feedbackCaption) {
            feedbackCaption->SetCaption(
                Localization::GetInstance().Translate("league_lineup_gk_rule"));
          }
          return;
        }

        Gui2Dialog* dlg = new Gui2Dialog(
            windowManager, "dialog_playersel_swap", 25, 30, 50, 25,
            Localization::GetInstance().Translate("league_lineup_swap_prompt"));
        (dlg->AddPosNegButtons(Localization::GetInstance().Translate("league_yes"),
                               Localization::GetInstance().Translate("league_no")))
            ->SetFocus();
        dlg->sig_OnPositive.connect([this, dlg, playerDBID](...) {
          dlg->Exit();
          delete dlg;
          SwapPlayers(selectedPlayerDBID, playerDBID);
        });
        dlg->sig_OnNegative.connect([this, dlg](...) {
          dlg->Exit();
          delete dlg;
        });
        this->AddView(dlg);
        dlg->Show();
      });
      squadGrid->AddView(btn, row++, 0);
    }
  } else {
    Gui2Caption* emptyCap = new Gui2Caption(windowManager, "caption_playersel_empty", 0, 0, 65, 3,
                                            "No players in the squad.");
    squadGrid->AddView(emptyCap, row++, 0);
  }

  // Back lives in the same grid so keyboard/gamepad can reach it too.
  Gui2Button* btnBack = new Gui2Button(windowManager, "btn_playersel_back", 0, 0, 65, 2.5,
                                       Localization::GetInstance().Translate("action_back"));
  btnBack->sig_OnClick.connect([this](...) {
    this->Exit();
    Properties properties;
    windowManager->GetPageFactory()->CreatePage(static_cast<int>(e_PageID_League_Team), properties,
                                                0);
    delete this;
  });
  squadGrid->AddView(btnBack, row, 0);

  squadGrid->UpdateLayout(0.5);
  frame->AddView(squadGrid);
  squadGrid->Show();
}

void LeagueTeamPlayerSelectionPage::SwapPlayers(int idA, int idB) {
  auto orders = GetDB()->Query(
      "SELECT id, formationorder FROM players WHERE id = " + int_to_str(idA) +
      " OR id = " + int_to_str(idB));
  if (orders->data.size() < 2) {
    return;
  }
  int orderA = atoi(orders->data.at(0).at(1).c_str());
  int orderB = atoi(orders->data.at(1).at(1).c_str());

  GetDB()->Query("UPDATE players SET formationorder = " + int_to_str(orderB) +
                 " WHERE id = " + int_to_str(idA));
  GetDB()->Query("UPDATE players SET formationorder = " + int_to_str(orderA) +
                 " WHERE id = " + int_to_str(idB));

  selectedPlayerDBID = -1;
  if (feedbackCaption) {
    feedbackCaption->SetCaption(Localization::GetInstance().Translate("league_lineup_swapped"));
  }
  RefreshSquad();
}

LeagueTeamPlayerSelectionPage::~LeagueTeamPlayerSelectionPage() {}

LeagueTeamTacticsPage::LeagueTeamTacticsPage(Gui2WindowManager* windowManager,
                                             const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData),
      frame(nullptr),
      feedbackCaption(nullptr) {
  frame = new Gui2Frame(windowManager, "frame_league_team_tactics", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  auto& loc = Localization::GetInstance();

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_league_team_tactics", 2, 2, 66, 3,
                      loc.Translate("league_team_tactics"));
  frame->AddView(title);
  title->Show();

  auto result = GetDB()->Query(
      "SELECT t.tactics_xml, t.tactics_factory_xml FROM teams t, settings s "
      "WHERE t.id = s.team_id LIMIT 1");

  if (!result->data.empty() && !result->data.at(0).at(0).empty()) {
    std::string tacticsXML = result->data.at(0).at(0);

    // One slider per tactic key currently stored for the team; values are the
    // 0..1 factors the match engine reads from tactics_xml.
    XMLLoader loader;
    XMLTree tree = loader.Load(tacticsXML);
    Gui2Grid* sliderGrid = new Gui2Grid(windowManager, "grid_tactics", 2, 7, 66, 70);
    int row = 0;
    for (auto& kv : tree.children) {
      float value = atof(kv.second.value.c_str());
      std::string label = kv.first;
      std::replace(label.begin(), label.end(), '_', ' ');
      Gui2Slider* slider =
          new Gui2Slider(windowManager, "slider_tactics_" + kv.first, 0, 0, 62, 4.5, label);
      slider->SetQuantization(20);
      slider->SetValue(value);
      tacticSliders.push_back({kv.first, slider});
      sliderGrid->AddView(slider, row++, 0);
      if (row >= 14) {
        break;  // grid gets scrollable beyond that; keep initial layout sane
      }
    }

    Gui2Button* btnSave = new Gui2Button(windowManager, "btn_tactics_save", 0, 0, 62, 3,
                                         loc.Translate("league_tactics_save"));
    btnSave->sig_OnClick.connect([this](...) { SaveTactics(); });
    sliderGrid->AddView(btnSave, row++, 0);

    sliderGrid->UpdateLayout(0.3);
    frame->AddView(sliderGrid);
    sliderGrid->Show();
    sliderGrid->SetFocus();
  }

  feedbackCaption = new Gui2Caption(windowManager, "caption_tactics_feedback", 2, 81, 66, 3, "");
  frame->AddView(feedbackCaption);
  feedbackCaption->Show();

  Gui2Button* btnBack = new Gui2Button(windowManager, "btn_tactics_back", 15, 86, 40, 3,
                                       loc.Translate("action_back"));
  btnBack->sig_OnClick.connect([this, windowManager](...) {
    this->Exit();
    Properties properties;
    windowManager->GetPageFactory()->CreatePage(static_cast<int>(e_PageID_League_Team), properties,
                                                0);
    delete this;
  });
  frame->AddView(btnBack);
  btnBack->Show();
  this->Show();
}

void LeagueTeamTacticsPage::SaveTactics() {
  if (tacticSliders.empty()) {
    return;
  }

  std::string tacticsXML;
  for (const auto& entry : tacticSliders) {
    char buf[64];
    snprintf(buf, sizeof(buf), "<%s>%.6f</%s>", entry.first.c_str(), entry.second->GetValue(),
             entry.first.c_str());
    tacticsXML += buf;
  }

  int userTeamID = 0;
  if (LeagueGetUserTeamID(userTeamID)) {
    GetDB()->Query("UPDATE teams SET tactics_xml = '" + tacticsXML +
                   "' WHERE id = " + int_to_str(userTeamID));
  }
  if (feedbackCaption) {
    feedbackCaption->SetCaption(Localization::GetInstance().Translate("league_tactics_saved"));
  }
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
                                       66, 3,
                                        Localization::GetInstance().Translate("league_player_overview"));
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
          new Gui2Button(windowManager, "btn_povr_" + std::to_string(row), 0, 0, 65, 2.5, btnLabel);
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

  Gui2Button* btnBack = new Gui2Button(windowManager, "btn_playerovr_back", 15, 86, 40, 3,
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
    : Gui2Page(windowManager, pageData),
      frame(nullptr),
      squadGrid(nullptr),
      feedbackCaption(nullptr),
      selectedPlayerDBID(-1) {
  frame = new Gui2Frame(windowManager, "frame_league_team_playerdev", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  auto& loc = Localization::GetInstance();

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_league_team_playerdevelopment", 2, 2, 66, 3,
                      loc.Translate("league_player_development"));
  frame->AddView(title);
  title->Show();

  feedbackCaption = new Gui2Caption(windowManager, "caption_playerdev_feedback", 2, 6, 66, 2.5,
                                    loc.Translate("league_train_hint"));
  frame->AddView(feedbackCaption);
  feedbackCaption->Show();

  RefreshSquad();

  // Training focus actions: pick a player above, then a focus below.
  Gui2Grid* trainGrid = new Gui2Grid(windowManager, "grid_playerdev_train", 2, 75, 66, 8);
  struct TrainFocus {
    const char* labelKey;
    const char* attributeName;
  };
  std::vector<TrainFocus> focuses = {
      {"league_train_accel", "physical_acceleration"},
      {"league_train_stamina", "physical_stamina"},
      {"league_train_dribble", "technical_dribble"},
      {"league_train_shot", "technical_shot"},
  };
  int col = 0;
  for (const auto& focus : focuses) {
    Gui2Button* btn = new Gui2Button(windowManager, "btn_train_" + std::string(focus.attributeName),
                                     0, 0, 15.5, 3, loc.Translate(focus.labelKey));
    std::string attributeName(focus.attributeName);
    std::string label(focus.labelKey);
    btn->sig_OnClick.connect([this, attributeName, label](...) {
      TrainSelected(attributeName, Localization::GetInstance().Translate(label));
    });
    trainGrid->AddView(btn, 0, col++);
  }
  trainGrid->UpdateLayout(0.3);
  frame->AddView(trainGrid);
  trainGrid->Show();

  Gui2Button* btnBack = new Gui2Button(windowManager, "btn_playerdev_back", 2, 84, 66, 3,
                                       loc.Translate("action_back"));
  btnBack->sig_OnClick.connect([this, windowManager](...) {
    this->Exit();
    Properties properties;
    windowManager->GetPageFactory()->CreatePage(static_cast<int>(e_PageID_League_Team), properties,
                                                0);
    delete this;
  });
  frame->AddView(btnBack);
  btnBack->Show();

  this->Show();
}

void LeagueTeamPlayerDevelopmentPage::RefreshSquad() {
  if (squadGrid) {
    squadGrid->Exit();
    delete squadGrid;
    squadGrid = nullptr;
  }

  squadGrid = new Gui2Grid(windowManager, "grid_playerdev", 2, 9, 66, 64);
  int row = 0;

  auto result = GetDB()->Query(
      "SELECT p.id, p.firstname, p.lastname, p.role, p.age, p.base_stat FROM players p "
      "JOIN teams t ON p.team_id = t.id JOIN settings s ON t.id = s.team_id "
      "ORDER BY p.age ASC, p.base_stat DESC");

  if (result->data.empty()) {
    Gui2Caption* info = new Gui2Caption(windowManager, "caption_playerdev_info", 0, 0, 65, 3,
                                        "No players found on your squad.");
    squadGrid->AddView(info, row++, 0);
  }

  for (const auto& r : result->data) {
    int playerDBID = atoi(r.at(0).c_str());
    std::string fullName = r.at(1) + " " + r.at(2);
    char buf[256];
    snprintf(buf, sizeof(buf), "%-20s | %-19s | %2s | %s", fullName.c_str(), r.at(3).c_str(),
             r.at(4).c_str(), r.at(5).c_str());
    Gui2Button* btn =
        new Gui2Button(windowManager, "btn_pdev_" + r.at(0), 0, 0, 65, 2.5, buf);
    if (playerDBID == selectedPlayerDBID) {
      btn->SetColor(Vector3(250, 210, 60));
    }
    btn->sig_OnClick.connect([this, playerDBID, fullName](...) {
      selectedPlayerDBID = playerDBID;
      if (feedbackCaption) {
        feedbackCaption->SetCaption(
            Localization::GetInstance().TranslateAndFormat("league_lineup_selected", {fullName}));
      }
      RefreshSquad();
    });
    squadGrid->AddView(btn, row++, 0);
  }

  squadGrid->UpdateLayout(0.5);
  frame->AddView(squadGrid);
  squadGrid->Show();
}

void LeagueTeamPlayerDevelopmentPage::TrainSelected(const std::string& attributeName,
                                                    const std::string& attributeLabel) {
  auto& loc = Localization::GetInstance();

  if (selectedPlayerDBID == -1) {
    if (feedbackCaption) {
      feedbackCaption->SetCaption(loc.Translate("league_train_select_first"));
    }
    return;
  }

  auto result = GetDB()->Query(
      "SELECT p.profile_xml, p.age, p.firstname, p.lastname FROM players WHERE id = " +
      int_to_str(selectedPlayerDBID));
  if (result->data.empty() || result->data.at(0).at(0).empty()) {
    return;
  }
  std::string profileXML = result->data.at(0).at(0);
  int age = atoi(result->data.at(0).at(1).c_str());
  std::string fullName = result->data.at(0).at(2) + " " + result->data.at(0).at(3);

  XMLLoader loader;
  XMLTree tree = loader.Load(profileXML);
  auto it = tree.children.find(attributeName);
  if (it == tree.children.end()) {
    return;
  }
  float oldValue = atof(it->second.value.c_str());

  // Younger players grow faster; veterans barely at all.
  float growth = 0.02f;
  if (age <= 23) {
    growth = 0.03f;
  } else if (age >= 32) {
    growth = 0.005f;
  } else if (age >= 29) {
    growth = 0.01f;
  }
  float newValue = std::min(0.99f, oldValue + growth);
  if (newValue <= oldValue) {
    if (feedbackCaption) {
      feedbackCaption->SetCaption(loc.Translate("league_train_maxed"));
    }
    return;
  }

  char buf[96];
  snprintf(buf, sizeof(buf), "<%s>%.6f</%s>", attributeName.c_str(), newValue,
           attributeName.c_str());
  std::string oldTagStart = "<" + attributeName + ">";
  std::string oldTagEnd = "</" + attributeName + ">";
  size_t start = profileXML.find(oldTagStart);
  size_t end = profileXML.find(oldTagEnd, start);
  if (start == std::string::npos || end == std::string::npos) {
    return;
  }
  std::string updatedXML = profileXML.substr(0, start) + buf +
                           profileXML.substr(end + oldTagEnd.size());
  GetDB()->Query("UPDATE players SET profile_xml = '" + updatedXML + "' WHERE id = " +
                 int_to_str(selectedPlayerDBID));

  if (feedbackCaption) {
    feedbackCaption->SetCaption(loc.TranslateAndFormat(
        "league_train_applied",
        {fullName + " - " + attributeLabel, real_to_str(oldValue), real_to_str(newValue)}));
  }
  RefreshSquad();
}

LeagueTeamPlayerDevelopmentPage::~LeagueTeamPlayerDevelopmentPage() {}

LeagueTeamSetupPage::LeagueTeamSetupPage(Gui2WindowManager* windowManager,
                                         const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* frame = new Gui2Frame(windowManager, "frame_league_team_setup", 15, 5, 70, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_league_team_setup", 2, 2, 66, 3,
                      Localization::GetInstance().Translate("league_team_setup"));
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
      new Gui2Caption(windowManager, "caption_setup_team", 2, 6, 66, 2.5,
                        TRF("league_team_line", {teamName}));
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
                                         0, 65, 2.5, buf);
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
      new Gui2Caption(windowManager, "caption_setup_summary", 2, 82, 66, 2.5, summaryBuf);
  frame->AddView(summaryCap);
  summaryCap->Show();

  Gui2Button* btnBack = new Gui2Button(windowManager, "btn_setup_back", 10, 86, 50, 3,
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
