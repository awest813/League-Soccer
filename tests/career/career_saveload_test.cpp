#include <filesystem>
#include <fstream>
#include <string>

#include <gtest/gtest.h>
#include <sqlite3.h>

#include "menu/career/career_database.hpp"

namespace {

using blunted::CareerDatabase;

namespace fs = std::filesystem;

// Returns a temp directory unique to the current test so tests can run in
// parallel under ctest without racing over shared save-file paths.
std::string UniqueTempDir(const std::string& label) {
  const char* testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
  return (fs::temp_directory_path() / ("league_soccer_" + std::string(testName) + "_" + label))
      .string();
}

// Writes the given career.save contents into a fresh temp directory and returns
// the directory path for CareerDatabase::Initialize().
std::string WriteSaveFile(const std::string& contents) {
  fs::path dir = UniqueTempDir("saveload");
  fs::create_directories(dir);
  std::ofstream file(dir / "career.save", std::ios::trunc);
  file << contents;
  file.close();
  return dir.string();
}

// A save file riddled with non-numeric values in numeric fields must load
// without throwing: valid fields are kept, bad ones fall back to defaults, and
// every roster row is still parsed.
TEST(CareerSaveLoadTest, MissingSaveFileReturnsFalse) {
  fs::path dir = UniqueTempDir("missing");
  fs::remove_all(dir);
  fs::create_directories(dir);

  CareerDatabase& db = CareerDatabase::GetInstance();
  db.Initialize(dir.string());
  EXPECT_FALSE(db.HasSaveFile());
  EXPECT_FALSE(db.LoadCareerSave("Ghost Club"));
}

TEST(CareerSaveLoadTest, CorruptNumericFieldsDoNotThrow) {
  const std::string contents =
      "# Career Save: My Club\n"
      "name=My Club\n"
      "mode=1\n"
      "clubID=12\n"
      "reputation=abc\n"
      "boardConfidence=\n"
      "transferBudget=not-a-budget\n"
      "wageBudget=250000\n"
      "season=oops\n"
      "player.0=Alice|ST|notanage|88|90|1000000|5000\n"
      "player.1=Bob|GK|29|notanovr||badvalue|\n";

  CareerDatabase& db = CareerDatabase::GetInstance();
  db.Initialize(WriteSaveFile(contents));

  ASSERT_NO_THROW({ EXPECT_TRUE(db.LoadCareerSave("My Club")); });

  CareerSave* save = db.GetActiveSave();
  ASSERT_NE(save, nullptr);

  // Valid fields survive.
  EXPECT_EQ(save->name, "My Club");
  EXPECT_EQ(save->club.clubID, 12);
  EXPECT_EQ(save->wageBudget, 250000);

  // Corrupt numeric fields fall back to a default rather than crashing.
  EXPECT_EQ(save->reputation, 0);
  EXPECT_EQ(save->boardConfidence, 0);
  EXPECT_EQ(save->transferBudget, 0);
  EXPECT_EQ(save->season.currentSeason, 0);

  // Both roster rows load; bad per-player fields default, good ones survive.
  ASSERT_EQ(save->roster.size(), 2u);
  EXPECT_EQ(save->roster[0].name, "Alice");
  EXPECT_EQ(save->roster[0].position, "ST");
  EXPECT_EQ(save->roster[0].age, 0);   // "notanage" -> 0
  EXPECT_EQ(save->roster[0].ovr, 88);  // valid
  EXPECT_EQ(save->roster[1].name, "Bob");
  EXPECT_EQ(save->roster[1].ovr, 0);  // "notanovr" -> 0
}

// A well-formed save round-trips its values correctly.
TEST(CareerSaveLoadTest, ValidSaveLoadsValues) {
  const std::string contents =
      "# Career Save: United\n"
      "name=United\n"
      "clubID=7\n"
      "reputation=72\n"
      "transferBudget=5000000\n"
      "player.0=Carol|CM|24|81|86|2000000|9000\n";

  CareerDatabase& db = CareerDatabase::GetInstance();
  db.Initialize(WriteSaveFile(contents));

  ASSERT_TRUE(db.LoadCareerSave("United"));
  CareerSave* save = db.GetActiveSave();
  ASSERT_NE(save, nullptr);

  EXPECT_EQ(save->name, "United");
  EXPECT_EQ(save->club.clubID, 7);
  EXPECT_EQ(save->reputation, 72);
  EXPECT_EQ(save->transferBudget, 5000000);
  ASSERT_EQ(save->roster.size(), 1u);
  EXPECT_EQ(save->roster[0].name, "Carol");
  EXPECT_EQ(save->roster[0].age, 24);
  EXPECT_EQ(save->roster[0].ovr, 81);
  EXPECT_EQ(save->roster[0].wage, 9000);
}

// Older saves wrote only 7 player fields; they must still load, with the newer
// fields keeping their struct defaults rather than being garbage.
TEST(CareerSaveLoadTest, LegacySevenFieldPlayerLoads) {
  const std::string contents =
      "name=Old Save\n"
      "player.0=Dave|CB|30|79|79|3000000|8000\n";

  CareerDatabase& db = CareerDatabase::GetInstance();
  db.Initialize(WriteSaveFile(contents));
  ASSERT_TRUE(db.LoadCareerSave("Old Save"));

  CareerSave* save = db.GetActiveSave();
  ASSERT_NE(save, nullptr);
  ASSERT_EQ(save->roster.size(), 1u);
  EXPECT_EQ(save->roster[0].name, "Dave");
  EXPECT_EQ(save->roster[0].wage, 8000);
  // Newer fields absent in the file -> struct defaults.
  EXPECT_EQ(save->roster[0].morale, 50);
  EXPECT_EQ(save->roster[0].fitness, 100);
  EXPECT_EQ(save->roster[0].careerGoals, 0);
}

// A full save -> load round-trip must preserve player progression (goals,
// morale, etc.), not just the basic identity fields.
TEST(CareerSaveLoadTest, RoundTripPreservesPlayerProgress) {
  CareerDatabase& db = CareerDatabase::GetInstance();
  db.Initialize(WriteSaveFile(""));  // sets save dir; file overwritten on save

  ASSERT_TRUE(db.CreateNewCareer("Rovers", "manager", "Boss"));
  CareerSave* save = db.GetActiveSave();
  ASSERT_NE(save, nullptr);
  save->roster.clear();
  PlayerCareerState striker;
  striker.name = "Emma";
  striker.position = "ST";
  striker.ovr = 84;
  striker.morale = 88;
  striker.matchForm = 73;
  striker.fitness = 91;
  striker.careerGoals = 27;
  striker.careerAssists = 11;
  striker.matchesPlayed = 40;
  save->roster.push_back(striker);
  save->reputation = 64;
  save->transferBudget = 8000000;

  ASSERT_TRUE(db.SaveCareerData());

  // Reload from disk and confirm progression survived.
  ASSERT_TRUE(db.LoadCareerSave("Rovers"));
  CareerSave* loaded = db.GetActiveSave();
  ASSERT_NE(loaded, nullptr);
  ASSERT_EQ(loaded->roster.size(), 1u);
  const PlayerCareerState& e = loaded->roster[0];
  EXPECT_EQ(e.name, "Emma");
  EXPECT_EQ(e.ovr, 84);
  EXPECT_EQ(e.morale, 88);
  EXPECT_EQ(e.matchForm, 73);
  EXPECT_EQ(e.fitness, 91);
  EXPECT_EQ(e.careerGoals, 27);
  EXPECT_EQ(e.careerAssists, 11);
  EXPECT_EQ(e.matchesPlayed, 40);

  // Mirrored/derived fields are kept consistent on load.
  EXPECT_EQ(loaded->reputation, 64);
  EXPECT_EQ(loaded->club.reputation, 64);
  EXPECT_EQ(loaded->finance.transferBudget, loaded->transferBudget);
}

// The broader career state (free agents, youth, staff, sponsors, events, inbox,
// season history, legacy stats, board objectives) must survive a round-trip.
TEST(CareerSaveLoadTest, RoundTripPreservesCareerCollections) {
  CareerDatabase& db = CareerDatabase::GetInstance();
  db.Initialize(WriteSaveFile(""));

  ASSERT_TRUE(db.CreateNewCareer("Rangers", "owner", "Chief"));
  CareerSave* save = db.GetActiveSave();
  ASSERT_NE(save, nullptr);

  // Start from a known state (CreateNewCareer seeds objectives/sponsors).
  save->freeAgents.clear();
  save->youthAcademy.clear();
  save->staff.clear();
  save->activeSponsors.clear();
  save->recentEvents.clear();
  save->inbox.clear();
  save->history.clear();
  save->boardObjectives.clear();
  save->legacyStats.clear();

  PlayerCareerState fa;
  fa.name = "Free Agent";
  fa.position = "LW";
  fa.ovr = 70;
  fa.careerGoals = 5;
  save->freeAgents.push_back(fa);

  PlayerCareerState yp;
  yp.name = "Wonder Kid";
  yp.position = "AM";
  yp.ovr = 58;
  yp.pot = 88;
  save->youthAcademy.push_back(yp);

  save->staff.push_back(StaffMember("Coach Ray", "Assistant Manager", 77, 120000, 3));
  save->activeSponsors.push_back(SponsorDeal("MegaCorp", "Main", 4000000, 2, 40));
  save->recentEvents.emplace_back("matchday", "matchday: beat rivals 3-0", 2, 123456, true);
  save->legacyStats["titles"] = 4;

  InboxItem msg;
  msg.id = 9;
  msg.subject = "Welcome";
  msg.body = "Good luck this season | stay sharp";  // pipe must be sanitized
  msg.read = false;
  msg.weekCreated = 1;
  save->inbox.push_back(msg);

  SeasonRecord rec;
  rec.season = 1;
  rec.wins = 24;
  rec.draws = 8;
  rec.losses = 6;
  rec.goalsFor = 71;
  rec.wonTitle = true;
  save->history.push_back(rec);

  OwnerBoardObjective obj;
  obj.type = OwnerObjectiveType::WIN_TITLE;
  obj.description = "Win the league";
  obj.completed = true;
  save->boardObjectives.push_back(obj);

  ASSERT_TRUE(db.SaveCareerData());
  ASSERT_TRUE(db.LoadCareerSave("Rangers"));
  CareerSave* l = db.GetActiveSave();
  ASSERT_NE(l, nullptr);

  ASSERT_EQ(l->freeAgents.size(), 1u);
  EXPECT_EQ(l->freeAgents[0].name, "Free Agent");
  EXPECT_EQ(l->freeAgents[0].careerGoals, 5);

  ASSERT_EQ(l->youthAcademy.size(), 1u);
  EXPECT_EQ(l->youthAcademy[0].name, "Wonder Kid");
  EXPECT_EQ(l->youthAcademy[0].pot, 88);

  ASSERT_EQ(l->staff.size(), 1u);
  EXPECT_EQ(l->staff[0].name, "Coach Ray");
  EXPECT_EQ(l->staff[0].skill, 77);
  EXPECT_EQ(l->staff[0].salary, 120000);

  ASSERT_EQ(l->activeSponsors.size(), 1u);
  EXPECT_EQ(l->activeSponsors[0].sponsorName, "MegaCorp");
  EXPECT_EQ(l->activeSponsors[0].annualRevenue, 4000000);

  ASSERT_EQ(l->recentEvents.size(), 1u);
  EXPECT_EQ(l->recentEvents[0].type, "matchday");
  EXPECT_TRUE(l->recentEvents[0].isMajor);

  ASSERT_EQ(l->inbox.size(), 1u);
  EXPECT_EQ(l->inbox[0].subject, "Welcome");
  EXPECT_EQ(l->inbox[0].id, 9);
  // The pipe in the body was sanitized to a space, so parsing stays intact.
  EXPECT_EQ(l->inbox[0].body.find('|'), std::string::npos);

  ASSERT_EQ(l->history.size(), 1u);
  EXPECT_EQ(l->history[0].wins, 24);
  EXPECT_TRUE(l->history[0].wonTitle);

  ASSERT_EQ(l->boardObjectives.size(), 1u);
  EXPECT_EQ(l->boardObjectives[0].type, OwnerObjectiveType::WIN_TITLE);
  EXPECT_TRUE(l->boardObjectives[0].completed);

  EXPECT_EQ(l->legacyStats["titles"], 4);
}

// The save file produced by SaveCareerData must be a genuine SQLite database
// (the SQLite magic header), matching the project's "SQLite-backed saves"
// contract rather than the legacy plain-text format.
TEST(CareerSaveLoadTest, SaveProducesSqliteContainer) {
  fs::path dir = UniqueTempDir("sqlite_format");
  fs::create_directories(dir);
  fs::remove_all(dir / "career.save");

  CareerDatabase& db = CareerDatabase::GetInstance();
  db.Initialize(dir.string());
  ASSERT_TRUE(db.CreateNewCareer("Sqlite United", "manager", "Boss"));

  const std::string path = (dir / "career.save").string();
  ASSERT_TRUE(fs::exists(path));

  std::ifstream file(path, std::ios::binary);
  ASSERT_TRUE(file.good());
  std::vector<char> header(16);
  file.read(header.data(), 16);
  // SQLite's magic header is exactly 16 bytes: "SQLite format 3" + NUL.
  const std::string magic(header.begin(), header.end());
  EXPECT_EQ(magic, std::string("SQLite format 3\0", 16));
}

// A legacy plain-text save file (no SQLite container) must still load through
// the backward-compatibility fallback.
TEST(CareerSaveLoadTest, LegacyPlainTextSaveStillLoads) {
  const fs::path dir = UniqueTempDir("legacy_text");
  fs::remove_all(dir);
  fs::create_directories(dir);
  const std::string path = (dir / "career.save").string();
  {
    std::ofstream file(path, std::ios::trunc);
    file << "# Career Save: Old Club\n"
         << "name=Old Club\n"
         << "mode=1\n"
         << "reputation=33\n"
         << "transferBudget=2000000\n"
         << "player.0=Leo|ST|26|83|90|4000000|9000|70|65|95|12|4|18\n";
  }

  CareerDatabase& db = CareerDatabase::GetInstance();
  db.Initialize(dir.string());
  ASSERT_TRUE(db.LoadCareerSave("Old Club"));
  CareerSave* save = db.GetActiveSave();
  ASSERT_NE(save, nullptr);
  EXPECT_EQ(save->name, "Old Club");
  EXPECT_EQ(save->reputation, 33);
  ASSERT_EQ(save->roster.size(), 1u);
  EXPECT_EQ(save->roster[0].name, "Leo");
  EXPECT_EQ(save->roster[0].ovr, 83);
  EXPECT_EQ(save->roster[0].careerGoals, 12);
}

}  // namespace

TEST(CareerProgressionSaveTest, SelectedPlanAndProgressSurviveReloadWithoutAwardingGrowth) {
  CareerDatabase& db = CareerDatabase::GetInstance();
  db.Initialize(WriteSaveFile(
      "name=Progress Club\nplayer.0=Prospect|ST|19|60|80|100000|500|50|50|90|0|0|0|57\n"));
  ASSERT_TRUE(db.LoadCareerSave("Progress Club"));
  EXPECT_EQ(db.GetActiveSave()->trainingPlan, CareerTrainingPlan::BALANCED);
  ASSERT_TRUE(db.SetTrainingPlan(CareerTrainingPlan::DEVELOPMENT));
  ASSERT_TRUE(db.SetTrainingPlan(CareerTrainingPlan::RECOVERY));
  ASSERT_TRUE(db.LoadCareerSave("Progress Club"));
  EXPECT_EQ(db.GetActiveSave()->trainingPlan, CareerTrainingPlan::RECOVERY);
  ASSERT_EQ(db.GetActiveSave()->roster.size(), 1u);
  EXPECT_EQ(db.GetActiveSave()->roster[0].developmentPoints, 57);
  EXPECT_EQ(db.GetActiveSave()->roster[0].ovr, 60);
  EXPECT_FALSE(db.SetTrainingPlan(static_cast<CareerTrainingPlan>(99)));
  EXPECT_EQ(db.GetActiveSave()->trainingPlan, CareerTrainingPlan::RECOVERY);
}

TEST(CareerProgressionSaveTest, InvalidSavedPlanDefaultsToBalanced) {
  CareerDatabase& db = CareerDatabase::GetInstance();
  db.Initialize(WriteSaveFile("name=Progress Club\ntrainingPlan=99\n"));
  ASSERT_TRUE(db.LoadCareerSave("Progress Club"));
  EXPECT_EQ(db.GetActiveSave()->trainingPlan, CareerTrainingPlan::BALANCED);
}

namespace {

// Exercise the real text and SQLite readers rather than only the role decoder.
TEST(CareerRoleMigrationTest, AllFiveLegacyRolesSurviveBothFormatsAndRepeatedSaves) {
  const CareerMode expected[] = {CareerMode::PLAYER, CareerMode::OWNER_GM, CareerMode::OWNER_GM,
                                 CareerMode::COACH, CareerMode::OWNER_GM};
  for (int oldRole = 0; oldRole < 5; ++oldRole) {
    for (bool sqlite : {false, true}) {
      SCOPED_TRACE(std::to_string(oldRole) + (sqlite ? " sqlite" : " text"));
      const fs::path dir = UniqueTempDir(std::to_string(oldRole) + (sqlite ? "db" : "text"));
      fs::remove_all(dir);
      fs::create_directories(dir);
      const auto path = (dir / "legacy.save").string();
      const std::string payload =
          "name=Preserved Club\nmode=" + std::to_string(oldRole) +
          "\nmanagerName=Alex\nclubID=12\ntransferBudget=1234567\nwageBudget=76543\n"
          "season=7\nweek=19\ncontrolledEntityID=901\ntrainingPoints=9\n"
          "player.0=Alex|CF|24|76|90|100000|5000\n"
          "fixture.0=1800|12|8|2|1|1\n"
          "history.0=6|12|20|10|8|60|30|2|0\n";
      if (sqlite) {
        sqlite3* db = nullptr;
        ASSERT_EQ(sqlite3_open(path.c_str(), &db), SQLITE_OK);
        const std::string sql =
            "CREATE TABLE career_meta(schema_version INTEGER,name TEXT,mode INTEGER,season "
            "INTEGER);"
            "INSERT INTO career_meta VALUES(1,'Preserved Club'," +
            std::to_string(oldRole) +
            ",7);"
            "CREATE TABLE career_payload(id INTEGER,data TEXT);";
        ASSERT_EQ(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
        sqlite3_stmt* stmt = nullptr;
        ASSERT_EQ(
            sqlite3_prepare_v2(db, "INSERT INTO career_payload VALUES(1,?)", -1, &stmt, nullptr),
            SQLITE_OK);
        sqlite3_bind_text(stmt, 1, payload.c_str(), -1, SQLITE_TRANSIENT);
        ASSERT_EQ(sqlite3_step(stmt), SQLITE_DONE);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
      } else {
        std::ofstream(path) << payload;
      }
      std::ifstream originalFile(path, std::ios::binary);
      const std::string original((std::istreambuf_iterator<char>(originalFile)), {});
      originalFile.close();
      CareerSave save;
      std::vector<TransferBid> bids;
      blunted::CareerPersistence::CareerSaveSummary summary;
      ASSERT_TRUE(blunted::CareerPersistence::ReadSummary(path, summary));
      EXPECT_EQ(summary.mode, expected[oldRole]);
      ASSERT_TRUE(blunted::CareerPersistence::Load(save, bids, path));
      EXPECT_EQ(save.mode, expected[oldRole]);
      EXPECT_EQ(save.handsOnManagement, oldRole != 0 && oldRole != 2);
      ASSERT_TRUE(blunted::CareerPersistence::Save(save, bids, path));
      ASSERT_TRUE(blunted::CareerPersistence::Save(save, bids, path));
      CareerSave reloaded;
      ASSERT_TRUE(blunted::CareerPersistence::Load(reloaded, bids, path));
      ASSERT_TRUE(blunted::CareerPersistence::ReadSummary(path, summary));
      EXPECT_EQ(summary.schemaVersion, 2);
      EXPECT_EQ(summary.mode, expected[oldRole]);
      EXPECT_EQ(reloaded.mode, expected[oldRole]);
      EXPECT_EQ(reloaded.handsOnManagement, save.handsOnManagement);
      EXPECT_EQ(reloaded.transferBudget, 1234567);
      EXPECT_EQ(reloaded.wageBudget, 76543);
      EXPECT_EQ(reloaded.season.currentSeason, 7);
      EXPECT_EQ(reloaded.season.currentWeek, 19);
      EXPECT_EQ(reloaded.controlledEntityID, 901);
      EXPECT_EQ(reloaded.trainingPoints, 9);
      ASSERT_EQ(reloaded.roster.size(), 1u);
      EXPECT_EQ(reloaded.roster[0].name, "Alex");
      ASSERT_EQ(reloaded.season.fixtures.size(), 1u);
      EXPECT_EQ(reloaded.season.fixtures[0].homeGoals, 2);
      ASSERT_EQ(reloaded.history.size(), 1u);
      EXPECT_EQ(reloaded.history[0].season, 6);
      std::ifstream backup(path + ".pre-three-modes.bak", std::ios::binary);
      const std::string preserved((std::istreambuf_iterator<char>(backup)), {});
      EXPECT_EQ(preserved, original);
    }
  }
}

TEST(CareerRoleMigrationTest, InvalidRolesAndFuturePayloadsDoNotReplaceActiveState) {
  for (const auto& fields : {"mode=notanumber\n", "mode=5\n", "mode=-1\n",
                             "formatVersion=2\nmode=1\nhandsOnManagement=1\n",
                             "formatVersion=3\nmode=player\nhandsOnManagement=0\n",
                             "formatVersion=2\nmode=owner_gm\nhandsOnManagement=maybe\n"}) {
    SCOPED_TRACE(fields);
    const auto dir = UniqueTempDir("invalid_role");
    fs::remove_all(dir);
    fs::create_directories(dir);
    const auto path = (fs::path(dir) / "invalid.save").string();
    std::ofstream(path) << "name=Invalid Club\n" << fields;
    CareerSave save;
    save.name = "Keep me";
    std::vector<TransferBid> bids(1);
    blunted::CareerPersistence::CareerSaveSummary summary;
    summary.isValid = true;
    EXPECT_FALSE(blunted::CareerPersistence::Load(save, bids, path));
    EXPECT_FALSE(blunted::CareerPersistence::ReadSummary(path, summary));
    EXPECT_FALSE(summary.isValid);
    EXPECT_EQ(save.name, "Keep me");
    EXPECT_EQ(bids.size(), 1u);
  }
}

TEST(CareerRoleMigrationTest, ThreeRolesAndLegacyAliasesCreateCorrectResponsibilities) {
  auto& db = CareerDatabase::GetInstance();
  const auto dir = UniqueTempDir("creation_roles");
  fs::remove_all(dir);
  db.Initialize(dir);
  for (const std::string alias :
       {"owner_gm", "manager", "owner", "mygm", "coach", "mycoach", "player"}) {
    SCOPED_TRACE(alias);
    ASSERT_TRUE(db.CreateNewCareer("Role Club", alias, "Alex"));
    auto* save = db.GetActiveSave();
    ASSERT_NE(save, nullptr);
    if (alias == "player") {
      EXPECT_EQ(save->mode, CareerMode::PLAYER);
      EXPECT_FALSE(CanManageClub(*save));
      EXPECT_FALSE(CanManageTeam(*save));
      EXPECT_FALSE(db.SetTrainingPlan(CareerTrainingPlan::DEVELOPMENT));
      EXPECT_FALSE(db.TrainSquad());
      EXPECT_FALSE(db.TrainFocus("Attacking"));
      EXPECT_EQ(save->trainingPoints, 10);
      EXPECT_EQ(save->trainingPlan, CareerTrainingPlan::BALANCED);
      EXPECT_FALSE(db.SetHandsOnManagement(true));
    } else if (alias == "coach" || alias == "mycoach") {
      EXPECT_EQ(save->mode, CareerMode::COACH);
      EXPECT_TRUE(CanManageTeam(*save));
      EXPECT_FALSE(CanManageClub(*save));
    } else {
      EXPECT_EQ(save->mode, CareerMode::OWNER_GM);
      EXPECT_TRUE(CanManageClub(*save));
      EXPECT_EQ(CanManageTeam(*save), alias != "mygm");
      ASSERT_TRUE(db.SetHandsOnManagement(false));
      EXPECT_FALSE(CanPlayCareerMatch(*save));
      EXPECT_FALSE(db.SetTrainingPlan(CareerTrainingPlan::DEVELOPMENT));
      ASSERT_TRUE(db.LoadCareerSlot(0));
      EXPECT_FALSE(db.GetActiveSave()->handsOnManagement);
      ASSERT_TRUE(db.SetHandsOnManagement(true));
      EXPECT_TRUE(CanPlayCareerMatch(*db.GetActiveSave()));
    }
  }
  EXPECT_FALSE(db.CreateNewCareer("Replace me", "unknown", "Nobody"));
  EXPECT_EQ(db.GetActiveSave()->name, "Role Club");
}

}  // namespace
