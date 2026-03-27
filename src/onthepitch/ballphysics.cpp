#include "ballphysics.hpp"

#include <algorithm>
#include <cmath>

#include "base/math/bluntmath.hpp"

BallGroundInteraction ApplyBallMotionForces(BallPhysicsState& state, const BallPhysicsConfig& config,
                                            float timeStep_s) {
  BallGroundInteraction interaction;

  state.momentum.coords[2] = state.momentum.coords[2] + config.gravity * timeStep_s;

  const float momentumVelo = state.momentum.GetLength();
  const float momentumVeloDragged =
      momentumVelo - config.drag * std::pow(momentumVelo, 2.0f) * timeStep_s;
  state.momentum = state.momentum.GetNormalized(0) * momentumVeloDragged;

  const float ballBottom = state.position.coords[2] - config.ballRadius;
  interaction.grassInfluenceBias =
      blunted::clamp(1.0f - (ballBottom / config.grassHeight), 0.0f, 1.0f);
  interaction.grassInfluenceBias = std::pow(interaction.grassInfluenceBias, 0.7f);

  if (state.position.coords[2] < config.ballRadius) {
    if (state.momentum.coords[2] < 0.0f) {
      interaction.frictionFactor =
          blunted::NormalizedClamp(-state.momentum.coords[2] - 0.5f, 0.0f, 12.0f);
      state.momentum.coords[2] = -state.momentum.coords[2] * config.bounce;
      state.momentum.coords[2] =
          std::max(state.momentum.coords[2] - config.linearBounce, 0.0f);
    }

    state.position.coords[2] = config.ballRadius;
  }

  if (state.position.coords[2] < config.ballRadius + config.grassHeight) {
    const float adaptedFriction = config.friction * interaction.grassInfluenceBias;
    blunted::Vector3 xy = state.momentum.Get2D();
    const float velo = xy.GetLength();

    float newVelo = velo - adaptedFriction * std::pow(velo, 2.0f) * timeStep_s;
    newVelo = blunted::clamp(newVelo - (config.linearFriction * interaction.grassInfluenceBias *
                                        timeStep_s),
                             0.0f, 100000.0f);

    xy.Normalize(blunted::Vector3(0));
    xy *= newVelo;
    state.momentum.coords[0] = xy.coords[0];
    state.momentum.coords[1] = xy.coords[1];
  }

  return interaction;
}
