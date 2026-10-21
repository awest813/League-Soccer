#ifndef _HPP_FOOTBALL_ONTHEPITCH_PENDINGCAUTIONS
#define _HPP_FOOTBALL_ONTHEPITCH_PENDINGCAUTIONS

#include <algorithm>
#include <vector>

// Players remain owned by their team for the lifetime of the match, including
// after substitution or dismissal. Separate offences may caution the same player.
template <typename PlayerType>
class PendingCautions {
public:
  void Add(PlayerType* player) {
    if (player)
      players.push_back(player);
  }

  int CountFor(PlayerType* player) const {
    return static_cast<int>(std::count(players.begin(), players.end(), player));
  }

  std::vector<PlayerType*> TakeAtStoppage(bool inPlay) {
    std::vector<PlayerType*> result;
    if (!inPlay)
      result.swap(players);
    return result;
  }

private:
  std::vector<PlayerType*> players;
};

#endif
