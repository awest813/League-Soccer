// SDL_ttf and FreeType are not thread-safe, and text is drawn from both the
// game thread (menus, match messages) and the graphics thread (scoreboard
// clock, player names). Every TTF call takes this lock.

#ifndef _HPP_UTILS_TEXTRENDERLOCK
#define _HPP_UTILS_TEXTRENDERLOCK

#include <mutex>

namespace blunted {

inline std::recursive_mutex& TextRenderMutex() {
  static std::recursive_mutex mutex;
  return mutex;
}

}  // namespace blunted

#endif  // _HPP_UTILS_TEXTRENDERLOCK
