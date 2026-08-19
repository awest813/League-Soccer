#ifndef _HPP_OFFICIALS
#define _HPP_OFFICIALS

#include "player/humanoid/animcollection.hpp"
#include <memory>

class PlayerBase;
class PlayerOfficial;
class PlayerData;

class Officials {
public:
  Officials(Match* match, boost::intrusive_ptr<Node> fullbodySourceNode,
            std::map<Vector3, Vector3>& colorCoords, boost::intrusive_ptr<Resource<Surface>> kit,
            std::shared_ptr<AnimCollection> animCollection);
  virtual ~Officials();

  void GetPlayers(std::vector<PlayerBase*>& players);
  PlayerOfficial* GetReferee() { return referee.get(); }
  PlayerOfficial* GetLinesmanNorth() { return linesmen[0].get(); }
  PlayerOfficial* GetLinesmanSouth() { return linesmen[1].get(); }

  virtual void Process();
  virtual void PreparePutBuffers(unsigned long snapshotTime_ms);
  virtual void FetchPutBuffers(unsigned long putTime_ms);
  virtual void Put();

  boost::intrusive_ptr<Geometry> GetYellowCardGeom() { return yellowCard; }
  boost::intrusive_ptr<Geometry> GetRedCardGeom() { return redCard; }

protected:
  Match* match;

  std::unique_ptr<PlayerOfficial> referee;
  std::unique_ptr<PlayerOfficial> linesmen[2];
  std::unique_ptr<PlayerData> playerData;

  boost::intrusive_ptr<Geometry> yellowCard;
  boost::intrusive_ptr<Geometry> redCard;
};

#endif
