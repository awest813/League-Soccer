#ifndef _HPP_OBJECTFACTORY
#define _HPP_OBJECTFACTORY

#include "defines.hpp"
#include "object.hpp"
#include "types/singleton.hpp"

namespace blunted {

class ObjectFactory : public Singleton<ObjectFactory> {
public:
  ObjectFactory();
  virtual ~ObjectFactory();

  virtual void Exit();

  boost::intrusive_ptr<Object> CreateObject(const std::string& name, std::string objectTypeStr,
                                            std::string postfix = "_copy");
  boost::intrusive_ptr<Object> CreateObject(const std::string& name, e_ObjectType objectType,
                                            std::string postfix = "_copy");
  boost::intrusive_ptr<Object> CopyObject(boost::intrusive_ptr<Object> source,
                                          std::string postfix = "_copy");
  void RegisterPrototype(e_ObjectType objectType, boost::intrusive_ptr<Object> prototype);

protected:
  std::map<e_ObjectType, boost::intrusive_ptr<Object>> prototypes;
};

}  // namespace blunted

#endif
