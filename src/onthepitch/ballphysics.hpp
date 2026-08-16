#ifndef _HPP_FOOTBALL_ONTHEPITCH_BALLPHYSICS
#define _HPP_FOOTBALL_ONTHEPITCH_BALLPHYSICS

#include "base/math/vector3.hpp"

struct BallPhysicsConfig {
  float ballRadius = 0.11f;
  // PES 5/6 feel: slightly livelier bounce so the ball skips off turf
  // authentically, without feeling rubbery.
  float bounce = 0.64f;
  float linearBounce = 0.05f;
  // More aerodynamic drag: long balls and lofted passes fade realistically
  // instead of carrying flat at full pace all the way.
  float drag = 0.017f;
  // Heavier ground friction: ground passes decelerate noticeably so timing
  // matters and the receiver has to move to collect the ball.
  float friction = 0.06f;
  float linearFriction = 1.55f;
  float gravity = -9.81f;
  float grassHeight = 0.025f;

  // Weather effects (roadmap 3.8). Defaults are zero so a calm, dry pitch
  // behaves identically to the original physics.
  // Wind is an acceleration (m/s^2) applied to the ball while it is airborne;
  // it bends passes, crosses and long shots off-line.
  blunted::Vector3 wind = blunted::Vector3(0.0f, 0.0f, 0.0f);
  // Pitch wetness in [0, 1]. A wet pitch is slicker, so the ball skids and
  // retains more speed along the ground (less grass friction).
  float wetness = 0.0f;
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
