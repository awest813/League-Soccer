// Career squads in the 3D match: placing a starting eleven in formation slots.

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "data/squadslots.hpp"

namespace {

const std::vector<std::string> k442 = {"GK", "LB", "CB", "CB", "RB", "LM",
                                       "CM", "CM", "RM", "CF", "CF"};

TEST(SquadSlots, ExactPositionsFillTheirSlots) {
  const std::vector<std::string> players = {"CF", "CB", "GK", "RM", "CM", "LB",
                                            "CB", "CF", "CM", "RB", "LM"};
  const std::vector<int> order = squadslots::AssignToSlots(k442, players, 11);
  ASSERT_EQ(order.size(), 11u);
  for (size_t slot = 0; slot < k442.size(); ++slot)
    EXPECT_EQ(players[order[slot]], k442[slot]) << "slot " << slot;
}

TEST(SquadSlots, SameLineBeforeAnyoneAndKeepersStayInGoal) {
  // No full-backs or wide midfielders, and a second keeper among the starters.
  const std::vector<std::string> players = {"GK", "CB", "CB", "CB", "CB", "DM",
                                            "AM", "CM", "CM", "CF", "GK"};
  const std::vector<int> order = squadslots::AssignToSlots(k442, players, 11);
  ASSERT_EQ(order.size(), 11u);
  EXPECT_EQ(players[order[0]], "GK");
  for (int slot = 1; slot <= 4; ++slot)
    EXPECT_EQ(squadslots::Line(players[order[slot]]), 1) << "defensive slot " << slot;
  for (int slot = 5; slot <= 8; ++slot)
    EXPECT_EQ(squadslots::Line(players[order[slot]]), 2) << "midfield slot " << slot;
  EXPECT_EQ(players[order[9]], "CF");
  EXPECT_EQ(players[order[10]], "GK");  // forced: nobody else is left
}

TEST(SquadSlots, BenchFollowsInItsOwnOrder) {
  std::vector<std::string> players(k442.begin(), k442.end());
  players.push_back("GK");
  players.push_back("CF");
  const std::vector<int> order = squadslots::AssignToSlots(k442, players, 11);
  ASSERT_EQ(order.size(), 13u);
  EXPECT_EQ(order[11], 11);
  EXPECT_EQ(order[12], 12);
}

}  // namespace
