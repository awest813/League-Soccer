// written by bastiaan konings schuiling 2008 - 2015
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not
// be used for anything important. i do not offer support, so don't ask. to be used for inspiration
// :)

#ifndef _HPP_FOOTBALL_ONTHEPITCH_ANIMCOLLECTION
#define _HPP_FOOTBALL_ONTHEPITCH_ANIMCOLLECTION

#include "../../../gamedefines.hpp"
#include "../../ball.hpp"
#include "scene/objects/geometry.hpp"
#include "utils/animation.hpp"
#include "utils/objectloader.hpp"

using namespace blunted;

inline radian FixAngle(radian angle);

enum e_DefString {
  e_DefString_Empty = 0,
  e_DefString_OutgoingSpecialState = 1,
  e_DefString_IncomingSpecialState = 2,
  e_DefString_SpecialVar1 = 3,
  e_DefString_SpecialVar2 = 4,
  e_DefString_Type = 5,
  e_DefString_Trap = 6,
  e_DefString_Deflect = 7,
  e_DefString_Interfere = 8,
  e_DefString_Trip = 9,
  e_DefString_ShortPass = 10,
  e_DefString_LongPass = 11,
  e_DefString_Shot = 12,
  e_DefString_Sliding = 13,
  e_DefString_Movement = 14,
  e_DefString_Special = 15,
  e_DefString_BallControl = 16,
  e_DefString_HighPass = 17,
  e_DefString_Catch = 18,
  e_DefString_OutgoingRetainState = 19,
  e_DefString_IncomingRetainState = 20,
  e_DefString_Size = 21
};

radian FixAngle(radian angle) {
  // convert engine angle into football angle (different base orientation: 'down' on y instead of
  // 'right' on x)
  radian newAngle = angle;
  newAngle += 0.5f * pi;
  return ModulateIntoRange(-pi, pi, newAngle);
}

struct CrudeSelectionQuery {
  CrudeSelectionQuery() {
    byFunctionType = false;
    byFoot = false;
    foot = e_Foot_Left;
    heedForcedFoot = false;
    strongFoot = e_Foot_Right;
    bySide = false;
    allowLastDitchAnims = false;
    byIncomingVelocity = false;
    incomingVelocity_Strict = false;
    incomingVelocity_NoDribbleToIdle = false;
    incomingVelocity_NoDribbleToSprint = false;
    incomingVelocity_ForceLinearity = false;
    byOutgoingVelocity = false;
    byPickupBall = false;
    pickupBall = true;
    byIncomingBodyDirection = false;
    incomingBodyDirection_Strict = false;
    incomingBodyDirection_ForceLinearity = false;
    byIncomingBallDirection = false;
    byOutgoingBallDirection = false;
    byTripType = false;
  }

  bool byFunctionType;
  e_FunctionType functionType;

  bool byFoot;
  e_Foot foot;

  bool heedForcedFoot;
  e_Foot strongFoot;

  bool bySide;
  Vector3 lookAtVecRel;

  bool allowLastDitchAnims;

  bool byIncomingVelocity;
  bool incomingVelocity_Strict;  // if true, allow no difference in velocity
  bool incomingVelocity_NoDribbleToIdle;
  bool incomingVelocity_NoDribbleToSprint;
  bool incomingVelocity_ForceLinearity;
  e_Velocity incomingVelocity;

  bool byOutgoingVelocity;
  e_Velocity outgoingVelocity;

  bool byPickupBall;
  bool pickupBall;

  bool byIncomingBodyDirection;
  Vector3 incomingBodyDirection;
  bool incomingBodyDirection_Strict;
  bool incomingBodyDirection_ForceLinearity;

  bool byIncomingBallDirection;
  Vector3 incomingBallDirection;

  bool byOutgoingBallDirection;
  Vector3 outgoingBallDirection;

  bool byTripType;
  int tripType;

  Properties properties;
};

struct Quadrant {
  int id;
  Vector3 position;
  e_Velocity velocity;
  radian angle;
};

void FillNodeMap(boost::intrusive_ptr<Node> targetNode,
                 std::map<const std::string, boost::intrusive_ptr<Node>>& nodeMap);

class AnimCollection {
public:
  // scene3D for debugging pilon
  AnimCollection(std::shared_ptr<Scene3D> scene3D);
  virtual ~AnimCollection();

  void Clear();
  void Load(std::filesystem::path directory);

  const std::vector<Animation*>& GetAnimations() const;

  void CrudeSelection(DataSet& dataSet, const CrudeSelectionQuery& query);

  inline Animation* GetAnim(int index) { return animations.at(index); }

  inline const Quadrant& GetQuadrant(int id) { return quadrants.at(id); }

  int GetQuadrantID(Animation* animation, const Vector3& movement, radian angle) const;

protected:
  void _PrepareAnim(Animation* animation, boost::intrusive_ptr<Node> playerNode,
                    const std::list<boost::intrusive_ptr<Object>>& bodyParts,
                    const std::map<const std::string, boost::intrusive_ptr<Node>>& nodeMap,
                    bool convertAngledDribbleToWalk = false);

  bool _CheckFunctionType(const std::string& functionType, e_FunctionType queryFunctionType) const;

  std::shared_ptr<Scene3D> scene3D;

  std::vector<Animation*> animations;
  std::vector<Quadrant> quadrants;

  std::string defString[e_DefString_Size];

  radian maxIncomingBallDirectionDeviation;
  radian maxOutgoingBallDirectionDeviation;
};

#endif
