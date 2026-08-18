

#include "radar.hpp"

#include <cmath>
#include <SDL2/SDL.h>
#include <SDL2/SDL2_rotozoom.h>

#include "../../gamedefines.hpp"
#include "../../onthepitch/match.hpp"
#include "utils/gui2/windowmanager.hpp"

namespace blunted {
namespace {

constexpr float kRadarAspectRatio = 550.0f / 360.0f;
constexpr float kBallHeightPercent = 1.2f;
constexpr float kAvatarHeightPercent = 1.6f;

}  // namespace

Gui2Radar::Gui2Radar(Gui2WindowManager* windowManager, const std::string& name, float x_percent,
                     float y_percent, float width_percent, float height_percent, Match* match,
                     const Vector3& color1_1, const Vector3& color1_2, const Vector3& color2_1,
                     const Vector3& color2_2)
    : Gui2View(windowManager, name, x_percent, y_percent, width_percent, height_percent),
      match(match),
      color1_1(color1_1),
      color1_2(color1_2),
      color2_1(color2_1),
      color2_2(color2_2) {
  // todo: use colors!

  radarWidthPercent =
      windowManager->GetWidthPercentForHeight(height_percent, kRadarAspectRatio);
  radarXOffsetPercent = (width_percent - radarWidthPercent) * 0.5f;
  ballWidthPercent = windowManager->GetWidthPercentForHeight(kBallHeightPercent, 1.0f);
  avatarWidthPercent = windowManager->GetWidthPercentForHeight(kAvatarHeightPercent, 1.0f);

  bg = new Gui2Image(windowManager, "bg_radar", radarXOffsetPercent, 0, radarWidthPercent,
                     height_percent);
  this->AddView(bg);
  bg->LoadImage("media/menu/radar/radar.png");
  bg->Show();

  ball = new Gui2Image(windowManager, "radar_ball", 0, 0, ballWidthPercent,
                       kBallHeightPercent);
  this->AddView(ball);
  ball->LoadImage("media/menu/radar/ball.png");
  ball->Show();

  this->Show();
}

Gui2Radar::~Gui2Radar() {}

void Gui2Radar::ReloadAvatars(int teamID, unsigned int playerCount) {
  auto& avatars = (teamID == 0) ? team1avatars : team2avatars;
  std::string imgPath = (teamID == 0) ? "media/menu/radar/p1.png" : "media/menu/radar/p2.png";

  for (auto* avatar : avatars) {
    avatar->Exit();
    delete avatar;
  }
  avatars.clear();

  for (unsigned int i = 0; i < playerCount; i++) {
    Gui2Image* avatar =
        new Gui2Image(windowManager, "radar_avatar_" + int_to_str(teamID) + "_" + int_to_str(i),
                      0, 0, avatarWidthPercent, kAvatarHeightPercent);
    this->AddView(avatar);
    avatar->LoadImage(imgPath);
    avatar->Show();
    avatars.push_back(avatar);
  }
}

void Gui2Radar::Process() {}

void Gui2Radar::Put() {
  auto getRadarPos = [this](const Vector3& position, float elementWidth, float elementHeight) -> Vector3 {
    Vector3 pos2d = position * Vector3(1 / (pitchHalfW * 2), -(1 / (pitchHalfH * 2)), 0);
    pos2d = pos2d + Vector3(0.5, 0.5, 0);
    pos2d = pos2d * Vector3(0.96f, 0.96f, 0) + Vector3(0.02f, 0.02f, 0);  // margin
    return Vector3(radarXOffsetPercent + pos2d.coords[0] * radarWidthPercent - elementWidth * 0.5f,
                   pos2d.coords[1] * height_percent - elementHeight * 0.5f, 0);
  };

  Vector3 ballPos = getRadarPos(match->GetBall()->Predict(0).Get2D(), ballWidthPercent, kBallHeightPercent);
  ball->SetPosition(ballPos.coords[0], ballPos.coords[1]);
  ball->SetZPriority(1);  // ball on top

  // get player positions
  for (int t = 0; t < 2; t++) {
    std::vector<Player*> teamPlayers;
    match->GetActiveTeamPlayers(t, teamPlayers);
    auto& avatars = (t == 0) ? team1avatars : team2avatars;

    if (teamPlayers.size() != avatars.size()) {
      ReloadAvatars(t, teamPlayers.size());
    }

    for (unsigned int i = 0; i < teamPlayers.size(); i++) {
      Vector3 pos = getRadarPos(teamPlayers.at(i)->GetPosition(), avatarWidthPercent, kAvatarHeightPercent);
      avatars.at(i)->SetPosition(pos.coords[0], pos.coords[1]);
    }
  }
}

}  // namespace blunted
