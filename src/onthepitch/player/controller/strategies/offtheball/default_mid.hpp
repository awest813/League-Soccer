#ifndef _HPP_STRATEGY_DEFAULT_MIDFIELD
#define _HPP_STRATEGY_DEFAULT_MIDFIELD

#include "../strategy.hpp"

class DefaultMidfieldStrategy : public Strategy {
public:
  DefaultMidfieldStrategy(ElizaController* controller);
  virtual ~DefaultMidfieldStrategy();

  virtual void RequestInput(const MentalImage* mentalImage, Vector3& direction, float& velocity);

protected:
};

#endif
