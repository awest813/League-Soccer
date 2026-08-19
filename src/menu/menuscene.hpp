#ifndef _HPP_MENUSCENE
#define _HPP_MENUSCENE

#include "managers/environmentmanager.hpp"
#include "scene/objects/camera.hpp"
#include "scene/objects/geometry.hpp"
#include "scene/objects/light.hpp"
#include "scene/objects/sound.hpp"
#include "scene/scene3d/node.hpp"

using namespace blunted;

struct MenuSceneLocation {
  MenuSceneLocation() {
    position = Vector3(0.0f, 0.0f, 1.0f);
    orientation = Quaternion(QUATERNION_IDENTITY);
    timeStamp_ms = EnvironmentManager::GetInstance().GetTime_ms();
  }
  Vector3 position;
  Quaternion orientation;
  unsigned long timeStamp_ms;
};

class MenuScene {
public:
  MenuScene();
  virtual ~MenuScene();

  void Get();
  void Process();
  void Put();

  void RandomizeTargetLocation();
  void SetTargetLocation(const Vector3& position, radian angle);
  void SetTargetLocation(const Vector3& position, const Quaternion& orientation);

  void PlayClickSound();
  void PlayHoverSound();

protected:
  boost::intrusive_ptr<Node> containerNode;
  boost::intrusive_ptr<Camera> camera;
  boost::intrusive_ptr<Light> mainLight;
  boost::intrusive_ptr<Geometry> geom;

  boost::intrusive_ptr<Sound> clickSound;
  boost::intrusive_ptr<Sound> hoverSound;

  boost::intrusive_ptr<Light> hoverLights[3];
  Vector3 hoverLightPosition;

  std::shared_ptr<Scene3D> scene3D;

  MenuSceneLocation sourceLocation;
  MenuSceneLocation targetLocation;

  Vector3 currentPosition;
  Quaternion currentOrientation;

  bool seamless;
};

#endif
