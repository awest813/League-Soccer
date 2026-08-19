#include "icontroller.hpp"

#include "../../match.hpp"
#include "../playerbase.hpp"

void IController::SetPlayer(PlayerBase* player) {
  this->player = player;
}

int IController::GetReactionTime_ms() {
  return int(round(80.0f - player->GetStat("physical_reaction") * 40.0f));
}
