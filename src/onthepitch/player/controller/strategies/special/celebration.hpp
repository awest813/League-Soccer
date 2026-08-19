#ifndef _HPP_STRATEGY_CELEBRATION
#define _HPP_STRATEGY_CELEBRATION

#include "../strategy.hpp"

class CelebrationStrategy : public Strategy {
public:
  CelebrationStrategy(ElizaController* controller);
  virtual ~CelebrationStrategy();

  virtual void RequestInput(const MentalImage* mentalImage, Vector3& direction,
                            e_Velocity& velocity);

protected:
  unsigned long startTime_ms;
};

#endif
