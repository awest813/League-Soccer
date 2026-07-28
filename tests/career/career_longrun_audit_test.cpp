#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "menu/career/career_database.hpp"

namespace {

using blunted::CareerDatabase;

namespace fs = std::filesystem;

struct PersonaTier {
  const char* name;
  const char* mode;  // CreateNewCareer mode string
  int rosterOvr;     // starting average overall
  int rosterSize;
  long long transferBudget;
  long long wageBudget;
  const char* strategy;
  bool ownerLoop;        // run owner finance / board / sponsor flow
  bool heavyTraining;    // coach-style training focus
  bool activeTransfers;  // gm-style transfer activity
  unsigned int seed;
};

// Four testing personas spanning club tiers and career modes.
const PersonaTier kPersonas[] = {
    {"TierD_RelegationBattler_Manager", "manager", 58, 18, 2000000, 80000, "Defensive", false,
     false, false, 101},
    {"TierC_MidtableBuilder_Coach", "mycoach", 68, 20, 8000000, 180000, "Balanced", false, true,
     false, 202},
    {"TierB_TitleChallenger_GM", "mygm", 76, 22, 25000000, 350000, "Attacking", false, false, true,
     303},
    {"TierA_EliteDynasty_Owner", "owner", 86, 24, 60000000, 600000, "Balanced", true, false, true,
     404},
};

const int kSeasons = 12;
const int kMatchesPerSeason = 38;

std::string PersonaTempDir(const PersonaTier& persona) {
  fs::path dir = fs::temp_directory_path() / ("league_soccer_audit_" + std::string(persona.name));
  fs::remove_all(dir);
  fs::create_directories(dir);
  return dir.string();
}

void SeedRoster(CareerSave* save, int ovr, int size) {
  ASSERT_NE(save, nullptr);
  save->roster.clear();
  static const char* positions[] = {"GK", "CB", "CB", "LB", "RB", "DM",
                                    "CM", "CM", "AM", "CF", "ST"};
  for (int i = 0; i < size; ++i) {
    PlayerCareerState p;
    p.name = "Audit Player " + std::to_string(i);
    p.position = positions[i % 11];
    p.preferredPosition = p.position;
    // Spread overalls around the tier target so growth/decline has room.
    p.ovr = std::max(40, std::min(94, ovr - 4 + (i % 9)));
    p.pot = std::min(99, p.ovr + 8 + (i % 5));
    p.age = 19 + (i % 14);
    p.morale = 65 + (i % 20);
    p.matchForm = 45 + (i % 30);
    p.fitness = 90;
    p.contract.yearsRemaining = 2 + (i % 4);
    p.value = static_cast<long long>(p.ovr) * p.ovr * 4000;
    p.wage = std::max(500LL, p.value / 1200LL);
    save->roster.push_back(p);
  }
}

int AverageOvr(const CareerSave* save) {
  if (!save || save->roster.empty())
    return 0;
  int sum = 0;
  for (const auto& p : save->roster)
    sum += p.ovr;
  return sum / static_cast<int>(save->roster.size());
}

void RunPersonaSeasonActions(CareerDatabase& db, const PersonaTier& persona, int seasonIndex) {
  CareerSave* save = db.GetActiveSave();
  ASSERT_NE(save, nullptr);

  db.SetStrategy(persona.strategy);

  if (persona.heavyTraining) {
    save->trainingPoints = 10;
    db.TrainFocus("Attacking");
    db.TrainFocus("Defending");
    while (db.TrainSquad()) {
    }
  } else {
    save->trainingPoints = 4;
    db.TrainSquad();
    db.TrainSquad();
  }

  // Youth intake every other season keeps academy / promote paths exercised.
  if (seasonIndex % 2 == 0) {
    db.ScoutYouthPlayer();
    if (!save->youthAcademy.empty()) {
      db.PromoteYouthPlayer(save->youthAcademy.front().name);
    }
  }

  if (persona.activeTransfers) {
    db.PopulateTransferMarket();
    auto targets = db.GetTransferTargets();
    if (!targets.empty()) {
      // Bid near asking price on the strongest affordable target.
      std::sort(targets.begin(), targets.end(),
                [](const TransferTarget& a, const TransferTarget& b) {
                  return a.overallRating > b.overallRating;
                });
      for (const auto& target : targets) {
        long long bid = target.askingPrice;
        long long fee = std::max(25000LL, bid / 20);
        if (bid + fee <= save->transferBudget && target.wage <= save->wageBudget) {
          db.PlaceBid(target.name, bid, static_cast<int>(target.wage), 3);
          db.ProcessPendingBids();
          db.CompleteTransfer(target.name);
          break;
        }
      }
    }
  }

  if (persona.ownerLoop) {
    if (!save->availableSponsorOffers.empty()) {
      db.AcceptSponsorDeal(0);
    }
    if (save->fanBase < 60 && save->finances.netWorth > 2000000) {
      db.InvestInFanBase(2000000);
    }
    if (seasonIndex == 2 && !save->stadium.availableUpgrades.empty()) {
      db.UpgradeStadium(0);
    }
  }
}

void CloseSeasonLikeUi(CareerDatabase& db, const PersonaTier& persona) {
  CareerSave* save = db.GetActiveSave();
  ASSERT_NE(save, nullptr);
  if (persona.ownerLoop) {
    db.ProcessSeasonFinances();
  }
  db.AdvanceSeason();
  if (persona.ownerLoop) {
    db.EvaluateBoardObjectives();
    db.GenerateSponsorOffers();
    db.GenerateBoardObjectives();
  }
}

TEST(CareerLongRunAudit, EstimateLeaguePositionBands) {
  EXPECT_EQ(CareerDatabase::EstimateLeaguePosition(30, 5, 3), 1);     // ~95 pts
  EXPECT_EQ(CareerDatabase::EstimateLeaguePosition(24, 8, 6), 2);     // ~80 pts
  EXPECT_EQ(CareerDatabase::EstimateLeaguePosition(23, 4, 11), 4);    // 73 pts
  EXPECT_EQ(CareerDatabase::EstimateLeaguePosition(12, 12, 14), 12);  // ~48 pts
  EXPECT_EQ(CareerDatabase::EstimateLeaguePosition(5, 8, 25), 19);    // ~23 pts
  EXPECT_EQ(CareerDatabase::EstimateLeaguePosition(0, 0, 0), 10);
}

TEST(CareerLongRunAudit, ApplyMatchResultDrawIsReputationNeutral) {
  CareerDatabase& db = CareerDatabase::GetInstance();
  db.Initialize(PersonaTempDir(kPersonas[0]));
  ASSERT_TRUE(db.CreateNewCareer("Draw Club", "manager", "Tester"));
  CareerSave* save = db.GetActiveSave();
  ASSERT_NE(save, nullptr);
  SeedRoster(save, 70, 11);
  const int repBefore = save->reputation;
  const int confBefore = save->boardConfidence;
  db.ApplyMatchResult(1, 1, "Rival FC");
  EXPECT_EQ(save->seasonDraws, 1);
  EXPECT_EQ(save->seasonWins, 0);
  EXPECT_EQ(save->seasonLosses, 0);
  EXPECT_EQ(save->reputation, repBefore);
  EXPECT_EQ(save->boardConfidence, confBefore);
}

TEST(CareerLongRunAudit, TwelveSeasonsFourPersonaTiers) {
  struct PersonaReport {
    std::string name;
    int finalSeason = 0;
    double avgFinish = 0.0;
    double avgWins = 0.0;
    int bestFinish = 20;
    int titles = 0;
    int finalOvr = 0;
    long long finalTransferBudget = 0;
    int finalReputation = 0;
    int finalBoardConfidence = 0;
  };
  std::vector<PersonaReport> reports;

  for (const PersonaTier& persona : kPersonas) {
    CareerDatabase& db = CareerDatabase::GetInstance();
    db.Initialize(PersonaTempDir(persona));
    db.SeedRng(persona.seed);

    ASSERT_TRUE(db.CreateNewCareer(persona.name, persona.mode, persona.name)) << persona.name;
    CareerSave* save = db.GetActiveSave();
    ASSERT_NE(save, nullptr) << persona.name;

    SeedRoster(save, persona.rosterOvr, persona.rosterSize);
    save->transferBudget = persona.transferBudget;
    save->wageBudget = persona.wageBudget;
    save->finance.transferBudget = persona.transferBudget;
    save->finance.wageBudget = persona.wageBudget;
    save->activeStrategy = persona.strategy;
    if (persona.ownerLoop) {
      save->finances.netWorth = 100000000;
      save->fanBase = 55;
      db.InitializeOwnerData();
      db.GenerateBoardObjectives();
      db.GenerateSponsorOffers();
    }

    const int startOvr = AverageOvr(save);
    EXPECT_NEAR(startOvr, persona.rosterOvr, 5) << persona.name;

    static const char* opponents[] = {
        "FC United",     "Athletic Club", "Wanderers FC",      "Real Deportivo",  "Inter Milano",
        "Bayern Munich", "FC Barcelona",  "Chelsea FC",        "Arsenal FC",      "Juventus Turin",
        "AC Milan",      "Liverpool FC",  "Borussia Dortmund", "Paris SG",        "Ajax Amsterdam",
        "Porto FC",      "Benfica",       "Sporting CP",       "Napoli",          "Atletico Madrid",
        "Tottenham",     "Rivertown FC",  "Harbor City",       "Northgate United"};

    for (int season = 0; season < kSeasons; ++season) {
      db.SeedRng(persona.seed + static_cast<unsigned int>(season) * 97u);
      RunPersonaSeasonActions(db, persona, season);

      ASSERT_NE(db.GetActiveSave(), nullptr) << persona.name << " season " << season;
      save = db.GetActiveSave();

      for (int match = 0; match < kMatchesPerSeason; ++match) {
        const std::string& opponent = opponents[match % 24];
        const bool isHome = (match % 2) == 0;
        SimulatedMatch result = db.SimulateMatchResult(opponent, std::to_string(match + 1), isHome);
        ASSERT_TRUE(result.played) << persona.name << " S" << season << " M" << match;
        EXPECT_GE(result.homeGoals, 0);
        EXPECT_LE(result.homeGoals, 9);
        EXPECT_GE(result.awayGoals, 0);
        EXPECT_LE(result.awayGoals, 7);
        db.ApplyMatchResult(result.homeGoals, result.awayGoals, opponent, result.scorers);
      }

      save = db.GetActiveSave();
      ASSERT_NE(save, nullptr);
      const int played = save->seasonWins + save->seasonDraws + save->seasonLosses;
      EXPECT_EQ(played, kMatchesPerSeason) << persona.name << " season " << (season + 1);
      EXPECT_GE(save->transferBudget, 0) << persona.name << " season " << (season + 1);
      EXPECT_GE(save->wageBudget, 0) << persona.name << " season " << (season + 1);
      EXPECT_FALSE(save->roster.empty()) << persona.name << " season " << (season + 1);

      for (const auto& player : save->roster) {
        EXPECT_GE(player.ovr, 30) << player.name;
        EXPECT_LE(player.ovr, 99) << player.name;
        EXPECT_GE(player.age, 15) << player.name;
        EXPECT_LE(player.age, 50) << player.name;
        EXPECT_GE(player.morale, 0);
        EXPECT_LE(player.morale, 100);
        EXPECT_GE(player.matchForm, 0);
        EXPECT_LE(player.matchForm, 100);
      }

      const int seasonNumberBeforeAdvance = save->season.currentSeason;
      CloseSeasonLikeUi(db, persona);
      save = db.GetActiveSave();
      ASSERT_NE(save, nullptr);
      EXPECT_EQ(save->season.currentSeason, seasonNumberBeforeAdvance + 1) << persona.name;
      EXPECT_EQ(save->currentSeason, save->season.currentSeason) << persona.name;
      ASSERT_EQ(static_cast<int>(save->history.size()), season + 1) << persona.name;

      const SeasonRecord& rec = save->history.back();
      EXPECT_EQ(rec.season, seasonNumberBeforeAdvance) << persona.name;
      EXPECT_EQ(rec.wins + rec.draws + rec.losses, kMatchesPerSeason) << persona.name;
      EXPECT_GE(rec.leaguePosition, 1);
      EXPECT_LE(rec.leaguePosition, 20);
      EXPECT_EQ(rec.wonTitle, rec.leaguePosition == 1);
      EXPECT_EQ(rec.leaguePosition,
                CareerDatabase::EstimateLeaguePosition(rec.wins, rec.draws, rec.losses))
          << persona.name << " finish must be derived from W/D/L, not random";

      // Season counters must reset after advance.
      EXPECT_EQ(save->seasonWins, 0);
      EXPECT_EQ(save->seasonDraws, 0);
      EXPECT_EQ(save->seasonLosses, 0);
      EXPECT_EQ(save->season.currentWeek, 1);

      // Mid-career save/load round-trip at season 6.
      if (season == 5) {
        ASSERT_TRUE(db.SaveCareerData()) << persona.name;
        const int seasonSnap = save->season.currentSeason;
        const size_t rosterSnap = save->roster.size();
        const size_t historySnap = save->history.size();
        ASSERT_TRUE(db.LoadCareerSave(persona.name)) << persona.name;
        save = db.GetActiveSave();
        ASSERT_NE(save, nullptr);
        EXPECT_EQ(save->season.currentSeason, seasonSnap);
        EXPECT_EQ(save->currentSeason, seasonSnap);
        EXPECT_EQ(save->roster.size(), rosterSnap);
        EXPECT_EQ(save->history.size(), historySnap);
      }
    }

    save = db.GetActiveSave();
    ASSERT_NE(save, nullptr);
    EXPECT_EQ(save->season.currentSeason, kSeasons + 1) << persona.name;
    EXPECT_EQ(static_cast<int>(save->history.size()), kSeasons) << persona.name;

    PersonaReport report;
    report.name = persona.name;
    report.finalSeason = save->season.currentSeason;
    report.finalOvr = AverageOvr(save);
    report.finalTransferBudget = save->transferBudget;
    report.finalReputation = save->reputation;
    report.finalBoardConfidence = save->boardConfidence;
    int finishSum = 0;
    int winsSum = 0;
    for (const auto& rec : save->history) {
      finishSum += rec.leaguePosition;
      winsSum += rec.wins;
      report.bestFinish = std::min(report.bestFinish, rec.leaguePosition);
      if (rec.wonTitle)
        report.titles++;
    }
    report.avgFinish = static_cast<double>(finishSum) / kSeasons;
    report.avgWins = static_cast<double>(winsSum) / kSeasons;
    reports.push_back(report);

    // Soft tier guards on the 12-season sample.
    if (std::string(persona.name).find("TierA") != std::string::npos) {
      EXPECT_LE(report.avgFinish, 6.0) << persona.name;
      EXPECT_GE(report.avgWins, 18.0) << persona.name;
    }
    if (std::string(persona.name).find("TierD") != std::string::npos) {
      EXPECT_GE(report.avgFinish, 8.0) << persona.name;
      EXPECT_LE(report.avgWins, 18.0) << persona.name;
    }

    // Emit a concise audit line for each persona (visible in test logs).
    printf(
        "[career-audit] %s | avgFinish=%.1f avgWins=%.1f best=%d titles=%d finalOvr=%d rep=%d "
        "board=%d budget=%lld\n",
        report.name.c_str(), report.avgFinish, report.avgWins, report.bestFinish, report.titles,
        report.finalOvr, report.finalReputation, report.finalBoardConfidence,
        report.finalTransferBudget);

    EXPECT_GE(save->reputation, -100);
    EXPECT_LE(save->reputation, 100);
    EXPECT_GE(save->boardConfidence, 0);
    EXPECT_LE(save->boardConfidence, 100);
  }

  ASSERT_EQ(reports.size(), 4u);
  // Stronger personas should win more matches across the 12-season sample.
  EXPECT_GT(reports[3].avgWins, reports[0].avgWins);
  EXPECT_GT(reports[2].avgWins, reports[1].avgWins);
  EXPECT_LT(reports[3].avgFinish, reports[0].avgFinish);
}

}  // namespace
