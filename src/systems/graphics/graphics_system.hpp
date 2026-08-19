#ifndef _HPP_SYSTEMS_GRAPHICS_SYSTEM
#define _HPP_SYSTEMS_GRAPHICS_SYSTEM

#include "defines.hpp"
#include "graphics_task.hpp"
#include "managers/resourcemanager.hpp"
#include "resources/texture.hpp"
#include "resources/vertexbuffer.hpp"
#include "scene/iscene.hpp"
#include "systems/graphics/rendering/opengl_renderer3d.hpp"
#include "systems/isystem.hpp"
#include "systems/isystemscene.hpp"

namespace blunted {

class Renderer3D;

class GraphicsSystem : public ISystem {
public:
  GraphicsSystem();
  virtual ~GraphicsSystem();

  virtual void Initialize(const Properties& config) override;
  virtual void Exit() override;

  e_SystemType GetSystemType() const override;

  virtual ISystemScene* CreateSystemScene(std::shared_ptr<IScene> scene) override;

  virtual ISystemTask* GetTask() override;
  virtual Renderer3D* GetRenderer3D();

  std::shared_ptr<ResourceManager<Texture>> GetTextureResourceManager();
  std::shared_ptr<ResourceManager<VertexBuffer>> GetVertexBufferResourceManager();

  MessageQueue<Overlay2DQueueEntry>& GetOverlay2DQueue();

  void GetContextSize(int& width, int& height, int& bpp) {
    width = this->width;
    height = this->height;
    bpp = this->bpp;
  }
  Vector3 GetContextSize() {
    return Vector3(static_cast<real>(width), static_cast<real>(height), static_cast<real>(bpp));
  }

  int GetAverageFrameTime_ms(unsigned int frameCount) const {
    assert(task);
    return task->GetAverageFrameTime_ms(frameCount);
  };
  unsigned long GetLastSwapTime_ms() const {
    assert(task);
    return task->GetLastSwapTime_ms();
  }
  int GetTimeSinceLastSwap_ms() const {
    assert(task);
    return task->GetTimeSinceLastSwap_ms();
  }

  virtual std::string GetName() const { return "graphics"; }

  std::mutex getPhaseMutex;

protected:
  const e_SystemType systemType;

  Renderer3D* renderer3DTask;

  GraphicsTask* task;

  std::shared_ptr<ResourceManager<Texture>> textureResourceManager;
  std::shared_ptr<ResourceManager<VertexBuffer>> vertexBufferResourceManager;

  MessageQueue<Overlay2DQueueEntry> overlay2DQueue;

  int width, height, bpp;
};

}  // namespace blunted

#endif
