#ifndef _HPP_PLAYEROFFICIAL
#define _HPP_PLAYEROFFICIAL

#include "humanoid/humanoidbase.hpp"
#include "playerbase.hpp"

enum e_OfficialType { e_OfficialType_Referee, e_OfficialType_Linesman };

class Match;
class RefereeController;

class PlayerOfficial : public PlayerBase {
public:
  PlayerOfficial(e_OfficialType officialType, Match* match, PlayerData* playerData);
  virtual ~PlayerOfficial();

  HumanoidBase* CastHumanoid();
  RefereeController* CastController();

  e_OfficialType GetOfficialType() { return officialType; }

  virtual void Activate(boost::intrusive_ptr<Node> humanoidSourceNode,
                        boost::intrusive_ptr<Node> fullbodySourceNode,
                        std::map<Vector3, Vector3>& colorCoords,
                        boost::intrusive_ptr<Resource<Surface>> kit,
                        std::shared_ptr<AnimCollection> animCollection);
  virtual void Deactivate();

  virtual void Process();
  virtual void PreparePutBuffers(unsigned long snapshotTime_ms);
  virtual void FetchPutBuffers(unsigned long putTime_ms);

protected:
  e_OfficialType officialType;
};

#endif
