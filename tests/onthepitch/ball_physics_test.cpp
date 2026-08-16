#include <gtest/gtest.h>

#include "onthepitch/ballphysics.hpp"

namespace {

constexpr float kEpsilon = 1e-4f;

TEST(BallPhysicsTest, GravityActsBeforeDrag) {
  BallPhysicsConfig config;
  config.drag = 0.0f;
  config.friction = 0.0f;
  config.linearFriction = 0.0f;

  BallPhysicsState state{blunted::Vector3(0.0f, 0.0f, 1.0f), blunted::Vector3(10.0f, 0.0f, 10.0f)};
  ApplyBallMotionForces(state, config, 0.1f);

  EXPECT_NEAR(state.momentum.coords[0], 10.0f, kEpsilon);
  EXPECT_NEAR(state.momentum.coords[1], 0.0f, kEpsilon);
  EXPECT_NEAR(state.momentum.coords[2], 9.019f, kEpsilon);
}

TEST(BallPhysicsTest, DragReducesAirborneSpeed) {
  BallPhysicsConfig config;
  config.gravity = 0.0f;
  config.friction = 0.0f;
  config.linearFriction = 0.0f;

  BallPhysicsState state{blunted::Vector3(0.0f, 0.0f, 1.0f), blunted::Vector3(10.0f, 0.0f, 0.0f)};
  ApplyBallMotionForces(state, config, 0.1f);

  EXPECT_NEAR(state.momentum.GetLength(), 9.83f, kEpsilon);
}

TEST(BallPhysicsTest, BounceClampsBallToGroundAndReturnsImpactStrength) {
  BallPhysicsConfig config;
  config.gravity = 0.0f;
  config.drag = 0.0f;
  config.friction = 0.0f;
  config.linearFriction = 0.0f;

  BallPhysicsState state{blunted::Vector3(0.0f, 0.0f, 0.05f), blunted::Vector3(0.0f, 0.0f, -10.0f)};
  const BallGroundInteraction interaction = ApplyBallMotionForces(state, config, 0.1f);

  EXPECT_NEAR(state.position.coords[2], config.ballRadius, kEpsilon);
  EXPECT_NEAR(state.momentum.coords[2], 6.35f, kEpsilon);
  EXPECT_NEAR(interaction.frictionFactor, 9.5f / 12.0f, kEpsilon);
}

TEST(BallPhysicsTest, GroundFrictionSlowsRollingBall) {
  BallPhysicsConfig config;
  config.gravity = 0.0f;
  config.drag = 0.0f;

  BallPhysicsState state{
      blunted::Vector3(0.0f, 0.0f, config.ballRadius), blunted::Vector3(5.0f, 0.0f, 0.0f)};
  const BallGroundInteraction interaction = ApplyBallMotionForces(state, config, 0.1f);

  EXPECT_NEAR(state.momentum.coords[0], 4.695f, kEpsilon);
  EXPECT_NEAR(state.momentum.coords[1], 0.0f, kEpsilon);
  EXPECT_NEAR(interaction.grassInfluenceBias, 1.0f, kEpsilon);
}

TEST(BallPhysicsTest, AirborneBallAvoidsGrassFriction) {
  BallPhysicsConfig config;
  config.gravity = 0.0f;
  config.drag = 0.0f;

  BallPhysicsState state{blunted::Vector3(0.0f, 0.0f, config.ballRadius + config.grassHeight + 0.05f),
                         blunted::Vector3(5.0f, 1.0f, 0.0f)};
  const BallGroundInteraction interaction = ApplyBallMotionForces(state, config, 0.1f);

  EXPECT_NEAR(state.momentum.coords[0], 5.0f, kEpsilon);
  EXPECT_NEAR(state.momentum.coords[1], 1.0f, kEpsilon);
  EXPECT_NEAR(interaction.grassInfluenceBias, 0.0f, kEpsilon);
}

TEST(BallPhysicsTest, WindBendsAirborneBall) {
  BallPhysicsConfig config;
  config.gravity = 0.0f;
  config.drag = 0.0f;
  config.friction = 0.0f;
  config.linearFriction = 0.0f;
  config.wind = blunted::Vector3(5.0f, 0.0f, 0.0f);

  // High above the grass, so the ball is fully airborne (airborneFactor == 1).
  BallPhysicsState state{blunted::Vector3(0.0f, 0.0f, 1.0f), blunted::Vector3(10.0f, 0.0f, 0.0f)};
  ApplyBallMotionForces(state, config, 0.1f);

  // Wind acceleration of 5 m/s^2 over 0.1s adds 0.5 m/s downwind.
  EXPECT_NEAR(state.momentum.coords[0], 10.5f, kEpsilon);
  EXPECT_NEAR(state.momentum.coords[1], 0.0f, kEpsilon);
}

TEST(BallPhysicsTest, WindDoesNotShoveGroundedBall) {
  BallPhysicsConfig config;
  config.gravity = 0.0f;
  config.drag = 0.0f;
  config.wind = blunted::Vector3(5.0f, 0.0f, 0.0f);

  // Resting in the grass: friction dominates and wind is shielded out, so the
  // result matches GroundFrictionSlowsRollingBall exactly.
  BallPhysicsState state{blunted::Vector3(0.0f, 0.0f, config.ballRadius),
                         blunted::Vector3(5.0f, 0.0f, 0.0f)};
  ApplyBallMotionForces(state, config, 0.1f);

  EXPECT_NEAR(state.momentum.coords[0], 4.695f, kEpsilon);
  EXPECT_NEAR(state.momentum.coords[1], 0.0f, kEpsilon);
}

TEST(BallPhysicsTest, WetPitchLetsBallSkid) {
  BallPhysicsConfig dryConfig;
  dryConfig.gravity = 0.0f;
  dryConfig.drag = 0.0f;

  BallPhysicsConfig wetConfig = dryConfig;
  wetConfig.wetness = 1.0f;

  BallPhysicsState dryState{blunted::Vector3(0.0f, 0.0f, dryConfig.ballRadius),
                            blunted::Vector3(5.0f, 0.0f, 0.0f)};
  BallPhysicsState wetState{blunted::Vector3(0.0f, 0.0f, wetConfig.ballRadius),
                            blunted::Vector3(5.0f, 0.0f, 0.0f)};

  ApplyBallMotionForces(dryState, dryConfig, 0.1f);
  ApplyBallMotionForces(wetState, wetConfig, 0.1f);

  // A soaked pitch halves ground friction, so the ball keeps more speed.
  EXPECT_NEAR(dryState.momentum.coords[0], 4.695f, kEpsilon);
  EXPECT_NEAR(wetState.momentum.coords[0], 4.8475f, kEpsilon);
  EXPECT_GT(wetState.momentum.coords[0], dryState.momentum.coords[0]);
}

}  // namespace
