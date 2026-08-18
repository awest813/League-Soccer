#include "playerhud.hpp"

#include <algorithm>
#include <cmath>
#include <string>

#include "../../data/playerdata.hpp"
#include "../../onthepitch/humangamer.hpp"
#include "../../onthepitch/match.hpp"
#include "../../onthepitch/player/controller/humancontroller.hpp"
#include "../../onthepitch/player/player.hpp"
#include "../../onthepitch/team.hpp"
#include "utils/gui2/windowmanager.hpp"

using namespace blunted;

Gui2PlayerHUD::Gui2PlayerHUD(Gui2WindowManager* windowManager, Match* match)
    : Gui2View(windowManager, "player_hud", 0, 0, 100, 100), match(match) {
  Vector3 textColor = 255;
  Vector3 textOutlineColor = 0;

  // Left Player Panel (Team 0)
  roleCaption[0] = new Gui2Caption(windowManager, "hud_player0_role", 2, 92.5f, 5, 2.8f, "");
  roleCaption[0]->SetColor(Vector3(200, 200, 200));
  roleCaption[0]->SetOutlineColor(textOutlineColor);
  this->AddView(roleCaption[0]);
  roleCaption[0]->Show();

  playerNameCaption[0] = new Gui2Caption(windowManager, "hud_player0_name", 7, 92.5f, 15, 2.8f, "");
  playerNameCaption[0]->SetColor(textColor);
  playerNameCaption[0]->SetOutlineColor(textOutlineColor);
  this->AddView(playerNameCaption[0]);
  playerNameCaption[0]->Show();

  conditionCaption[0] = new Gui2Caption(windowManager, "hud_player0_cond", 22, 92.5f, 4, 2.8f, "");
  conditionCaption[0]->SetOutlineColor(textOutlineColor);
  this->AddView(conditionCaption[0]);
  conditionCaption[0]->Show();

  staminaImage[0] = new Gui2Image(windowManager, "hud_player0_stm", 2, 95.5f, 15, 1.5f);
  this->AddView(staminaImage[0]);
  staminaImage[0]->Show();

  powerGaugeImage[0] = new Gui2Image(windowManager, "hud_player0_power", 2, 89.5f, 15, 1.2f);
  this->AddView(powerGaugeImage[0]);
  powerGaugeImage[0]->Hide();

  // Right Player Panel (Team 1)
  roleCaption[1] = new Gui2Caption(windowManager, "hud_player1_role", 73, 92.5f, 5, 2.8f, "");
  roleCaption[1]->SetColor(Vector3(200, 200, 200));
  roleCaption[1]->SetOutlineColor(textOutlineColor);
  this->AddView(roleCaption[1]);
  roleCaption[1]->Show();

  playerNameCaption[1] =
      new Gui2Caption(windowManager, "hud_player1_name", 78, 92.5f, 15, 2.8f, "");
  playerNameCaption[1]->SetColor(textColor);
  playerNameCaption[1]->SetOutlineColor(textOutlineColor);
  this->AddView(playerNameCaption[1]);
  playerNameCaption[1]->Show();

  conditionCaption[1] = new Gui2Caption(windowManager, "hud_player1_cond", 93, 92.5f, 4, 2.8f, "");
  conditionCaption[1]->SetOutlineColor(textOutlineColor);
  this->AddView(conditionCaption[1]);
  conditionCaption[1]->Show();

  staminaImage[1] = new Gui2Image(windowManager, "hud_player1_stm", 83, 95.5f, 15, 1.5f);
  this->AddView(staminaImage[1]);
  staminaImage[1]->Show();

  powerGaugeImage[1] = new Gui2Image(windowManager, "hud_player1_power", 83, 89.5f, 15, 1.2f);
  this->AddView(powerGaugeImage[1]);
  powerGaugeImage[1]->Hide();

  for (int i = 0; i < 2; i++) {
    lastStamina[i] = -1.0f;
    lastPowerGauge[i] = -1.0f;
  }

  this->Show();
}

Gui2PlayerHUD::~Gui2PlayerHUD() {}

void Gui2PlayerHUD::Put() {
  if (!match)
    return;

  // Query human or designated player for Team 0 & Team 1
  for (int t = 0; t < 2; t++) {
    Team* team = match->GetTeam(t);
    if (!team)
      continue;

    Player* activePlayer = nullptr;
    const std::vector<HumanGamer*>& gamers = team->GetHumanGamers();
    if (!gamers.empty() && gamers[0]->GetSelectedPlayerID() >= 0) {
      activePlayer = team->GetPlayer(gamers[0]->GetSelectedPlayerID());
    }
    if (!activePlayer) {
      activePlayer = team->GetDesignatedTeamPossessionPlayer();
    }
    if (!activePlayer) {
      activePlayer = team->GetLastTouchPlayer();
    }

    auto drawBar = [](boost::intrusive_ptr<Image2D>& img, float factor, const Vector3& color) {
      if (!img)
        return;
      Vector3 imgSize = img->GetSize();
      int w = static_cast<int>(imgSize.coords[0]);
      int h = static_cast<int>(imgSize.coords[1]);
      if (w > 2 && h > 2) {
        img->DrawRectangle(0, 0, w, h, Vector3(0, 0, 0), 180);      // Background
        img->DrawRectangle(0, 0, w, 1, Vector3(0, 0, 0), 255);      // Border Top
        img->DrawRectangle(0, h - 1, w, 1, Vector3(0, 0, 0), 255);  // Border Bottom
        img->DrawRectangle(0, 0, 1, h, Vector3(0, 0, 0), 255);      // Border Left
        img->DrawRectangle(w - 1, 0, 1, h, Vector3(0, 0, 0), 255);  // Border Right
        int fillW = static_cast<int>(factor * (w - 2));
        if (fillW > 0) {
          img->DrawRectangle(1, 1, fillW, h - 2, color, 220);  // Fill
        }
        img->OnChange();
      }
    };

    if (activePlayer && activePlayer->GetPlayerData()) {
      PlayerData* pd = activePlayer->GetPlayerData();
      std::string name = pd->GetLastName();
      if (name != lastPlayerName[t]) {
        playerNameCaption[t]->SetCaption(name);
        lastPlayerName[t] = name;
      }

      std::string role = "[" + pd->GetRoleName() + "]";
      if (role != lastRole[t]) {
        roleCaption[t]->SetCaption(role);
        lastRole[t] = role;
      }

      std::string condSym = pd->GetConditionSymbol();
      if (condSym != lastCondition[t]) {
        conditionCaption[t]->SetCaption(condSym);
        conditionCaption[t]->SetColor(pd->GetConditionColor());
        lastCondition[t] = condSym;
      }

      float stamina = activePlayer->GetFatigueFactorInv();
      if (std::fabs(stamina - lastStamina[t]) > 0.02f) {
        boost::intrusive_ptr<Image2D> stmImg = staminaImage[t]->GetImage2D();
        Vector3 color = (stamina < 0.25f) ? Vector3(220, 50, 50) : Vector3(100, 220, 100);
        drawBar(stmImg, stamina, color);
        lastStamina[t] = stamina;
      }
    }

    // Update PES Power Gauge for human controller
    float gaugeFactor = 0.0f;
    if (!gamers.empty() && gamers[0]->GetHumanController()) {
      HumanController* hc = dynamic_cast<HumanController*>(gamers[0]->GetHumanController());
      if (hc) {
        gaugeFactor = hc->GetGaugeFactor();
      }
    }
    if (gaugeFactor > 0.01f) {
      if (std::fabs(gaugeFactor - lastPowerGauge[t]) > 0.02f) {
        boost::intrusive_ptr<Image2D> pwrImg = powerGaugeImage[t]->GetImage2D();
        Vector3 color;
        if (gaugeFactor < 0.5f) {
          color = Vector3(50, 240, 50);  // Green
        } else if (gaugeFactor < 0.8f) {
          color = Vector3(240, 240, 50);  // Yellow
        } else {
          color = Vector3(255, 50, 50);  // Red
        }
        drawBar(pwrImg, gaugeFactor, color);
        lastPowerGauge[t] = gaugeFactor;
      }
      powerGaugeImage[t]->Show();
    } else {
      if (lastPowerGauge[t] > 0.01f) {
        powerGaugeImage[t]->Hide();
        lastPowerGauge[t] = 0.0f;
      }
    }
  }
}
