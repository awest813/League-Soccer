#include <functional>
#include <string>

#include <gtest/gtest.h>

#include "menu/career/career_database.hpp"

namespace {

using blunted::CareerDatabase;
using blunted::SimulatedMatch;

// Builds an in-memory career save with a roster of identical-rated players.
// CreateNewCareer does no file I/O when the database is uninitialized, so this
// is safe to call repeatedly inside tests.
void SetupRoster(int playerOvr, int size = 11) {
  CareerDatabase& db = CareerDatabase::GetInstance();
  db.CreateNewCareer("Test Club", "manager", "Tester");
  CareerSave* save = db.GetActiveSave();
  ASSERT_NE(save, nullptr);
  save->roster.clear();
  for (int i = 0; i < size; ++i) {
    PlayerCareerState p;
    p.name = "Player " + std::to_string(i);
    p.ovr = playerOvr;
    p.morale = 70;
    p.matchForm = 50;
    p.position = (i == 0) ? "GK" : "MF";
    p.preferredPosition = p.position;
    save->roster.push_back(p);
  }
  save->activeStrategy = "Balanced";
}

// Mirrors the opponent-rating formula in SimulateMatchResult so tests can pick
// opponents of known relative strength.
int OpponentRatingForName(const std::string& name) {
  int seed = static_cast<int>(std::hash<std::string>{}(name) % 1000);
  return 45 + (seed % 44);
}

TEST(CareerSimTest, SeededResultsAreReproducible) {
  CareerDatabase& db = CareerDatabase::GetInstance();

  SetupRoster(75);
  db.SeedRng(42);
  SimulatedMatch first = db.SimulateMatchResult("Rivertown FC", "12345");

  SetupRoster(75);
  db.SeedRng(42);
  SimulatedMatch second = db.SimulateMatchResult("Rivertown FC", "12345");

  EXPECT_EQ(first.homeGoals, second.homeGoals);
  EXPECT_EQ(first.awayGoals, second.awayGoals);
  EXPECT_EQ(first.homeShots, second.homeShots);
  EXPECT_EQ(first.awayShots, second.awayShots);
  EXPECT_EQ(first.homePossession, second.homePossession);
  EXPECT_EQ(first.scorers, second.scorers);
}

TEST(CareerSimTest, ResultFieldsStayWithinBounds) {
  CareerDatabase& db = CareerDatabase::GetInstance();
  SetupRoster(70);

  for (unsigned int seed = 1; seed <= 200; ++seed) {
    db.SeedRng(seed);
    SimulatedMatch m = db.SimulateMatchResult("Opponent " + std::to_string(seed), "999");

    EXPECT_TRUE(m.played);
    EXPECT_GE(m.homeGoals, 0);
    EXPECT_LE(m.homeGoals, 9);
    EXPECT_GE(m.awayGoals, 0);
    EXPECT_LE(m.awayGoals, 7);
    EXPECT_GE(m.homePossession, 30);
    EXPECT_LE(m.homePossession, 70);
    EXPECT_GE(m.homeShots, m.homeGoals);
    EXPECT_GE(m.awayShots, m.awayGoals);
    EXPECT_EQ(static_cast<int>(m.scorers.size()), m.homeGoals);
  }
}

// Regression guard for the conceded-goals fix: a stronger side (which has a
// stronger defense) must concede fewer goals than a weak side against the same
// opponent. The pre-fix model inverted this and would fail here.
TEST(CareerSimTest, StrongerTeamConcedesFewerGoals) {
  CareerDatabase& db = CareerDatabase::GetInstance();
  const int kRuns = 400;

  int strongConceded = 0;
  for (unsigned int seed = 1; seed <= static_cast<unsigned int>(kRuns); ++seed) {
    SetupRoster(90);
    db.SeedRng(seed);
    strongConceded += db.SimulateMatchResult("Neutral Opponent", "100").awayGoals;
  }

  int weakConceded = 0;
  for (unsigned int seed = 1; seed <= static_cast<unsigned int>(kRuns); ++seed) {
    SetupRoster(50);
    db.SeedRng(seed);
    weakConceded += db.SimulateMatchResult("Neutral Opponent", "100").awayGoals;
  }

  EXPECT_LT(strongConceded, weakConceded);
}

// Regression guard for the opponent-variety fix: opponent strength is derived
// from the opponent's identity, so a stronger opponent must score more against
// the same team. The pre-fix model used a constant opponent rating.
TEST(CareerSimTest, StrongerOpponentScoresMore) {
  CareerDatabase& db = CareerDatabase::GetInstance();

  std::string weakName;
  std::string strongName;
  int lowestRating = 1000;
  int highestRating = -1;
  for (int i = 0; i < 300; ++i) {
    std::string name = "Club " + std::to_string(i);
    int rating = OpponentRatingForName(name);
    if (rating < lowestRating) {
      lowestRating = rating;
      weakName = name;
    }
    if (rating > highestRating) {
      highestRating = rating;
      strongName = name;
    }
  }
  ASSERT_LT(lowestRating, highestRating);

  const int kRuns = 400;
  int concededVsWeak = 0;
  int concededVsStrong = 0;
  for (unsigned int seed = 1; seed <= static_cast<unsigned int>(kRuns); ++seed) {
    SetupRoster(70);
    db.SeedRng(seed);
    concededVsWeak += db.SimulateMatchResult(weakName, "0").awayGoals;
  }
  for (unsigned int seed = 1; seed <= static_cast<unsigned int>(kRuns); ++seed) {
    SetupRoster(70);
    db.SeedRng(seed);
    concededVsStrong += db.SimulateMatchResult(strongName, "0").awayGoals;
  }

  EXPECT_GT(concededVsStrong, concededVsWeak);
}

// A non-numeric opponent team id must not throw (the parse is guarded).
TEST(CareerSimTest, NonNumericTeamIdDoesNotThrow) {
  CareerDatabase& db = CareerDatabase::GetInstance();
  SetupRoster(70);
  db.SeedRng(7);
  EXPECT_NO_THROW({
    SimulatedMatch m = db.SimulateMatchResult("", "not-a-number");
    EXPECT_TRUE(m.played);
  });
}

}  // namespace
