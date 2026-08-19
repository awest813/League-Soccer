#ifndef _HPP_STRATEGY
#define _HPP_STRATEGY

#include "../../../../gamedefines.hpp"
#include "../../../AIsupport/AIfunctions.hpp"
#include "../../../AIsupport/mentalimage.hpp"
#include "../../../match.hpp"
#include "../../../team.hpp"
#include "../../player.hpp"
#include "../elizacontroller.hpp"

using namespace blunted;

class Strategy {
public:
  Strategy(ElizaController* controller)
      : controller(controller),
        player(controller->GetPlayer()),
        team(controller->GetTeam()),
        match(controller->GetMatch()) {};
  virtual ~Strategy() {};

  Player* CastPlayer() { return static_cast<Player*>(player); }

  virtual void RequestInput(const MentalImage* mentalImage, Vector3& direction,
                            float& velocity) = 0;

  std::string GetName() { return name; }

protected:
  ElizaController* controller;

  std::string name;

  // for convenience
  PlayerBase* player;
  Team* team;
  Match* match;
};

#endif
