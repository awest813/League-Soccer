// written by bastiaan konings schuiling 2008 - 2014
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not be used for anything important.
// i do not offer support, so don't ask. to be used for inspiration :)

#ifndef _HPP_UTILS_DIRECTORYPARSER
#define _HPP_UTILS_DIRECTORYPARSER

#include "defines.hpp"





#include "base/log.hpp"

namespace fs = std::filesystem;

namespace blunted {

  class DirectoryParser {

    public:
      DirectoryParser();
      virtual ~DirectoryParser();

      void Parse(std::filesystem::path path, const std::string &extension, std::vector<std::string> &files, bool recurse = true);

    protected:

  };

}

#endif
