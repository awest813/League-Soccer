// Database text handling: role notation and SQL quoting.

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "data/dbroles.hpp"
#include "utils/sqltext.hpp"

namespace {

// Splitting "D C, DM" on spaces made almost every outfielder a CM in the
// match HUD and line-up screens.
TEST(DbRoles, FootballManagerNotationListsEveryPosition) {
  EXPECT_EQ(dbroles::PositionsFromDbRole("GK"), (std::vector<std::string>{"GK"}));
  EXPECT_EQ(dbroles::PositionsFromDbRole("D C, DM"), (std::vector<std::string>{"CB", "DM"}));
  EXPECT_EQ(dbroles::PositionsFromDbRole("SW, D RLC, DM"), (std::vector<std::string>{"CB", "DM"}));
  EXPECT_EQ(dbroles::PositionsFromDbRole("D/WB RL, AM RL"), (std::vector<std::string>{"RB", "RM"}));
  EXPECT_EQ(dbroles::PositionsFromDbRole("AM RLC, F C"), (std::vector<std::string>{"AM", "CF"}));
  EXPECT_EQ(dbroles::PositionsFromDbRole("D/WB/AM L"), (std::vector<std::string>{"LB"}));
  EXPECT_EQ(dbroles::PositionsFromDbRole("ST"), (std::vector<std::string>{"CF"}));
  EXPECT_EQ(dbroles::PositionsFromDbRole(""), (std::vector<std::string>{"CM"}));
  EXPECT_EQ(dbroles::PositionFromDbRole("D RC, DM"), "CB");
}

// Every SQLite error quits the game, so text must never break out of its
// literal.
TEST(SqlText, QuotesAndEscapesApostrophes) {
  EXPECT_EQ(blunted::SqlString(""), "''");
  EXPECT_EQ(blunted::SqlString("Alex"), "'Alex'");
  EXPECT_EQ(blunted::SqlString("Martin O'Neill"), "'Martin O''Neill'");
  EXPECT_EQ(blunted::SqlString("'); DROP TABLE players; --"), "'''); DROP TABLE players; --'");
}

}  // namespace
