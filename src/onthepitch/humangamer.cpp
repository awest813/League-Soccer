#include "humangamer.hpp"

#include "../main.hpp"
#include "managers/resourcemanagerpool.hpp"
#include "managers/usereventmanager.hpp"
#include "scene/objectfactory.hpp"
#include "team.hpp"

HumanGamer::HumanGamer(Team* team, IHIDevice* hid, e_PlayerColor color)
    : team(team), hid(hid), playerColor(color) {
  controller = new HumanController(team->GetMatch(), hid);

  std::vector<Player*> activePlayers;
  team->GetActivePlayers(activePlayers);
  selectedPlayer = 0;
  SetSelectedPlayerID(-1);
}

HumanGamer::~HumanGamer() {
  if (Verbose())
    printf("exiting humangamer.. ");
  delete controller;
  // todo: team is being destructed at this point, cannot use its methods
  // Player *currentPlayer = team->GetPlayer(selectedPlayerID);
  if (selectedPlayer) {
    selectedPlayer->SetExternalController(0);
    selectedPlayer->SetDebug(false);
  }
  if (Verbose())
    printf("done\n");
}

int HumanGamer::GetSelectedPlayerID() const {
  if (selectedPlayer)
    return selectedPlayer->GetID();
  else
    return -1;
}

void HumanGamer::SetSelectedPlayerID(int id) {
  if (selectedPlayer) {
    if (selectedPlayer->GetID() == id)
      return;
    selectedPlayer->SetExternalController(0);
    selectedPlayer->SetDebug(false);
  }
  if (id != -1) {
    selectedPlayer = team->GetPlayer(id);
    selectedPlayer->SetExternalController(controller);
    if (team->GetID() == 0) {
      selectedPlayer->SetDebug(true);
    }
  } else {
    selectedPlayer = 0;
  }
}

void HumanGamer::PreparePutBuffers() {}

void HumanGamer::Put() {}
