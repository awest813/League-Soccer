#include "exception.hpp"

#include <cstdio>

#include "base/log.hpp"

namespace blunted {

BluntedException::BluntedException(const std::string& classname, const std::string& methodname,
                                   const std::string& description) {
  printf("exception in [%s::%s] %s\n", classname.c_str(), methodname.c_str(), description.c_str());
  char errorString[256];
  std::snprintf(errorString, sizeof(errorString), "exception in [%s::%s] %s\n",
                classname.c_str(), methodname.c_str(), description.c_str());
  Log(e_FatalError, "blunted", "exception", errorString);
  exit(1);
}

}  // namespace blunted
