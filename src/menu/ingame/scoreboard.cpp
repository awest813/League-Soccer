

#include "scoreboard.hpp"

#include "../../onthepitch/match.hpp"
#include "utils/gui2/windowmanager.hpp"

using namespace blunted;

Gui2ScoreBoard::Gui2ScoreBoard(Gui2WindowManager* windowManager, Match* match)
    : Gui2View(windowManager, "scoreboard", 2, 2, 96, 4), match(match) {
  x_percent = 2;
  y_percent = 2;
  width_percent = 96;
  height_percent = 4;

  Vector3 textColor = 255;
  Vector3 timeColor =
      windowManager->GetStyle()->GetColor(e_DecorationType_Bright2);  // Yellow/Gold for Time
  Vector3 textOutlineColor = 0;

  goalCount[0] = 0;
  goalCount[1] = 0;

  constexpr float kScoreboardBackgroundAspectRatio = 1024.0f / 64.0f;
  const float scoreboardBackgroundWidth =
      windowManager->GetWidthPercentForHeight(height_percent, kScoreboardBackgroundAspectRatio);
  const float scoreboardBackgroundX = (width_percent - scoreboardBackgroundWidth) * 0.5f;
  Gui2Image* bg = new Gui2Image(windowManager, "image_scoreboard_bg", scoreboardBackgroundX, 0,
                                scoreboardBackgroundWidth, height_percent);
  bg->LoadImage("media/menu/scoreboard_bg.png");
  this->AddView(bg);
  bg->Show();

  const float squareLogoWidth = windowManager->GetWidthPercentForHeight(height_percent, 1.0f);
  const float tvLogoWidth = windowManager->GetWidthPercentForHeight(height_percent, 2.0f);

  // Layout relative to the centered scoreboard background
  const float bgX = scoreboardBackgroundX;
  const float bgW = scoreboardBackgroundWidth;

  // 1. League logo (left edge)
  const float leagueLogoX = bgX + bgW * 0.02f;
  leagueLogo = new Gui2Image(windowManager, "game_scoreboard_leaguelogo", leagueLogoX, 0,
                             squareLogoWidth, height_percent);
  this->AddView(leagueLogo);
  leagueLogo->LoadImage("media/menu/league.png");
  leagueLogo->Show();

  // 2. Time caption
  const float timeX = leagueLogoX + squareLogoWidth + bgW * 0.015f;
  const float timeW = bgW * 0.15f;
  timeCaption = new Gui2Caption(windowManager, "game_scoreboard_timecaption", timeX, 0,
                                timeW, height_percent * 0.9f, "0:00");

  // 3. Team 1 (Home) Logo & Name
  const float team1LogoX = bgX + bgW * 0.23f;
  teamLogo[0] = new Gui2Image(windowManager, "game_scoreboard_team1logo", team1LogoX, 0,
                              squareLogoWidth, height_percent);
  this->AddView(teamLogo[0]);
  teamLogo[0]->LoadImage(match->GetTeam(0)->GetTeamData()->GetLogoUrl());
  teamLogo[0]->Show();

  const float team1NameX = team1LogoX + squareLogoWidth + bgW * 0.015f;
  const float team1NameW = bgW * 0.14f;
  teamNameCaption[0] =
      new Gui2Caption(windowManager, "game_scoreboard_team1name", team1NameX, 0,
                      team1NameW, height_percent * 0.9f, match->GetTeam(0)->GetTeamData()->GetShortName());

  // 4. Scores (centered around 50% of background width)
  const float score1X = bgX + bgW * 0.445f;
  const float score1W = bgW * 0.045f;
  goalCountCaption[0] =
      new Gui2Caption(windowManager, "game_scoreboard_team1goals", score1X, 0,
                      score1W, height_percent * 0.9f, "0");

  const float score2X = bgX + bgW * 0.51f;
  const float score2W = bgW * 0.045f;
  goalCountCaption[1] =
      new Gui2Caption(windowManager, "game_scoreboard_team2goals", score2X, 0,
                      score2W, height_percent * 0.9f, "0");

  // 5. Team 2 (Away) Name & Logo
  const float team2NameX = bgX + bgW * 0.57f;
  const float team2NameW = bgW * 0.14f;
  teamNameCaption[1] =
      new Gui2Caption(windowManager, "game_scoreboard_team2name", team2NameX, 0,
                      team2NameW, height_percent * 0.9f, match->GetTeam(1)->GetTeamData()->GetShortName());

  const float team2LogoX = team2NameX + team2NameW + bgW * 0.015f;
  teamLogo[1] = new Gui2Image(windowManager, "game_scoreboard_team2logo", team2LogoX, 0,
                              squareLogoWidth, height_percent);
  this->AddView(teamLogo[1]);
  teamLogo[1]->LoadImage(match->GetTeam(1)->GetTeamData()->GetLogoUrl());
  teamLogo[1]->Show();

  // 6. TV logo (right edge)
  const float tvLogoX = bgX + bgW - tvLogoWidth - bgW * 0.02f;
  tvLogo = new Gui2Image(windowManager, "game_scoreboard_tvlogo", tvLogoX, 0,
                         tvLogoWidth, height_percent);
  this->AddView(tvLogo);
  tvLogo->LoadImage("media/menu/tvlogo.png");
  tvLogo->Show();

  timeCaption->SetColor(timeColor);
  timeCaption->SetOutlineColor(textOutlineColor);
  teamNameCaption[0]->SetColor(textColor);
  teamNameCaption[0]->SetOutlineColor(textOutlineColor);
  teamNameCaption[1]->SetColor(textColor);
  teamNameCaption[1]->SetOutlineColor(textOutlineColor);
  goalCountCaption[0]->SetColor(textColor);
  goalCountCaption[0]->SetOutlineColor(textOutlineColor);
  goalCountCaption[1]->SetColor(textColor);
  goalCountCaption[1]->SetOutlineColor(textOutlineColor);

  this->AddView(timeCaption);
  timeCaption->Show();
  this->AddView(teamNameCaption[0]);
  teamNameCaption[0]->Show();
  this->AddView(teamNameCaption[1]);
  teamNameCaption[1]->Show();
  this->AddView(goalCountCaption[0]);
  goalCountCaption[0]->Show();
  this->AddView(goalCountCaption[1]);
  goalCountCaption[1]->Show();

  SetGoalCount(0, 0);
  SetGoalCount(1, 0);

  this->Show();
}

Gui2ScoreBoard::~Gui2ScoreBoard() {}

void Gui2ScoreBoard::GetImages(std::vector<boost::intrusive_ptr<Image2D>>& target) {
  Gui2View::GetImages(target);
}

void Gui2ScoreBoard::Redraw() {}

void Gui2ScoreBoard::SetTimeStr(const std::string& timeStr) {
  this->timeStr = timeStr;
  timeCaption->SetCaption(timeStr);
}

void Gui2ScoreBoard::SetGoalCount(int teamID, int goalCount) {
  this->goalCount[teamID] = goalCount;
  goalCountCaption[teamID]->SetCaption(int_to_str(goalCount));
}
