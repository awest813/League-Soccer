#include "graphics_object.hpp"

namespace blunted {

GraphicsObject::GraphicsObject(GraphicsScene* graphicsScene) : graphicsScene(graphicsScene) {}

GraphicsObject::~GraphicsObject() {}

GraphicsScene* GraphicsObject::GetGraphicsScene() {
  return graphicsScene;
}

}  // namespace blunted
