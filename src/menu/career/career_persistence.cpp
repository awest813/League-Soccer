#include "career_persistence.hpp"

#include <fstream>
#include <sstream>

#include "career_common.hpp"
#include "sqlite3.h"

namespace blunted {
namespace CareerPersistence {

namespace {

// Serializes the full career state (and transient bids) to the pipe-delimited,
// line-based text payload. This is the durable format stored inside the SQLite
// save file; keeping it also preserves the legacy plain-text interchange.
std::string Serialize(const CareerSave& save, const std::vector<TransferBid>& bids) {
  std::ostringstream file;
  file << "# Career Save: " << save.name << "\n";
  file << "mode=" << static_cast<int>(save.mode) << "\n";
  file << "name=" << save.name << "\n";
  file << "managerName=" << save.managerName << "\n";
  file << "clubName=" << save.club.clubName << "\n";
  file << "clubID=" << save.club.clubID << "\n";
  file << "clubLeague=" << save.club.leagueName << "\n";
  file << "reputation=" << save.reputation << "\n";
  file << "boardConfidence=" << save.boardConfidence << "\n";
  file << "transferBudget=" << save.transferBudget << "\n";
  file << "wageBudget=" << save.wageBudget << "\n";
  file << "season=" << save.season.currentSeason << "\n";
  file << "week=" << save.season.currentWeek << "\n";
  file << "strategy=" << save.activeStrategy << "\n";
  file << "fanBase=" << save.fanBase << "\n";
  file << "clubPrestige=" << save.clubPrestige << "\n";
  file << "seasonWins=" << save.seasonWins << "\n";
  file << "seasonDraws=" << save.seasonDraws << "\n";
  file << "seasonLosses=" << save.seasonLosses << "\n";
  file << "seasonGoalsFor=" << save.seasonGoalsFor << "\n";
  file << "seasonGoalsAgainst=" << save.seasonGoalsAgainst << "\n";
  file << "netWorth=" << save.finances.netWorth << "\n";
  file << "ticketPrice=" << save.finances.ticketPrice << "\n";
  file << "stadiumCapacity=" << save.stadium.capacity << "\n";
  file << "stadiumName=" << save.stadium.name << "\n";
  file << "stadiumCondition=" << save.stadium.condition << "\n";
  file << "stadiumFanSatisfaction=" << save.stadium.fanSatisfaction << "\n";
  file << "controlledEntityID=" << save.controlledEntityID << "\n";
  file << "trainingPoints=" << save.trainingPoints << "\n";
  file << "scoutingNetworkLevel=" << save.scoutingNetworkLevel << "\n";
  file << "objective=" << CareerCommon::Sanitize(save.objective) << "\n";

  file << "rosterSize=" << save.roster.size() << "\n";
  for (size_t i = 0; i < save.roster.size(); i++)
    file << "player." << i << "=" << CareerCommon::PlayerToRecord(save.roster[i]) << "\n";
  for (size_t i = 0; i < save.freeAgents.size(); i++)
    file << "freeAgent." << i << "=" << CareerCommon::PlayerToRecord(save.freeAgents[i]) << "\n";
  for (size_t i = 0; i < save.youthAcademy.size(); i++)
    file << "youth." << i << "=" << CareerCommon::PlayerToRecord(save.youthAcademy[i]) << "\n";

  for (size_t i = 0; i < save.staff.size(); i++) {
    const auto& s = save.staff[i];
    file << "staff." << i << "=" << CareerCommon::Sanitize(s.name) << "|"
         << CareerCommon::Sanitize(s.role) << "|" << s.skill << "|" << s.salary << "|"
         << s.contractYearsRemaining << "|" << s.morale << "\n";
  }
  for (size_t i = 0; i < save.activeSponsors.size(); i++) {
    const auto& s = save.activeSponsors[i];
    file << "sponsor." << i << "=" << CareerCommon::Sanitize(s.sponsorName) << "|"
         << CareerCommon::Sanitize(s.type) << "|" << s.annualRevenue << "|" << s.yearsRemaining
         << "|" << s.reputationRequirement << "\n";
  }
  for (size_t i = 0; i < save.recentEvents.size(); i++) {
    const auto& e = save.recentEvents[i];
    file << "event." << i << "=" << CareerCommon::Sanitize(e.type) << "|" << e.reputationImpact
         << "|" << e.timestamp << "|" << (e.isMajor ? 1 : 0) << "|"
         << CareerCommon::Sanitize(e.description) << "\n";
  }
  for (size_t i = 0; i < save.inbox.size(); i++) {
    const auto& m = save.inbox[i];
    file << "inbox." << i << "=" << m.id << "|" << static_cast<int>(m.type) << "|" << m.weekCreated
         << "|" << (m.read ? 1 : 0) << "|" << m.relatedPlayerID << "|" << m.relatedTeamID << "|"
         << CareerCommon::Sanitize(m.subject) << "|" << CareerCommon::Sanitize(m.body) << "\n";
  }
  for (size_t i = 0; i < save.history.size(); i++) {
    const auto& h = save.history[i];
    file << "history." << i << "=" << h.season << "|" << h.teamID << "|" << h.wins << "|" << h.draws
         << "|" << h.losses << "|" << h.goalsFor << "|" << h.goalsAgainst << "|" << h.leaguePosition
         << "|" << (h.wonTitle ? 1 : 0) << "\n";
  }
  for (size_t i = 0; i < save.boardObjectives.size(); i++) {
    const auto& o = save.boardObjectives[i];
    file << "boardObjective." << i << "=" << static_cast<int>(o.type) << "|"
         << (o.completed ? 1 : 0) << "|" << o.reputationReward << "|" << o.confidencePenalty << "|"
         << CareerCommon::Sanitize(o.description) << "\n";
  }
  for (const auto& kv : save.legacyStats)
    file << "legacy." << CareerCommon::Sanitize(kv.first) << "=" << kv.second << "\n";
  for (size_t i = 0; i < bids.size(); i++) {
    const auto& b = bids[i];
    file << "bid." << i << "=" << CareerCommon::Sanitize(b.playerName) << "|" << b.bidAmount << "|"
         << b.offeredWage << "|" << b.contractYears << "|" << b.agentFee << "|"
         << static_cast<int>(b.status) << "|" << b.negotiationRounds << "\n";
  }
  return file.str();
}

// Parses the text payload back into a CareerSave, keeping mirrored/derived
// fields consistent. Returns false only on storage-level failures; corrupt
// individual fields fall back to defaults rather than aborting.
bool Deserialize(const std::string& text, CareerSave& out, std::vector<TransferBid>& bids) {
  std::istringstream file(text);
  if (!file)
    return false;
  CareerSave fresh;
  std::vector<TransferBid> loadedBids;
  std::string line;
  while (std::getline(file, line)) {
    if (line.empty() || line[0] == '#')
      continue;
    size_t eq = line.find('=');
    if (eq == std::string::npos)
      continue;
    std::string key = line.substr(0, eq);
    std::string val = line.substr(eq + 1);
    if (key == "name")
      fresh.name = val;
    else if (key == "mode")
      fresh.mode = static_cast<CareerMode>(CareerCommon::SafeStoi(val));
    else if (key == "managerName")
      fresh.managerName = val;
    else if (key == "clubName")
      fresh.club.clubName = val;
    else if (key == "clubID")
      fresh.club.clubID = CareerCommon::SafeStoi(val);
    else if (key == "clubLeague")
      fresh.club.leagueName = val;
    else if (key == "reputation")
      fresh.reputation = CareerCommon::SafeStoi(val);
    else if (key == "boardConfidence")
      fresh.boardConfidence = CareerCommon::SafeStoi(val);
    else if (key == "transferBudget")
      fresh.transferBudget = CareerCommon::SafeStoll(val);
    else if (key == "wageBudget")
      fresh.wageBudget = CareerCommon::SafeStoll(val);
    else if (key == "season")
      fresh.season.currentSeason = CareerCommon::SafeStoi(val);
    else if (key == "week")
      fresh.season.currentWeek = CareerCommon::SafeStoi(val);
    else if (key == "strategy")
      fresh.activeStrategy = val;
    else if (key == "fanBase")
      fresh.fanBase = CareerCommon::SafeStoi(val);
    else if (key == "clubPrestige")
      fresh.clubPrestige = CareerCommon::SafeStoi(val);
    else if (key == "seasonWins")
      fresh.seasonWins = CareerCommon::SafeStoi(val);
    else if (key == "seasonDraws")
      fresh.seasonDraws = CareerCommon::SafeStoi(val);
    else if (key == "seasonLosses")
      fresh.seasonLosses = CareerCommon::SafeStoi(val);
    else if (key == "seasonGoalsFor")
      fresh.seasonGoalsFor = CareerCommon::SafeStoi(val);
    else if (key == "seasonGoalsAgainst")
      fresh.seasonGoalsAgainst = CareerCommon::SafeStoi(val);
    else if (key == "netWorth")
      fresh.finances.netWorth = CareerCommon::SafeStoll(val);
    else if (key == "ticketPrice")
      fresh.finances.ticketPrice = CareerCommon::SafeStoi(val);
    else if (key == "stadiumCapacity")
      fresh.stadium.capacity = CareerCommon::SafeStoi(val);
    else if (key == "stadiumName")
      fresh.stadium.name = val;
    else if (key == "stadiumCondition")
      fresh.stadium.condition = CareerCommon::SafeStoi(val, fresh.stadium.condition);
    else if (key == "stadiumFanSatisfaction")
      fresh.stadium.fanSatisfaction = CareerCommon::SafeStoi(val, fresh.stadium.fanSatisfaction);
    else if (key == "controlledEntityID")
      fresh.controlledEntityID = CareerCommon::SafeStoi(val);
    else if (key == "trainingPoints")
      fresh.trainingPoints = CareerCommon::SafeStoi(val, fresh.trainingPoints);
    else if (key == "scoutingNetworkLevel")
      fresh.scoutingNetworkLevel = CareerCommon::SafeStoi(val, fresh.scoutingNetworkLevel);
    else if (key == "objective")
      fresh.objective = val;
    else if (key == "rosterSize") { /* size is implied by the player.N rows */
    } else if (key.rfind("player.", 0) == 0) {
      fresh.roster.push_back(CareerCommon::PlayerFromRecord(val));
    } else if (key.rfind("freeAgent.", 0) == 0) {
      fresh.freeAgents.push_back(CareerCommon::PlayerFromRecord(val));
    } else if (key.rfind("youth.", 0) == 0) {
      fresh.youthAcademy.push_back(CareerCommon::PlayerFromRecord(val));
    } else if (key.rfind("staff.", 0) == 0) {
      std::vector<std::string> t = CareerCommon::SplitPipes(val);
      StaffMember s;
      if (t.size() > 0)
        s.name = t[0];
      if (t.size() > 1)
        s.role = t[1];
      if (t.size() > 2)
        s.skill = CareerCommon::SafeStoi(t[2], s.skill);
      if (t.size() > 3)
        s.salary = CareerCommon::SafeStoll(t[3], s.salary);
      if (t.size() > 4)
        s.contractYearsRemaining = CareerCommon::SafeStoi(t[4], s.contractYearsRemaining);
      if (t.size() > 5)
        s.morale = CareerCommon::SafeStoi(t[5], s.morale);
      fresh.staff.push_back(s);
    } else if (key.rfind("sponsor.", 0) == 0) {
      std::vector<std::string> t = CareerCommon::SplitPipes(val);
      SponsorDeal s;
      if (t.size() > 0)
        s.sponsorName = t[0];
      if (t.size() > 1)
        s.type = t[1];
      if (t.size() > 2)
        s.annualRevenue = CareerCommon::SafeStoll(t[2]);
      if (t.size() > 3)
        s.yearsRemaining = CareerCommon::SafeStoi(t[3]);
      if (t.size() > 4)
        s.reputationRequirement = CareerCommon::SafeStoi(t[4]);
      fresh.activeSponsors.push_back(s);
    } else if (key.rfind("event.", 0) == 0) {
      std::vector<std::string> t = CareerCommon::SplitPipes(val);
      CareerEvent e;
      if (t.size() > 0)
        e.type = t[0];
      if (t.size() > 1)
        e.reputationImpact = CareerCommon::SafeStoi(t[1]);
      if (t.size() > 2)
        e.timestamp = CareerCommon::SafeStoll(t[2]);
      if (t.size() > 3)
        e.isMajor = CareerCommon::SafeStoi(t[3]) != 0;
      if (t.size() > 4)
        e.description = t[4];
      fresh.recentEvents.push_back(e);
    } else if (key.rfind("inbox.", 0) == 0) {
      std::vector<std::string> t = CareerCommon::SplitPipes(val);
      InboxItem m;
      if (t.size() > 0)
        m.id = CareerCommon::SafeStoi(t[0]);
      if (t.size() > 1)
        m.type = static_cast<InboxItemType>(CareerCommon::SafeStoi(t[1]));
      if (t.size() > 2)
        m.weekCreated = CareerCommon::SafeStoi(t[2]);
      if (t.size() > 3)
        m.read = CareerCommon::SafeStoi(t[3]) != 0;
      if (t.size() > 4)
        m.relatedPlayerID = CareerCommon::SafeStoi(t[4]);
      if (t.size() > 5)
        m.relatedTeamID = CareerCommon::SafeStoi(t[5]);
      if (t.size() > 6)
        m.subject = t[6];
      if (t.size() > 7)
        m.body = t[7];
      fresh.inbox.push_back(m);
    } else if (key.rfind("history.", 0) == 0) {
      std::vector<std::string> t = CareerCommon::SplitPipes(val);
      SeasonRecord r;
      if (t.size() > 0)
        r.season = CareerCommon::SafeStoi(t[0]);
      if (t.size() > 1)
        r.teamID = CareerCommon::SafeStoi(t[1]);
      if (t.size() > 2)
        r.wins = CareerCommon::SafeStoi(t[2]);
      if (t.size() > 3)
        r.draws = CareerCommon::SafeStoi(t[3]);
      if (t.size() > 4)
        r.losses = CareerCommon::SafeStoi(t[4]);
      if (t.size() > 5)
        r.goalsFor = CareerCommon::SafeStoi(t[5]);
      if (t.size() > 6)
        r.goalsAgainst = CareerCommon::SafeStoi(t[6]);
      if (t.size() > 7)
        r.leaguePosition = CareerCommon::SafeStoi(t[7]);
      if (t.size() > 8)
        r.wonTitle = CareerCommon::SafeStoi(t[8]) != 0;
      fresh.history.push_back(r);
    } else if (key.rfind("boardObjective.", 0) == 0) {
      std::vector<std::string> t = CareerCommon::SplitPipes(val);
      OwnerBoardObjective o;
      if (t.size() > 0)
        o.type = static_cast<OwnerObjectiveType>(CareerCommon::SafeStoi(t[0]));
      if (t.size() > 1)
        o.completed = CareerCommon::SafeStoi(t[1]) != 0;
      if (t.size() > 2)
        o.reputationReward = CareerCommon::SafeStoi(t[2], o.reputationReward);
      if (t.size() > 3)
        o.confidencePenalty = CareerCommon::SafeStoi(t[3], o.confidencePenalty);
      if (t.size() > 4)
        o.description = t[4];
      fresh.boardObjectives.push_back(o);
    } else if (key.rfind("legacy.", 0) == 0) {
      fresh.legacyStats[key.substr(7)] = CareerCommon::SafeStoi(val);
    } else if (key.rfind("bid.", 0) == 0) {
      std::vector<std::string> t = CareerCommon::SplitPipes(val);
      TransferBid b;
      if (t.size() > 0)
        b.playerName = t[0];
      if (t.size() > 1)
        b.bidAmount = CareerCommon::SafeStoll(t[1]);
      if (t.size() > 2)
        b.offeredWage = CareerCommon::SafeStoi(t[2]);
      if (t.size() > 3)
        b.contractYears = CareerCommon::SafeStoi(t[3], b.contractYears);
      if (t.size() > 4)
        b.agentFee = CareerCommon::SafeStoll(t[4]);
      if (t.size() > 5)
        b.status = static_cast<BidStatus>(CareerCommon::SafeStoi(t[5]));
      if (t.size() > 6)
        b.negotiationRounds = CareerCommon::SafeStoi(t[6]);
      loadedBids.push_back(b);
    }
  }

  // Keep the mirrored/derived fields consistent with the loaded top-level
  // values so a loaded save matches the state produced by CreateNewCareer.
  fresh.currentSeason = fresh.season.currentSeason;
  fresh.club.reputation = fresh.reputation;
  fresh.board.confidence = fresh.boardConfidence;
  fresh.finance.transferBudget = fresh.transferBudget;
  fresh.finance.wageBudget = fresh.wageBudget;

  out = fresh;
  bids = loadedBids;
  return true;
}

// Legacy plain-text save fallback: reads the on-disk file as text and parses
// it exactly as before the SQLite migration.
bool DeserializeLegacyFile(const std::string& path, CareerSave& save,
                           std::vector<TransferBid>& bids) {
  std::ifstream file(path);
  if (!file.is_open())
    return false;
  std::stringstream buffer;
  buffer << file.rdbuf();
  return Deserialize(buffer.str(), save, bids);
}

constexpr int kSchemaVersion = 1;

// Frees a prepared statement and returns false (with the provided db closed)
// so failure paths stay single-exit.
void CloseDb(sqlite3* db) {
  if (db)
    sqlite3_close(db);
}

std::string SqliteText(sqlite3_stmt* stmt, int col) {
  const unsigned char* text = sqlite3_column_text(stmt, col);
  return text ? reinterpret_cast<const char*>(text) : std::string();
}

}  // namespace

bool Save(const CareerSave& save, const std::vector<TransferBid>& bids, const std::string& path) {
  const std::string payload = Serialize(save, bids);

  sqlite3* db = nullptr;
  if (sqlite3_open_v2(path.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) !=
      SQLITE_OK) {
    if (db)
      sqlite3_close(db);
    return false;
  }

  auto fail = [&db]() -> bool {
    CloseDb(db);
    return false;
  };

  // Ensure schema exists (idempotent across saves).
  const char* createMeta =
      "CREATE TABLE IF NOT EXISTS career_meta ("
      "schema_version INTEGER NOT NULL,"
      "name TEXT NOT NULL,"
      "mode INTEGER NOT NULL,"
      "season INTEGER NOT NULL)";
  const char* createPayload =
      "CREATE TABLE IF NOT EXISTS career_payload ("
      "id INTEGER PRIMARY KEY,"
      "data TEXT NOT NULL)";
  if (sqlite3_exec(db, createMeta, nullptr, nullptr, nullptr) != SQLITE_OK)
    return fail();
  if (sqlite3_exec(db, createPayload, nullptr, nullptr, nullptr) != SQLITE_OK)
    return fail();
  if (sqlite3_exec(db, "BEGIN", nullptr, nullptr, nullptr) != SQLITE_OK)
    return fail();

  if (sqlite3_exec(db, "DELETE FROM career_meta", nullptr, nullptr, nullptr) != SQLITE_OK) {
    sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
    return fail();
  }
  if (sqlite3_exec(db, "DELETE FROM career_payload", nullptr, nullptr, nullptr) != SQLITE_OK) {
    sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
    return fail();
  }

  // Insert metadata (bound, so names with quotes are safe).
  {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT INTO career_meta(schema_version,name,mode,season) VALUES(?,?,?,?)";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
      return fail();
    sqlite3_bind_int(stmt, 1, kSchemaVersion);
    sqlite3_bind_text(stmt, 2, save.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, static_cast<int>(save.mode));
    sqlite3_bind_int(stmt, 4, save.season.currentSeason);
    const bool metaOk = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    if (!metaOk) {
      sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
      return fail();
    }
  }

  // Insert payload.
  {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT INTO career_payload(id,data) VALUES(1,?)";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
      return fail();
    sqlite3_bind_text(stmt, 1, payload.c_str(), -1, SQLITE_TRANSIENT);
    const bool payloadOk = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    if (!payloadOk) {
      sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
      return fail();
    }
  }

  if (sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr) != SQLITE_OK) {
    sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
    return fail();
  }
  CloseDb(db);
  return true;
}

bool Load(CareerSave& save, std::vector<TransferBid>& bids, const std::string& path) {
  sqlite3* db = nullptr;
  // Open for reading; if the file is missing or not a SQLite database this
  // either fails to open or fails the first query (SQLITE_NOTADB), both of
  // which route us to the legacy plain-text fallback below.
  if (sqlite3_open_v2(path.c_str(), &db, SQLITE_OPEN_READWRITE, nullptr) != SQLITE_OK) {
    if (db)
      sqlite3_close(db);
    return DeserializeLegacyFile(path, save, bids);
  }
  if (!db) {
    return DeserializeLegacyFile(path, save, bids);
  }

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, "SELECT data FROM career_payload ORDER BY id LIMIT 1", -1, &stmt,
                         nullptr) != SQLITE_OK) {
    CloseDb(db);
    return DeserializeLegacyFile(path, save, bids);
  }

  int rc = sqlite3_step(stmt);
  if (rc == SQLITE_NOTADB) {
    sqlite3_finalize(stmt);
    CloseDb(db);
    return DeserializeLegacyFile(path, save, bids);
  }
  if (rc != SQLITE_ROW) {
    sqlite3_finalize(stmt);
    CloseDb(db);
    return false;
  }

  const std::string payload = SqliteText(stmt, 0);
  sqlite3_finalize(stmt);
  CloseDb(db);

  if (payload.empty())
    return false;
  return Deserialize(payload, save, bids);
}

}  // namespace CareerPersistence
}  // namespace blunted
