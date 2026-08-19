#ifndef _HPP_EXCEPTION
#define _HPP_EXCEPTION

#include <exception>

#include "defines.hpp"

namespace blunted {

class BluntedException : public std::exception {
public:
  BluntedException(const std::string& classname, const std::string& methodname,
                   const std::string& description);
  ~BluntedException() throw() {}
};

#ifndef ThrowException
#define ThrowException(classname, methodname, description) \
  throw BluntedException::BluntedException(classname, methodname, description);
#endif

}  // namespace blunted

#endif
