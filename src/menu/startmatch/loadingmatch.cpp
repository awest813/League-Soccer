// written by bastiaan konings schuiling 2008 - 2015
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not
// be used for anything important. i do not offer support, so don't ask. to be used for inspiration
// :)

#include "loadingmatch.hpp"

#include <filesystem>

#include "../pagefactory.hpp"
#include "main.hpp"
#include "utils/gui2/widgets/frame.hpp"

using namespace blunted;

namespace {

const char* kLoadingFallbackLogo = "media/menu/league.png";

std::string ResolveTeamLogo(const TeamData* teamData) {
  const std::string& logoPath = teamData->GetLogoUrl();
  if (!logoPath.empty() && std::filesystem::exists(logoPath)) {
    return logoPath;
  }
  return kLoadingFallbackLogo;
}

}  // namespace

LoadingMatchPage::LoadingMatchPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  // logos
  MatchData* matchData = new MatchData(GetMenuTask()->GetTeamID(0), GetMenuTask()->GetTeamID(1));
  GetMenuTask()->SetMatchData(matchData);
  TeamData* teamData1 = matchData->GetTeamData(0);
  TeamData* teamData2 = matchData->GetTeamData(1);
  constexpr float kTeamLogoHeight = 12.5f;
  const float teamLogoWidth = windowManager->GetWidthPercentForHeight(kTeamLogoHeight, 1.0f);

  Gui2Frame* loadingPanel =
      new Gui2Frame(windowManager, "frame_loading_match", 18, 24, 64, 52, true);
  this->AddView(loadingPanel);
  loadingPanel->Show();

  Gui2Caption* header =
      new Gui2Caption(windowManager, "main_loading_header", 18, 28, 64, 4, "MATCH DAY");
  header->SetPosition(50.0f - header->GetTextWidthPercent() * 0.5f, 28);
  this->AddView(header);
  header->Show();

  Gui2Caption* versus =
      new Gui2Caption(windowManager, "main_loading_versus", 47, 52, 6, 4, "VS");
  versus->SetPosition(50.0f - versus->GetTextWidthPercent() * 0.5f, 52);
  this->AddView(versus);
  versus->Show();

  Gui2Caption* status = new Gui2Caption(windowManager, "main_loading_status", 18, 68, 64, 3,
                                        "Preparing the pitch...");
  status->SetPosition(50.0f - status->GetTextWidthPercent() * 0.5f, 68);
  this->AddView(status);
  status->Show();

  Gui2Caption* caption1 = new Gui2Caption(windowManager, "main_loading_team1caption", 20, 35, 40, 5,
                                          teamData1->GetName());
  float w = caption1->GetTextWidthPercent();
  caption1->SetPosition(30.0f - w * 0.5f, 35);
  this->AddView(caption1);
  Gui2Image* logo1 = new Gui2Image(windowManager, "main_loading_team1logo",
                                   30.0f - teamLogoWidth * 0.5f, 48, teamLogoWidth, kTeamLogoHeight);
  this->AddView(logo1);
  logo1->LoadImage(ResolveTeamLogo(teamData1));

  Gui2Caption* caption2 = new Gui2Caption(windowManager, "main_loading_team2caption", 60, 35, 40, 5,
                                          teamData2->GetName());
  w = caption2->GetTextWidthPercent();
  caption2->SetPosition(70.0f - w * 0.5f, 35);
  this->AddView(caption2);
  Gui2Image* logo2 = new Gui2Image(windowManager, "main_loading_team2logo",
                                   70.0f - teamLogoWidth * 0.5f, 48, teamLogoWidth, kTeamLogoHeight);
  this->AddView(logo2);
  logo2->LoadImage(ResolveTeamLogo(teamData2));

  caption1->Show();
  caption2->Show();
  logo1->Show();
  logo2->Show();

  this->SetFocus();

  this->Show();

  GetMenuTask()->SetActiveJoystickID(0);
  GetMenuTask()->EnableKeyboard();
  windowManager->GetPagePath()->Clear();

  sentStartGameSignal = false;
}

LoadingMatchPage::~LoadingMatchPage() {}

void LoadingMatchPage::Process() {
  Gui2Page::Process();

  if (!sentStartGameSignal) {
    sentStartGameSignal = true;
    // Allow one rendered loading frame before the heavier match setup begins.
    EnvironmentManager::GetInstance().Pause_ms(100);
    GetMenuTask()->SetMenuAction(e_MenuAction_Game);
  }
}

void LoadingMatchPage::Close() {
  this->Exit();

  Properties properties;
  windowManager->GetPageFactory()->CreatePage((int)e_PageID_Game, properties, 0);

  delete this;
}
