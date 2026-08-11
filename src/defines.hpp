// written by bastiaan konings schuiling 2008 - 2014
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not
// be used for anything important. i do not offer support, so don't ask. to be used for inspiration
// :)

#ifndef _HPP_DEFINES
#define _HPP_DEFINES

#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <algorithm>
#include <boost/intrusive_ptr.hpp>
#include <boost/signals2.hpp>
#include <boost/signals2/slot.hpp>
#include <cassert>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// MSVC's CRT provides the "safe" _s variants of some C standard library
// functions (fopen_s, strcpy_s, ...) as part of C11 Annex K. glibc / libc++
// on Linux and macOS do not implement Annex K, so provide minimal portable
// shims here for the handful of call sites that rely on them. This keeps a
// single source file compiling identically on MSVC, GCC and Clang without
// scattering #ifdef _WIN32 guards throughout the codebase.
#ifndef _MSC_VER
#include <cerrno>

inline int fopen_s(FILE** file, const char* filename, const char* mode) {
  *file = std::fopen(filename, mode);
  return *file ? 0 : errno;
}

inline int strcpy_s(char* dest, size_t destSize, const char* src) {
  if (!dest || !src || destSize == 0)
    return EINVAL;
  std::strncpy(dest, src, destSize - 1);
  dest[destSize - 1] = '\0';
  return 0;
}
#endif

namespace blunted {

using real = float;

inline bool GetLocalTime(std::time_t timestamp, std::tm& result) {
#ifdef _MSC_VER
  return localtime_s(&result, &timestamp) == 0;
#else
  return localtime_r(&timestamp, &result) != nullptr;
#endif
}

}  // namespace blunted

#endif
