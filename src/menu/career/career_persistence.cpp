#include "career_persistence.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "career_common.hpp"
#include "sqlite3.h"

namespace blunted {
namespace CareerPersistence {

namespace {

std::string DivisionLine(const DivisionConfig& division) {
  return CareerCommon::Sanitize(division.name) + "|" + std::to_string(division.numTeams) + "|" +
         std::to_string(division.promotionSpots) + "|" + std::to_string(division.relegationSpots) +
         "|" + std::to_string(division.playOffSpots);
}

bool ParseDivisionLine(const std::string& value, DivisionConfig& division) {
  const std::vector<std::string> t = CareerCommon::SplitPipes(value);
  if (t.size() < 5 || t[0].empty())
    return false;
  division.name = t[0];
  division.numTeams = std::max(2, CareerCommon::SafeStoi(t[1], division.numTeams));
  division.promotionSpots = std::max(0, CareerCommon::SafeStoi(t[2], division.promotionSpots));
  division.relegationSpots = std::max(0, CareerCommon::SafeStoi(t[3], division.relegationSpots));
  division.playOffSpots = std::max(0, CareerCommon::SafeStoi(t[4], division.playOffSpots));
  return true;
}

// Serializes the full career state (and transfer bids and market) to the
// pipe-delimited, line-based text payload. This is the durable format stored
// inside the SQLite save file; keeping it also preserves the legacy plain-text
// interchange.
std::string Serialize(const CareerSave& save, const std::vector<TransferBid>& bids,
                      const std::vector<TransferTarget>& targets) {
  std::ostringstream file;
  file << "# Career Save: " << save.name << "\n";
  file << "formatVersion=2\n";
  file << "lifecycleVersion=1\n";
  file << "mode=" << CareerModeKey(save.mode) << "\n";
  file << "handsOnManagement=" << (save.handsOnManagement ? 1 : 0) << "\n";
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
  file << "seasonForm=" << CareerCommon::Sanitize(save.seasonForm) << "\n";
  file << "netWorth=" << save.finances.netWorth << "\n";
  file << "ticketPrice=" << save.finances.ticketPrice << "\n";
  file << "stadiumCapacity=" << save.stadium.capacity << "\n";
  file << "stadiumName=" << save.stadium.name << "\n";
  file << "stadiumCondition=" << save.stadium.condition << "\n";
  file << "stadiumFanSatisfaction=" << save.stadium.fanSatisfaction << "\n";
  file << "controlledEntityID=" << save.controlledEntityID << "\n";
  file << "trainingPlan=" << static_cast<int>(save.trainingPlan) << "\n";
  file << "trainingPoints=" << save.trainingPoints << "\n";
  file << "scoutingNetworkLevel=" << save.scoutingNetworkLevel << "\n";
  file << "objective=" << CareerCommon::Sanitize(save.objective) << "\n";

  file << "maxWeeks=" << save.season.maxWeeks << "\n";
  file << "inPreseason=" << save.season.inPreseason << "\n";
  file << "transferWindowOpen=" << save.season.transferWindowOpen << "\n";
  file << "financeSnapshot=" << save.finances.totalRevenue << "|" << save.finances.totalExpenses
       << "|" << save.finances.matchDayIncome << "|" << save.finances.sponsorIncome << "|"
       << save.finances.merchandiseIncome << "|" << save.finances.tvRevenue << "|"
       << save.finances.playerWages << "|" << save.finances.staffWages << "|"
       << save.finances.stadiumCosts << "|" << save.finances.transferSpending << "|"
       << save.finances.transferIncome << "\n";
  file << "stadiumRevenue=" << save.stadium.matchDayRevenue << "\n";
  file << "stadiumMaintenance=" << save.stadium.maintenanceCost << "\n";
  auto writeUpgrades = [&](const char* prefix, const std::vector<StadiumUpgrade>& upgrades) {
    for (size_t i = 0; i < upgrades.size(); ++i) {
      const auto& u = upgrades[i];
      file << prefix << i << "=" << CareerCommon::Sanitize(u.name) << "|"
           << CareerCommon::Sanitize(u.description) << "|" << u.cost << "|" << u.buildTimeSeasons
           << "|" << u.seasonsRemaining << "|" << u.capacityIncrease << "|" << u.revenueBonus
           << "\n";
    }
  };
  writeUpgrades("upgrade.", save.stadium.upgrades);
  writeUpgrades("availableUpgrade.", save.stadium.availableUpgrades);
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
  for (size_t i = 0; i < save.availableSponsorOffers.size(); i++) {
    const auto& s = save.availableSponsorOffers[i];
    file << "sponsorOffer." << i << "=" << CareerCommon::Sanitize(s.sponsorName) << "|"
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
         << CareerCommon::Sanitize(o.description) << "|" << (o.hasTarget ? 1 : 0) << "|" << o.target
         << "\n";
  }
  for (const auto& kv : save.legacyStats)
    file << "legacy." << CareerCommon::Sanitize(kv.first) << "=" << kv.second << "\n";
  for (size_t i = 0; i < save.season.fixtures.size(); i++) {
    const auto& f = save.season.fixtures[i];
    file << "fixture." << i << "=" << f.fixtureID << "|" << f.homeTeamID << "|" << f.awayTeamID
         << "|" << f.homeGoals << "|" << f.awayGoals << "|" << (f.played ? 1 : 0) << "|" << f.season
         << "\n";
  }
  for (size_t i = 0; i < bids.size(); i++) {
    const auto& b = bids[i];
    file << "bid." << i << "=" << CareerCommon::Sanitize(b.playerName) << "|" << b.bidAmount << "|"
         << b.offeredWage << "|" << b.contractYears << "|" << b.agentFee << "|"
         << static_cast<int>(b.status) << "|" << b.negotiationRounds << "\n";
  }
  for (size_t i = 0; i < save.leagueTable.size(); i++) {
    const auto& e = save.leagueTable[i];
    file << "leagueTable." << i << "=" << e.teamID << "|" << CareerCommon::Sanitize(e.name) << "|"
         << e.played << "|" << e.wins << "|" << e.draws << "|" << e.losses << "|" << e.goalsFor
         << "|" << e.goalsAgainst << "|" << CareerCommon::Sanitize(e.form) << "\n";
  }
  file << "leagueSettingsEnabled=" << (save.leagueSettings.enabled ? 1 : 0) << "\n";
  for (size_t i = 0; i < save.leagueSettings.divisions.size(); i++)
    file << "leagueDivision." << i << "=" << DivisionLine(save.leagueSettings.divisions[i]) << "\n";
  file << "customLeague=" << CareerCommon::Sanitize(save.customLeague.leagueName) << "|"
       << save.customLeague.numDivisions << "|" << (save.customLeague.cupCompetition ? 1 : 0) << "|"
       << CareerCommon::Sanitize(save.customLeague.cupName) << "\n";
  for (size_t i = 0; i < save.customLeague.divisions.size(); i++)
    file << "customDivision." << i << "=" << DivisionLine(save.customLeague.divisions[i]) << "\n";
  // The market is saved with the bids: an accepted bid completes against its
  // target, so dropping the market made accepted bids uncompletable on load.
  for (size_t i = 0; i < targets.size(); i++) {
    const auto& t = targets[i];
    file << "target." << i << "=" << CareerCommon::Sanitize(t.name) << "|"
         << CareerCommon::Sanitize(t.preferredPosition) << "|" << t.overallRating << "|"
         << t.potentialRating << "|" << t.age << "|" << t.value << "|" << t.wage << "|"
         << t.askingPrice << "|" << t.teamID << "|" << (t.isListed ? 1 : 0) << "\n";
  }
  return file.str();
}

// Parses the text payload back into a CareerSave, keeping mirrored/derived
// fields consistent. Invalid role/version fields reject the save; other corrupt
// individual fields fall back to defaults rather than aborting.
bool Deserialize(const std::string& text, CareerSave& out, std::vector<TransferBid>& bids,
                 std::vector<TransferTarget>& targets) {
  std::istringstream file(text);
  if (!file)
    return false;
  CareerSave fresh;
  std::vector<TransferBid> loadedBids;
  std::vector<TransferTarget> loadedTargets;
  std::string line;
  std::string roleValue = "1";  // Very old saves without a role were Manager careers.
  std::string formatVersion = "1";
  std::string handsOnValue;
  bool hasLifecycleVersion = false;
  while (std::getline(file, line)) {
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
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
      roleValue = val;
    else if (key == "lifecycleVersion")
      hasLifecycleVersion = val == "1";
    else if (key == "formatVersion")
      formatVersion = val;
    else if (key == "handsOnManagement")
      handsOnValue = val;
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
    else if (key == "seasonForm")
      fresh.seasonForm = val.substr(0, 5);
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
    else if (key == "maxWeeks")
      fresh.season.maxWeeks = std::max(1, CareerCommon::SafeStoi(val, 38));
    else if (key == "inPreseason")
      fresh.season.inPreseason = val == "1";
    else if (key == "transferWindowOpen")
      fresh.season.transferWindowOpen = val == "1";
    else if (key == "stadiumRevenue")
      fresh.stadium.matchDayRevenue = CareerCommon::SafeStoll(val);
    else if (key == "stadiumMaintenance")
      fresh.stadium.maintenanceCost = CareerCommon::SafeStoll(val);
    else if (key == "financeSnapshot") {
      auto fields = CareerCommon::SplitPipes(val);
      long long* amounts[] = {&fresh.finances.totalRevenue,      &fresh.finances.totalExpenses,
                              &fresh.finances.matchDayIncome,    &fresh.finances.sponsorIncome,
                              &fresh.finances.merchandiseIncome, &fresh.finances.tvRevenue,
                              &fresh.finances.playerWages,       &fresh.finances.staffWages,
                              &fresh.finances.stadiumCosts,      &fresh.finances.transferSpending,
                              &fresh.finances.transferIncome};
      for (size_t i = 0; i < fields.size() && i < 11; ++i)
        *amounts[i] = CareerCommon::SafeStoll(fields[i]);
    } else if (key.rfind("upgrade.", 0) == 0 || key.rfind("availableUpgrade.", 0) == 0) {
      auto fields = CareerCommon::SplitPipes(val);
      if (fields.size() == 7) {
        StadiumUpgrade u{fields[0],
                         fields[1],
                         CareerCommon::SafeStoll(fields[2]),
                         CareerCommon::SafeStoi(fields[3]),
                         CareerCommon::SafeStoi(fields[4]),
                         CareerCommon::SafeStoi(fields[5]),
                         CareerCommon::SafeStoi(fields[6])};
        (key.rfind("upgrade.", 0) == 0 ? fresh.stadium.upgrades : fresh.stadium.availableUpgrades)
            .push_back(u);
      }
    } else if (key == "trainingPlan") {
      int plan = CareerCommon::SafeStoi(val, 0);
      fresh.trainingPlan = plan >= 0 && plan <= 2 ? static_cast<CareerTrainingPlan>(plan)
                                                  : CareerTrainingPlan::BALANCED;
    } else if (key == "trainingPoints")
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
    } else if (key.rfind("sponsor.", 0) == 0 || key.rfind("sponsorOffer.", 0) == 0) {
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
      (key.rfind("sponsor.", 0) == 0 ? fresh.activeSponsors : fresh.availableSponsorOffers)
          .push_back(s);
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
      if (t.size() > 1) {
        const int type = CareerCommon::SafeStoi(t[1]);
        if (type >= 0 && type <= static_cast<int>(InboxItemType::DERBY_HYPE))
          m.type = static_cast<InboxItemType>(type);
      }
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
      if (t.size() > 6) {
        o.hasTarget = t[5] == "1";
        o.target = CareerCommon::SafeStoi(t[6]);
      }
      fresh.boardObjectives.push_back(o);
    } else if (key.rfind("legacy.", 0) == 0) {
      fresh.legacyStats[key.substr(7)] = CareerCommon::SafeStoi(val);
    } else if (key.rfind("fixture.", 0) == 0) {
      std::vector<std::string> t = CareerCommon::SplitPipes(val);
      FixtureResult f;
      if (t.size() > 0)
        f.fixtureID = CareerCommon::SafeStoi(t[0]);
      if (t.size() > 1)
        f.homeTeamID = CareerCommon::SafeStoi(t[1]);
      if (t.size() > 2)
        f.awayTeamID = CareerCommon::SafeStoi(t[2]);
      if (t.size() > 3)
        f.homeGoals = CareerCommon::SafeStoi(t[3]);
      if (t.size() > 4)
        f.awayGoals = CareerCommon::SafeStoi(t[4]);
      if (t.size() > 5)
        f.played = CareerCommon::SafeStoi(t[5]) != 0;
      if (t.size() > 6)
        f.season = CareerCommon::SafeStoi(t[6]);
      fresh.season.fixtures.push_back(f);
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
    } else if (key.rfind("leagueTable.", 0) == 0) {
      const std::vector<std::string> t = CareerCommon::SplitPipes(val);
      if (t.size() < 9 || t[1].empty())
        continue;
      LeagueTableEntry entry;
      entry.teamID = CareerCommon::SafeStoi(t[0]);
      entry.name = t[1];
      entry.played = std::max(0, CareerCommon::SafeStoi(t[2]));
      entry.wins = std::max(0, CareerCommon::SafeStoi(t[3]));
      entry.draws = std::max(0, CareerCommon::SafeStoi(t[4]));
      entry.losses = std::max(0, CareerCommon::SafeStoi(t[5]));
      entry.goalsFor = std::max(0, CareerCommon::SafeStoi(t[6]));
      entry.goalsAgainst = std::max(0, CareerCommon::SafeStoi(t[7]));
      entry.form = t[8].substr(0, 5);
      fresh.leagueTable.push_back(entry);
    } else if (key == "leagueSettingsEnabled") {
      fresh.leagueSettings.enabled = val != "0";
    } else if (key.rfind("leagueDivision.", 0) == 0) {
      DivisionConfig division;
      if (ParseDivisionLine(val, division))
        fresh.leagueSettings.divisions.push_back(division);
    } else if (key == "customLeague") {
      const std::vector<std::string> t = CareerCommon::SplitPipes(val);
      if (t.size() >= 4) {
        fresh.customLeague.leagueName = t[0];
        fresh.customLeague.numDivisions = std::max(1, CareerCommon::SafeStoi(t[1], 1));
        fresh.customLeague.cupCompetition = t[2] == "1";
        fresh.customLeague.cupName = t[3];
      }
    } else if (key.rfind("customDivision.", 0) == 0) {
      DivisionConfig division;
      if (ParseDivisionLine(val, division))
        fresh.customLeague.divisions.push_back(division);
    } else if (key.rfind("target.", 0) == 0) {
      std::vector<std::string> t = CareerCommon::SplitPipes(val);
      if (t.size() < 10 || t[0].empty())
        continue;
      TransferTarget target;
      target.name = t[0];
      target.preferredPosition = t[1];
      target.overallRating = CareerCommon::SafeStoi(t[2]);
      target.potentialRating = CareerCommon::SafeStoi(t[3]);
      target.age = CareerCommon::SafeStoi(t[4]);
      target.value = CareerCommon::SafeStoll(t[5]);
      target.wage = CareerCommon::SafeStoll(t[6]);
      target.askingPrice = CareerCommon::SafeStoll(t[7]);
      target.teamID = CareerCommon::SafeStoi(t[8], -1);
      target.isListed = t[9] != "0";
      loadedTargets.push_back(target);
    }
  }

  if (formatVersion != "1" && formatVersion != "2")
    return false;
  if (!DecodeCareerRole(roleValue, fresh.mode, fresh.handsOnManagement, formatVersion == "1"))
    return false;
  if (formatVersion == "2") {
    if (handsOnValue != "0" && handsOnValue != "1")
      return false;
    fresh.handsOnManagement = handsOnValue == "1";
  }

  // Older simulated matches were saved before Back advanced the week.
  if (!hasLifecycleVersion) {
    const int completed = fresh.seasonWins + fresh.seasonDraws + fresh.seasonLosses;
    if (completed >= fresh.season.currentWeek && completed > 0)
      fresh.season.currentWeek = completed + 1;
  }

  // Keep the mirrored/derived fields consistent with the loaded top-level
  // values so a loaded save matches the state produced by CreateNewCareer.
  fresh.currentSeason = fresh.season.currentSeason;
  fresh.club.reputation = fresh.reputation;
  fresh.board.confidence = fresh.boardConfidence;
  fresh.finance.transferBudget = fresh.transferBudget;
  fresh.finance.wageBudget = fresh.wageBudget;

  if (fresh.name.empty() && fresh.club.clubName.empty())
    return false;

  out = fresh;
  bids = loadedBids;
  targets = loadedTargets;
  return true;
}

// Legacy plain-text save fallback: reads the on-disk file as text and parses
// it exactly as before the SQLite migration.
bool DeserializeLegacyFile(const std::string& path, CareerSave& save,
                           std::vector<TransferBid>& bids, std::vector<TransferTarget>& targets) {
  std::ifstream file(path);
  if (!file.is_open())
    return false;
  std::stringstream buffer;
  buffer << file.rdbuf();
  return Deserialize(buffer.str(), save, bids, targets);
}

constexpr int kSchemaVersion = 2;

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

bool Save(const CareerSave& save, const std::vector<TransferBid>& bids,
          const std::vector<TransferTarget>& targets, const std::string& path) {
  if (std::string(CareerModeKey(save.mode)).empty())
    return false;
  namespace fs = std::filesystem;
  try {
    fs::path p(path);
    if (p.has_parent_path()) {
      fs::create_directories(p.parent_path());
    }
  } catch (...) {
  }

  const std::string tempPath = path + ".tmp";
  const std::string backupPath = path + ".bak";

  std::error_code ec;
  fs::remove(tempPath, ec);

  const std::string payload = Serialize(save, bids, targets);

  sqlite3* db = nullptr;
  if (sqlite3_open_v2(tempPath.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) !=
      SQLITE_OK) {
    if (db)
      sqlite3_close(db);
    return false;
  }

  auto fail = [&db, &tempPath]() -> bool {
    CloseDb(db);
    std::error_code ec;
    std::filesystem::remove(tempPath, ec);
    return false;
  };

  // Ensure schema exists (idempotent across saves).
  const char* createMeta =
      "CREATE TABLE IF NOT EXISTS career_meta ("
      "schema_version INTEGER NOT NULL,"
      "name TEXT NOT NULL,"
      "mode TEXT NOT NULL,"
      "season INTEGER NOT NULL,"
      "week INTEGER NOT NULL DEFAULT 1,"
      "club_name TEXT NOT NULL DEFAULT '',"
      "manager_name TEXT NOT NULL DEFAULT '',"
      "transfer_budget INTEGER NOT NULL DEFAULT 0,"
      "reputation INTEGER NOT NULL DEFAULT 50,"
      "board_confidence INTEGER NOT NULL DEFAULT 75,"
      "timestamp TEXT NOT NULL DEFAULT '')";
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
    const char* sql =
        "INSERT INTO "
        "career_meta(schema_version,name,mode,season,week,club_name,manager_name,transfer_budget,"
        "reputation,board_confidence,timestamp) "
        "VALUES(?,?,?,?,?,?,?,?,?,?,?)";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
      return fail();

    time_t now = time(nullptr);
    char timeBuf[64];
    struct tm tmNow;
#if defined(_MSC_VER) || defined(__MINGW32__)
    localtime_s(&tmNow, &now);
#else
    localtime_r(&now, &tmNow);
#endif
    strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M", &tmNow);

    sqlite3_bind_int(stmt, 1, kSchemaVersion);
    sqlite3_bind_text(stmt, 2, save.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, CareerModeKey(save.mode), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, save.season.currentSeason);
    sqlite3_bind_int(stmt, 5, save.season.currentWeek);
    sqlite3_bind_text(stmt, 6, save.club.clubName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, save.managerName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 8, save.transferBudget);
    sqlite3_bind_int(stmt, 9, save.reputation);
    sqlite3_bind_int(stmt, 10, save.boardConfidence);
    sqlite3_bind_text(stmt, 11, timeBuf, -1, SQLITE_TRANSIENT);

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

  // Preserve the original legacy file separately from the rotating recovery backup.
  if (fs::exists(path)) {
    CareerSaveSummary existing;
    const auto originalPath = path + ".pre-three-modes.bak";
    if (ReadSummary(path, existing) && existing.schemaVersion == 1 && !fs::exists(originalPath)) {
      fs::copy_file(path, originalPath, fs::copy_options::none, ec);
      if (ec)
        return false;
    }
    fs::copy_file(path, backupPath, fs::copy_options::overwrite_existing, ec);
    if (ec)
      return false;
  }

  // Same-directory rename is the commit point. Never fall back to copying
  // over the live save: a failed copy could leave partially committed state.
  fs::rename(tempPath, path, ec);
  return !ec;
}

static bool LoadDirect(CareerSave& save, std::vector<TransferBid>& bids,
                       std::vector<TransferTarget>& targets, const std::string& path) {
  sqlite3* db = nullptr;
  if (sqlite3_open_v2(path.c_str(), &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
    if (db)
      sqlite3_close(db);
    return DeserializeLegacyFile(path, save, bids, targets);
  }
  if (!db) {
    return DeserializeLegacyFile(path, save, bids, targets);
  }

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, "SELECT data FROM career_payload ORDER BY id LIMIT 1", -1, &stmt,
                         nullptr) != SQLITE_OK) {
    CloseDb(db);
    return DeserializeLegacyFile(path, save, bids, targets);
  }

  int rc = sqlite3_step(stmt);
  if (rc == SQLITE_NOTADB) {
    sqlite3_finalize(stmt);
    CloseDb(db);
    return DeserializeLegacyFile(path, save, bids, targets);
  }
  if (rc != SQLITE_ROW) {
    sqlite3_finalize(stmt);
    CloseDb(db);
    return false;
  }

  const std::string payload = SqliteText(stmt, 0);
  sqlite3_finalize(stmt);
  stmt = nullptr;
  if (sqlite3_prepare_v2(db, "SELECT schema_version FROM career_meta LIMIT 1", -1, &stmt,
                         nullptr) != SQLITE_OK) {
    CloseDb(db);
    return false;
  }
  const bool supported = sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_int(stmt, 0) >= 1 &&
                         sqlite3_column_int(stmt, 0) <= kSchemaVersion;
  sqlite3_finalize(stmt);
  CloseDb(db);
  if (!supported)
    return false;

  if (payload.empty())
    return false;
  return Deserialize(payload, save, bids, targets);
}

bool Load(CareerSave& save, std::vector<TransferBid>& bids, std::vector<TransferTarget>& targets,
          const std::string& path) {
  if (LoadDirect(save, bids, targets, path)) {
    return true;
  }

  // Backup fallback recovery: if primary file was corrupted or truncated, check for .bak
  namespace fs = std::filesystem;
  std::string backupPath = path + ".bak";
  if (fs::exists(backupPath)) {
    if (LoadDirect(save, bids, targets, backupPath)) {
      printf(
          "[career] Notice: Primary save '%s' corrupt or unreadable; successfully restored from "
          "backup '%s'\n",
          path.c_str(), backupPath.c_str());
      // Self-heal the primary save
      Save(save, bids, targets, path);
      return true;
    }
  }

  return false;
}

bool Save(const CareerSave& save, const std::vector<TransferBid>& bids, const std::string& path) {
  return Save(save, bids, {}, path);
}

bool Load(CareerSave& save, std::vector<TransferBid>& bids, const std::string& path) {
  std::vector<TransferTarget> targets;
  return Load(save, bids, targets, path);
}

static bool ReadLegacySummary(const std::string& path, CareerSaveSummary& summary) {
  CareerSave save;
  std::vector<TransferBid> bids;
  std::vector<TransferTarget> targets;
  if (!DeserializeLegacyFile(path, save, bids, targets))
    return false;
  summary = CareerSaveSummary{};
  summary.name = save.name;
  summary.clubName = save.club.clubName;
  summary.managerName = save.managerName;
  summary.mode = save.mode;
  summary.season = save.season.currentSeason;
  summary.week = save.season.currentWeek;
  summary.transferBudget = save.transferBudget;
  summary.reputation = save.reputation;
  summary.boardConfidence = save.boardConfidence;
  summary.isValid = true;
  return true;
}

static bool ReadSummaryDirect(const std::string& path, CareerSaveSummary& outSummary) {
  outSummary = CareerSaveSummary{};
  sqlite3* db = nullptr;
  if (sqlite3_open_v2(path.c_str(), &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
    CloseDb(db);
    return ReadLegacySummary(path, outSummary);
  }
  const char* queries[] = {
      "SELECT schema_version,name,mode,season,week,club_name,manager_name,transfer_budget,"
      "reputation,board_confidence,timestamp FROM career_meta LIMIT 1",
      "SELECT schema_version,name,mode,season FROM career_meta LIMIT 1"};
  for (int i = 0; i < 2; ++i) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, queries[i], -1, &stmt, nullptr) != SQLITE_OK) {
      sqlite3_finalize(stmt);
      continue;
    }
    if (sqlite3_step(stmt) != SQLITE_ROW) {
      sqlite3_finalize(stmt);
      continue;
    }
    CareerSaveSummary summary;
    summary.schemaVersion = sqlite3_column_int(stmt, 0);
    bool handsOn = true;
    const bool validRole =
        summary.schemaVersion >= 1 && summary.schemaVersion <= kSchemaVersion &&
        DecodeCareerRole(SqliteText(stmt, 2), summary.mode, handsOn, summary.schemaVersion == 1);
    summary.name = SqliteText(stmt, 1);
    summary.season = sqlite3_column_int(stmt, 3);
    if (i == 0) {
      summary.week = sqlite3_column_int(stmt, 4);
      summary.clubName = SqliteText(stmt, 5);
      summary.managerName = SqliteText(stmt, 6);
      summary.transferBudget = sqlite3_column_int64(stmt, 7);
      summary.reputation = sqlite3_column_int(stmt, 8);
      summary.boardConfidence = sqlite3_column_int(stmt, 9);
      summary.timestamp = SqliteText(stmt, 10);
    }
    sqlite3_finalize(stmt);
    CloseDb(db);
    summary.isValid = validRole && (!summary.name.empty() || !summary.clubName.empty());
    if (!summary.isValid)
      return false;
    outSummary = summary;
    return true;
  }
  CloseDb(db);
  // sqlite3_open can succeed on text files; query preparation detects NOTADB.
  return ReadLegacySummary(path, outSummary);
}

bool ReadSummary(const std::string& path, CareerSaveSummary& outSummary) {
  if (ReadSummaryDirect(path, outSummary) && outSummary.isValid) {
    return true;
  }

  namespace fs = std::filesystem;
  std::string backupPath = path + ".bak";
  if (fs::exists(backupPath)) {
    CareerSaveSummary backupSummary;
    if (ReadSummaryDirect(backupPath, backupSummary) && backupSummary.isValid) {
      outSummary = backupSummary;
      return true;
    }
  }

  return false;
}

}  // namespace CareerPersistence
}  // namespace blunted
