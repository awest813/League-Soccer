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

  EXPECT_NEAR(state.momentum.GetLength(), 9.85f, kEpsilon);
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
  EXPECT_NEAR(state.momentum.coords[2], 6.14f, kEpsilon);
  EXPECT_NEAR(interaction.frictionFactor, 9.5f / 12.0f, kEpsilon);
}

TEST(BallPhysicsTest, GroundFrictionSlowsRollingBall) {
  BallPhysicsConfig config;
  config.gravity = 0.0f;
  config.drag = 0.0f;

  BallPhysicsState state{
      blunted::Vector3(0.0f, 0.0f, config.ballRadius), blunted::Vector3(5.0f, 0.0f, 0.0f)};
  const BallGroundInteraction interaction = ApplyBallMotionForces(state, config, 0.1f);

  EXPECT_NEAR(state.momentum.coords[0], 4.74f, kEpsilon);
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

}  // namespace
