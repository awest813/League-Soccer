#include "celebration.hpp"

CelebrationStrategy::CelebrationStrategy(ElizaController* controller) : Strategy(controller) {
  name = "celebration";
}

CelebrationStrategy::~CelebrationStrategy() {}

void CelebrationStrategy::RequestInput(const MentalImage* mentalImage, Vector3& direction,
                                       e_Velocity& velocity) {
  direction = player->GetDirectionVec();
  velocity = e_Velocity_Idle;
}
