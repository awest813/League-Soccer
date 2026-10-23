// A writable working directory for read-only installs.
//
// The game reads its assets and writes its config and saves relative to the
// working directory. That fails inside a macOS app bundle: a downloaded app
// runs from a read-only, randomised copy (App Translocation), and writing
// into the bundle would break its signature anyway. PrepareUserDataDirectory
// builds a per-user directory that looks like the install: directories
// (media, databases, locale, data) are symlinks to the read-only originals,
// recreated on every launch because the bundle's path can change, and files
// (football.config) are copied once so the user's settings survive updates.
// Saves (saves/, user/) are created there by the game as usual.

#ifndef _HPP_UTILS_USERDATA
#define _HPP_UTILS_USERDATA

#include <filesystem>
#include <string>
#include <system_error>

namespace blunted {

inline bool PrepareUserDataDirectory(const std::filesystem::path& resources,
                                     const std::filesystem::path& userDirectory,
                                     std::string* error = nullptr) {
  namespace fs = std::filesystem;
  std::error_code ec;
  auto fail = [&](const std::string& what) {
    if (error)
      *error = what + (ec ? ": " + ec.message() : std::string());
    return false;
  };

  fs::create_directories(userDirectory, ec);
  if (ec)
    return fail("cannot create " + userDirectory.string());

  for (const auto& entry : fs::directory_iterator(resources, ec)) {
    const fs::path target = userDirectory / entry.path().filename();
    const fs::file_status status = fs::symlink_status(target, ec);
    ec.clear();
    if (entry.is_directory()) {
      // A real directory there is the user's own (or an old copy); keep it.
      if (fs::is_directory(status) && !fs::is_symlink(status))
        continue;
      if (fs::is_symlink(status))
        fs::remove(target, ec);
      fs::create_directory_symlink(entry.path(), target, ec);
      if (ec)
        return fail("cannot link " + target.string());
    } else if (entry.is_regular_file() && !fs::exists(status)) {
      fs::copy_file(entry.path(), target, ec);
      if (ec)
        return fail("cannot copy " + entry.path().string());
    }
  }
  if (ec)
    return fail("cannot read " + resources.string());
  return true;
}

}  // namespace blunted

#endif
