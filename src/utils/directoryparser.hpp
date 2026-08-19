#ifndef _HPP_UTILS_DIRECTORYPARSER
#define _HPP_UTILS_DIRECTORYPARSER

#include "base/log.hpp"
#include "defines.hpp"

namespace fs = std::filesystem;

namespace blunted {

class DirectoryParser {
public:
  DirectoryParser();
  virtual ~DirectoryParser();

  void Parse(std::filesystem::path path, const std::string& extension,
             std::vector<std::string>& files, bool recurse = true);

protected:
};

}  // namespace blunted

#endif
