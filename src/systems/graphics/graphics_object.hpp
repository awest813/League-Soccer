#ifndef _HPP_SYSTEMS_GRAPHICS_OBJECT
#define _HPP_SYSTEMS_GRAPHICS_OBJECT

#include "defines.hpp"
#include "systems/isystemobject.hpp"

namespace blunted {

class GraphicsScene;

class GraphicsObject : public ISystemObject {
public:
  GraphicsObject(GraphicsScene* graphicsScene);
  virtual ~GraphicsObject();

  virtual boost::intrusive_ptr<Interpreter> GetInterpreter(e_ObjectType objectType) = 0;

  GraphicsScene* GetGraphicsScene();

protected:
  GraphicsScene* graphicsScene;
};

}  // namespace blunted

#endif
