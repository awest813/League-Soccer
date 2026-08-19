#ifndef _HPP_FOOTBALL_ONTHEPITCH_ICONTROLLER
#define _HPP_FOOTBALL_ONTHEPITCH_ICONTROLLER

#include "../../../gamedefines.hpp"
#include "../humanoid/humanoid.hpp"

class Match;
class PlayerBase;

class IController {
public:
  IController(Match* match) : match(match), fallbackController(0) {};
  virtual ~IController() {};

  virtual void RequestCommand(PlayerCommandQueue& commandQueue) = 0;
  virtual void Process() {};
  virtual Vector3 GetDirection() = 0;
  virtual float GetFloatVelocity() = 0;

  virtual void SetPlayer(PlayerBase* player);

  // for convenience
  PlayerBase* GetPlayer() { return player; }
  Match* GetMatch() { return match; }

  virtual int GetReactionTime_ms();

  void SetFallbackController(IController* controller) { fallbackController = controller; }

  virtual void Reset() = 0;

protected:
  PlayerBase* player;
  Match* match;

  IController* fallbackController;
};

#endif
