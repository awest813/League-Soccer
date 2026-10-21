// Places a chosen starting eleven into a team's formation slots by position.
// Pure data, kept free of the database and the engine so it can be unit
// tested.

#ifndef _HPP_DATA_SQUADSLOTS
#define _HPP_DATA_SQUADSLOTS

#include <string>
#include <vector>

namespace squadslots {

// The line a position plays in: 0 goalkeeper, 1 defence, 2 midfield, 3 attack.
inline int Line(const std::string& role) {
  if (role == "GK")
    return 0;
  if (role == "CB" || role == "LB" || role == "RB")
    return 1;
  if (role == "CF" || role == "ST")
    return 3;
  return 2;
}

// Orders players for a formation. The first `starters` players are the
// starting eleven; result[i] for i < slotRoles.size() is the player for slot i
// (exact position first, then the same line, then anyone, keeping keepers out
// of outfield slots while possible). The bench follows in its original order.
inline std::vector<int> AssignToSlots(const std::vector<std::string>& slotRoles,
                                      const std::vector<std::string>& playerRoles,
                                      size_t starters) {
  const size_t count = playerRoles.size();
  if (starters > count)
    starters = count;
  std::vector<int> slotPlayer(slotRoles.size(), -1);
  std::vector<bool> used(count, false);

  auto fill = [&](auto matches) {
    for (size_t slot = 0; slot < slotRoles.size(); ++slot) {
      if (slotPlayer[slot] != -1)
        continue;
      for (size_t player = 0; player < starters; ++player) {
        if (!used[player] && matches(slotRoles[slot], playerRoles[player])) {
          slotPlayer[slot] = static_cast<int>(player);
          used[player] = true;
          break;
        }
      }
    }
  };
  fill([](const std::string& slot, const std::string& player) { return slot == player; });
  fill([](const std::string& slot, const std::string& player) {
    return Line(slot) == Line(player);
  });
  fill([](const std::string& slot, const std::string& player) {
    return (Line(slot) == 0) == (Line(player) == 0);
  });
  fill([](const std::string&, const std::string&) { return true; });

  std::vector<int> order;
  for (int player : slotPlayer) {
    if (player != -1)
      order.push_back(player);
  }
  for (size_t player = 0; player < count; ++player) {
    if (!used[player])
      order.push_back(static_cast<int>(player));
  }
  return order;
}

}  // namespace squadslots

#endif  // _HPP_DATA_SQUADSLOTS
