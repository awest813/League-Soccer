#include "officials.hpp"

#include "../data/playerdata.hpp"
#include "../main.hpp"
#include "managers/resourcemanagerpool.hpp"
#include "player/humanoid/humanoidbase.hpp"
#include "player/playerofficial.hpp"
#include "scene/objectfactory.hpp"
#include "scene/scene3d/scene3d.hpp"
#include "utils/objectloader.hpp"

Officials::Officials(Match* match, boost::intrusive_ptr<Node> fullbodySourceNode,
                     std::map<Vector3, Vector3>& colorCoords,
                     boost::intrusive_ptr<Resource<Surface>> kit,
                     std::shared_ptr<AnimCollection>)
    : match(match) {
  ObjectLoader loader;
  boost::intrusive_ptr<Node> playerNode =
      loader.LoadObject(GetScene3D(), "media/objects/players/player.object");
  playerNode->SetName("player");
  playerNode->SetLocalMode(e_LocalMode_Absolute);

  playerData = std::unique_ptr<PlayerData>(new PlayerData());
  referee = std::make_unique<PlayerOfficial>(e_OfficialType_Referee, match, playerData.get());
  linesmen[0] = std::make_unique<PlayerOfficial>(e_OfficialType_Linesman, match, playerData.get());
  linesmen[1] = std::make_unique<PlayerOfficial>(e_OfficialType_Linesman, match, playerData.get());

  referee->Activate(playerNode, fullbodySourceNode, colorCoords, kit, match->GetAnimCollection());
  linesmen[0]->Activate(playerNode, fullbodySourceNode, colorCoords, kit,
                        match->GetAnimCollection());
  linesmen[1]->Activate(playerNode, fullbodySourceNode, colorCoords, kit,
                        match->GetAnimCollection());
  playerNode->Exit();
  playerNode.reset();

  referee->CastHumanoid()->ResetPosition(Vector3(10, -10, 0), Vector3(0));
  linesmen[0]->CastHumanoid()->ResetPosition(Vector3(25, -36.5, 0), Vector3(0));
  linesmen[1]->CastHumanoid()->ResetPosition(Vector3(-25, 36.5, 0), Vector3(0));

  boost::intrusive_ptr<Resource<GeometryData>> geometry =
      ResourceManagerPool::GetInstance()
          .GetManager<GeometryData>(e_ResourceType_GeometryData)
          ->Fetch("media/objects/officials/yellowcard.ase", true);
  yellowCard = boost::static_pointer_cast<Geometry>(
      ObjectFactory::GetInstance().CreateObject("yellowcard", e_ObjectType_Geometry));
  GetScene3D()->CreateSystemObjects(yellowCard);
  yellowCard->SetGeometryData(geometry);
  yellowCard->SetLocalMode(e_LocalMode_Absolute);
  yellowCard->SetPosition(Vector3(0, 0, -10));

  geometry = ResourceManagerPool::GetInstance()
                 .GetManager<GeometryData>(e_ResourceType_GeometryData)
                 ->Fetch("media/objects/officials/redcard.ase", true);
  redCard = boost::static_pointer_cast<Geometry>(
      ObjectFactory::GetInstance().CreateObject("redcard", e_ObjectType_Geometry));
  GetScene3D()->CreateSystemObjects(redCard);
  redCard->SetGeometryData(geometry);
  redCard->SetLocalMode(e_LocalMode_Absolute);
  redCard->SetPosition(Vector3(0, 0, -10));
}

Officials::~Officials() {
  if (Verbose())
    printf("exiting officials.. ");
  referee.reset();
  linesmen[0].reset();
  linesmen[1].reset();
  playerData.reset();

  redCard.reset();
  yellowCard.reset();
  if (Verbose())
    printf("done\n");
}

void Officials::GetPlayers(std::vector<PlayerBase*>& players) {
  players.push_back(referee.get());
  players.push_back(linesmen[0].get());
  players.push_back(linesmen[1].get());
}

void Officials::Process() {
  referee->Process();
  linesmen[0]->Process();
  linesmen[1]->Process();
}

void Officials::PreparePutBuffers(unsigned long snapshotTime_ms) {
  referee->PreparePutBuffers(snapshotTime_ms);
  linesmen[0]->PreparePutBuffers(snapshotTime_ms);
  linesmen[1]->PreparePutBuffers(snapshotTime_ms);
}

void Officials::FetchPutBuffers(unsigned long putTime_ms) {
  referee->FetchPutBuffers(putTime_ms);
  linesmen[0]->FetchPutBuffers(putTime_ms);
  linesmen[1]->FetchPutBuffers(putTime_ms);
}

void Officials::Put() {
  referee->Put();
  linesmen[0]->Put();
  linesmen[1]->Put();

  // todo: I don't think it's legal to use all those unbuffered (unmutex'ed) Getters here
  if (referee->GetCurrentFunctionType() == e_FunctionType_Special &&
      (match->GetReferee()->GetCurrentFoulType() == 2 ||
       match->GetReferee()->GetCurrentFoulType() == 3)) {
    std::string bodyPartName = "right_elbow";
    if (referee->GetCurrentAnim()->anim->GetName().find("mirror") != std::string::npos)
      bodyPartName = "left_elbow";

    const NodeMap& nodeMap = referee->GetNodeMap();
    std::map<const std::string, boost::intrusive_ptr<Node>>::const_iterator bodyPartIter =
        nodeMap.find(bodyPartName);
    if (bodyPartIter != nodeMap.end()) {
      boost::intrusive_ptr<Spatial> bodyPart = bodyPartIter->second;
      Vector3 position = bodyPart->GetDerivedPosition() +
                         bodyPart->GetDerivedRotation() * Vector3(0.04f, 0.0f, -0.25f);  // -0.4
      if (match->GetReferee()->GetCurrentFoulType() == 2) {
        yellowCard->SetPosition(position);
        yellowCard->SetRotation(bodyPart->GetDerivedRotation());
      } else {
        redCard->SetPosition(position);
        redCard->SetRotation(bodyPart->GetDerivedRotation());
      }
    }
  } else if (referee->GetPreviousFunctionType() == e_FunctionType_Special) {
    yellowCard->SetPosition(Vector3(0, 0, -10));
    redCard->SetPosition(Vector3(0, 0, -10));
  }
}
