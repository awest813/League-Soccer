#ifndef _HPP_STRATEGY_DEFAULT_OFFENSE
#define _HPP_STRATEGY_DEFAULT_OFFENSE

#include "../strategy.hpp"

class DefaultOffenseStrategy : public Strategy {
public:
  DefaultOffenseStrategy(ElizaController* controller);
  virtual ~DefaultOffenseStrategy();

  virtual void RequestInput(const MentalImage* mentalImage, Vector3& direction, float& velocity);

protected:
};

#endif
