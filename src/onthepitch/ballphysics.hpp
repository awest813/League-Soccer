#ifndef _HPP_FOOTBALL_ONTHEPITCH_BALLPHYSICS
#define _HPP_FOOTBALL_ONTHEPITCH_BALLPHYSICS

#include "base/math/vector3.hpp"

struct BallPhysicsConfig {
  float ballRadius = 0.11f;
  float bounce = 0.62f;
  float linearBounce = 0.06f;
  float drag = 0.015f;
  float friction = 0.04f;
  float linearFriction = 1.6f;
  float gravity = -9.81f;
  float grassHeight = 0.025f;
};

struct BallPhysicsState {
  blunted::Vector3 position;
  blunted::Vector3 momentum;
};

struct BallGroundInteraction {
  float frictionFactor = 0.0f;
  float grassInfluenceBias = 0.0f;
};

BallGroundInteraction ApplyBallMotionForces(BallPhysicsState& state, const BallPhysicsConfig& config,
                                            float timeStep_s);

#endif
