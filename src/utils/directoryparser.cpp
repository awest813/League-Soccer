#include "directoryparser.hpp"

#include "base/log.hpp"

namespace fs = std::filesystem;

namespace blunted {

DirectoryParser::DirectoryParser() {}

DirectoryParser::~DirectoryParser() {}

void DirectoryParser::Parse(std::filesystem::path path, const std::string& extension,
                            std::vector<std::string>& files, bool recurse) {
  if (!fs::exists(path) || !fs::is_directory(path)) {
    Log(e_Error, "DirectoryParser", "Parse",
        "Could not open directory " + path.string() + " for reading");
    return;
  }

  std::error_code ec;
  fs::directory_iterator dirIter(path, ec);
  if (ec) {
    Log(e_Error, "DirectoryParser", "Parse",
        "Could not iterate directory " + path.string() + ": " + ec.message());
    return;
  }
  fs::directory_iterator endIter;
  while (dirIter != endIter) {
    if (is_directory(dirIter->status())) {
      if (recurse) {
        std::filesystem::path thePath(path);
        thePath /= dirIter->path().filename();
        Parse(thePath, extension, files);
      }

    } else {
      std::filesystem::path thePath(path);
      thePath /= dirIter->path().filename();

      if (thePath.extension() == "." + extension) {
        // add to results
        // printf("adding %s\n", dirIter->path().filename().c_str());
        files.push_back(path.string() + "/" + thePath.filename().string());
      }
    }

    dirIter.increment(ec);
    if (ec) {
      Log(e_Error, "DirectoryParser", "Parse",
          "Could not continue iterating directory " + path.string() + ": " + ec.message());
      return;
    }
  }
}

}  // namespace blunted
