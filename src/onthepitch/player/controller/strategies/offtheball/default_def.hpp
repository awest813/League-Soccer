#ifndef _HPP_STRATEGY_DEFAULT_DEFENSE
#define _HPP_STRATEGY_DEFAULT_DEFENSE

#include "../strategy.hpp"

class DefaultDefenseStrategy : public Strategy {
public:
  DefaultDefenseStrategy(ElizaController* controller);
  virtual ~DefaultDefenseStrategy();

  virtual void RequestInput(const MentalImage* mentalImage, Vector3& direction, float& velocity);

protected:
};

#endif
