// written by bastiaan konings schuiling 2008 - 2015
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not
// be used for anything important. i do not offer support, so don't ask. to be used for inspiration
// :)

#include "teamselect.hpp"

#include <filesystem>

#include "../../main.hpp"
#include "../pagefactory.hpp"
#include "utils/database.hpp"
#include "utils/localization.hpp"

using namespace blunted;

namespace {

const char* kSelectorFallbackImage = "media/menu/league.png";
constexpr unsigned long kMenuSmokeAdvanceDelay_ms = 250;

std::string ResolveDatabaseImage(const std::string& relativePath) {
  std::filesystem::path imageRoot = "databases/default";
  if (!GetActiveSaveDirectory().empty())
    imageRoot = std::filesystem::path("saves") / GetActiveSaveDirectory();

  std::filesystem::path imagePath = imageRoot / relativePath;
  if (!std::filesystem::exists(imagePath) &&
      imageRoot != std::filesystem::path("databases/default")) {
    imagePath = std::filesystem::path("databases/default") / relativePath;
  }
  return imagePath.generic_string();
}

bool MenuSmokeQuickMatchEnabled() {
  return GetConfiguration()->GetBool("menu_smoke_test_quick_match", false);
}

bool MenuSmokeFullMatchEnabled() {
  return GetConfiguration()->GetBool("menu_smoke_test_full_match", false);
}

bool MenuSmokeAutoQuickMatchEnabled() {
  return MenuSmokeQuickMatchEnabled() || MenuSmokeFullMatchEnabled();
}

int AddCompetitions(Gui2IconSelector* selector) {
  /*
  selector->AddEntry("1", "National teams",
  "databases/default/images_competitions/nationalteams.png"); selector->AddEntry("2", "Premier
  league", "databases/default/images_competitions/premierleague.png"); selector->AddEntry("3",
  "Eredivisie", "databases/default/images_competitions/eredivisie.png"); selector->AddEntry("4",
  "Bundesliga", "databases/default/images_competitions/bundesliga.png"); selector->AddEntry("5",
  "LFP", "databases/default/images_competitions/lfp.png"); selector->AddEntry("6", "Serie A",
  "databases/default/images_competitions/serie_a.png"); selector->AddEntry("7", "Ligue 1",
  "databases/default/images_competitions/ligue1.png");
  */
  auto result = GetDB()->Query("select id, name, logo_url from leagues");
  int competitionCount = 0;

  for (unsigned int r = 0; r < result->data.size(); r++) {
    int id = atoi(result->data.at(r).at(0).c_str());
    std::string name = result->data.at(r).at(1).c_str();
    std::string logo_url = result->data.at(r).at(2).c_str();

    std::string logoPath = ResolveDatabaseImage(logo_url);
    if (!std::filesystem::exists(logoPath))
      logoPath = kSelectorFallbackImage;
    selector->AddEntry(int_to_str(id), name, logoPath);
    competitionCount++;
  }

  if (competitionCount == 0) {
    selector->AddEntry("", "No competitions found", kSelectorFallbackImage);
  }

  return competitionCount;
}

int AddTeams(Gui2IconSelector* selector, const std::string& competition_id) {
  if (competition_id.empty()) {
    selector->AddEntry("", "No teams found", kSelectorFallbackImage);
    return 0;
  }

  auto result = GetDB()->Query("select id, name, logo_url from teams where league_id = " +
                               competition_id + " order by name");
  int teamCount = 0;

  for (unsigned int r = 0; r < result->data.size(); r++) {
    int id = atoi(result->data.at(r).at(0).c_str());
    std::string name = result->data.at(r).at(1).c_str();
    std::string logo_url = result->data.at(r).at(2).c_str();

    std::string logoPath = ResolveDatabaseImage(logo_url);
    if (!std::filesystem::exists(logoPath))
      logoPath = kSelectorFallbackImage;
    selector->AddEntry(int_to_str(id), name, logoPath);
    teamCount++;
  }

  if (teamCount == 0) {
    selector->AddEntry("", "No teams found", kSelectorFallbackImage);
  }

  selector->Show();
  return teamCount;
}

}  // namespace

TeamSelectPage::TeamSelectPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData),
      autoAdvanceTime_ms(EnvironmentManager::GetInstance().GetTime_ms()),
      autoAdvanceStage(0) {
  Gui2Frame* frame1 = new Gui2Frame(windowManager, "teamselect_frame1", 15, 18, 34, 52, true);
  this->AddView(frame1);
  frame1->Show();

  frame2 = new Gui2Frame(windowManager, "teamselect_frame2", 51, 18, 34, 52, true);
  this->AddView(frame2);

  Gui2Caption* brandingNotice =
      new Gui2Caption(windowManager, "teamselect_brandingnotice", 19, 73, 28, 3,
                      "Original League Soccer team branding");
  this->AddView(brandingNotice);
  brandingNotice->SetColor(Vector3(200, 200, 200));
  brandingNotice->SetTransparency(0.6f);
  brandingNotice->SetPosition(50.0f - brandingNotice->GetTextWidthPercent() * 0.5f, 73);
  brandingNotice->Show();

  Gui2Caption* p1 =
      new Gui2Caption(windowManager, "teamselect_caption_p1", 2, 2, 30, 3, "Player 1");
  p2 = new Gui2Caption(windowManager, "teamselect_caption_p2", 2, 2, 30, 3, "Player 2");
  Gui2Grid* grid1 = new Gui2Grid(windowManager, "teamselect_grid_team1", 2, 8, 30, 42);
  grid2 = new Gui2Grid(windowManager, "teamselect_grid_team2", 2, 8, 30, 42);

  competitionSelect1 = new Gui2IconSelector(windowManager, "teamselect_iconselector_competition1",
                                            0, 0, 29, 18, "Competition select");
  competitionSelect2 = new Gui2IconSelector(windowManager, "teamselect_iconselector_competition2",
                                            0, 0, 29, 18, "Competition select");
  teamSelect1 = new Gui2IconSelector(windowManager, "teamselect_iconselector_team1", 0, 0, 29, 18,
                                     "Team select");
  teamSelect2 = new Gui2IconSelector(windowManager, "teamselect_iconselector_team2", 0, 0, 29, 18,
                                     "Team select");
  buttonStart1 = new Gui2Button(windowManager, "teamselect_button_start1", 0, 0, 29, 3, "Ready");
  buttonStart2 = new Gui2Button(windowManager, "teamselect_button_start2", 0, 0, 29, 3, "Ready");
  Gui2Button* buttonBack = new Gui2Button(windowManager, "teamselect_button_back", 0, 0, 29, 3,
                                          Localization::GetInstance().Translate("action_back"));

  competitionSelect1->sig_OnClick.connect([this](...) { FocusTeamSelect1(); });
  teamSelect1->sig_OnClick.connect([this](...) { FocusStart1(); });
  buttonStart1->sig_OnClick.connect([this](...) { FocusCompetitionSelect2(); });
  competitionSelect2->sig_OnClick.connect([this](...) { FocusTeamSelect2(); });
  teamSelect2->sig_OnClick.connect([this](...) { FocusStart2(); });
  buttonStart2->sig_OnClick.connect([this](...) { GoOptionsMenu(); });
  buttonBack->sig_OnClick.connect([this](...) { StepBack(); });

  competitionSelect1->sig_OnChange.connect([this](...) { SetupTeamSelect1(); });
  competitionSelect2->sig_OnChange.connect([this](...) { SetupTeamSelect2(); });

  frame1->AddView(p1);
  p1->Show();
  frame1->AddView(grid1);
  grid1->AddView(competitionSelect1, 0, 0);
  grid1->AddView(teamSelect1, 1, 0);
  grid1->AddView(buttonStart1, 2, 0);
  grid1->AddView(buttonBack, 3, 0);
  grid1->UpdateLayout(0.5);
  grid1->Show();

  AddCompetitions(competitionSelect1);
  SetupTeamSelect1();

  frame2->AddView(p2);
  frame2->AddView(grid2);
  grid2->AddView(competitionSelect2, 0, 0);
  grid2->AddView(teamSelect2, 1, 0);
  grid2->AddView(buttonStart2, 2, 0);
  grid2->UpdateLayout(0.5);

  AddCompetitions(competitionSelect2);
  SetupTeamSelect2();

  competitionSelect1->SetFocus();

  SetActiveController(-1, true);

  p2->Hide();
  grid2->Hide();
  frame2->Hide();

  this->Show();
}

TeamSelectPage::~TeamSelectPage() {
  GetMenuTask()->SetActiveJoystickID(0);
  GetMenuTask()->EnableKeyboard();
}

void TeamSelectPage::Process() {
  Gui2Page::Process();

  if (!MenuSmokeAutoQuickMatchEnabled()) {
    return;
  }

  if (EnvironmentManager::GetInstance().GetTime_ms() <
      autoAdvanceTime_ms + kMenuSmokeAdvanceDelay_ms) {
    return;
  }

  if (autoAdvanceStage == 0) {
    if (!p2->IsVisible()) {
      printf("[menu-smoke] Team select ready, revealing CPU opponent selection\n");
      FocusCompetitionSelect2();
    }
    autoAdvanceStage = 1;
    autoAdvanceTime_ms = EnvironmentManager::GetInstance().GetTime_ms();
    return;
  }

  if (autoAdvanceStage == 1) {
    if (teamSelect1->GetSelectedEntryID().empty() || teamSelect2->GetSelectedEntryID().empty()) {
      printf("[menu-smoke] Team select is waiting for valid team data\n");
      return;
    }

    autoAdvanceStage = 2;
    printf("[menu-smoke] Team select confirmed, opening match options\n");
    GoOptionsMenu();
  }
}

void TeamSelectPage::FocusTeamSelect1() {
  if (!teamSelect1->GetSelectedEntryID().empty()) {
    teamSelect1->SetFocus();
  }
}

void TeamSelectPage::FocusStart1() {
  if (!teamSelect1->GetSelectedEntryID().empty()) {
    buttonStart1->SetFocus();
  }
}

void TeamSelectPage::FocusCompetitionSelect2() {
  if (teamSelect1->GetSelectedEntryID().empty()) {
    return;
  }

  p2->Show();
  grid2->Show();
  frame2->Show();

  competitionSelect2->SetFocus();

  SetActiveController(1, true);
}

void TeamSelectPage::FocusTeamSelect2() {
  if (!teamSelect2->GetSelectedEntryID().empty()) {
    teamSelect2->SetFocus();
  }
}

void TeamSelectPage::FocusStart2() {
  if (!teamSelect2->GetSelectedEntryID().empty()) {
    buttonStart2->SetFocus();
  }
}

void TeamSelectPage::SetupTeamSelect1() {
  teamSelect1->ClearEntries();
  AddTeams(teamSelect1, competitionSelect1->GetSelectedEntryID());
  UpdateReadyButtons();
}

void TeamSelectPage::SetupTeamSelect2() {
  teamSelect2->ClearEntries();
  const int teamCount = AddTeams(teamSelect2, competitionSelect2->GetSelectedEntryID());
  if (teamCount > 1 &&
      teamSelect2->GetSelectedEntryID() == teamSelect1->GetSelectedEntryID()) {
    teamSelect2->SetSelectedEntryIndex(1);
  }
  UpdateReadyButtons();

  // hax lol, well doesn't seem to work :(
  /*
  teamSelect2->Process();
  WindowingEvent *right = new WindowingEvent();
  right->SetDirection(Vector3(1, 0, 0));
  teamSelect2->ProcessWindowingEvent(right);
  delete right;
  */
}

void TeamSelectPage::UpdateReadyButtons() {
  buttonStart1->SetActive(!teamSelect1->GetSelectedEntryID().empty());
  buttonStart2->SetActive(!teamSelect2->GetSelectedEntryID().empty());
}

void TeamSelectPage::GoOptionsMenu() {
  if (teamSelect1->GetSelectedEntryID().empty() || teamSelect2->GetSelectedEntryID().empty()) {
    return;
  }

  GetMenuTask()->SetTeamIDs(teamSelect1->GetSelectedEntryID(), teamSelect2->GetSelectedEntryID());
  if (MenuSmokeAutoQuickMatchEnabled()) {
    printf("[menu-smoke] Selected fixture: team %s vs team %s\n",
           teamSelect1->GetSelectedEntryID().c_str(),
           teamSelect2->GetSelectedEntryID().c_str());
  }
  // printf("teams: %i vs %i\n", atoi(teamSelect1->GetSelectedEntryID().c_str()),
  // atoi(teamSelect2->GetSelectedEntryID().c_str()));

  this->Exit();

  Properties properties;
  windowManager->GetPageFactory()->CreatePage((int)e_PageID_MatchOptions, properties, 0);

  delete this;
}

void TeamSelectPage::StepBack() {
  // Mirrors the Escape key walk in ProcessWindowingEvent() so the visible
  // "Back" button behaves identically: step up one focus level, and leave the
  // page (to ControllerSelect) when already at the first selection.
  Gui2View* focus = windowManager->GetFocus();
  if (focus == competitionSelect1) {
    GoBack();
  } else if (focus == teamSelect1) {
    windowManager->SetFocus(competitionSelect1);
  } else if (focus == buttonStart1) {
    windowManager->SetFocus(teamSelect1);
  } else if (focus == competitionSelect2) {
    windowManager->SetFocus(buttonStart1);
    p2->Hide();
    grid2->Hide();
    frame2->Hide();
    SetActiveController(-1, true);
  } else if (focus == teamSelect2) {
    windowManager->SetFocus(competitionSelect2);
  } else if (focus == buttonStart2) {
    windowManager->SetFocus(teamSelect2);
  } else {
    GoBack();
  }
}

void TeamSelectPage::ProcessWindowingEvent(WindowingEvent* event) {
  if (event->IsEscape()) {
    if (windowManager->GetFocus() == competitionSelect1) {
      Gui2Page::ProcessWindowingEvent(event);
    } else if (windowManager->GetFocus() == teamSelect1) {
      windowManager->SetFocus(competitionSelect1);
    } else if (windowManager->GetFocus() == buttonStart1) {
      windowManager->SetFocus(teamSelect1);
    } else if (windowManager->GetFocus() == competitionSelect2) {
      windowManager->SetFocus(buttonStart1);

      p2->Hide();
      grid2->Hide();
      frame2->Hide();

      SetActiveController(-1, true);

    } else if (windowManager->GetFocus() == teamSelect2) {
      windowManager->SetFocus(competitionSelect2);
    } else if (windowManager->GetFocus() == buttonStart2) {
      windowManager->SetFocus(teamSelect2);
    }
  }
}
