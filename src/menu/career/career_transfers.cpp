#include "career_transfers.hpp"

#include <algorithm>

#include <sqlite3.h>
#include "career_common.hpp"
#include "utils/localization.hpp"

namespace blunted {
namespace CareerTransfers {

namespace {
using CareerCommon::RandomInt;
}  // namespace

void RecruitFreeAgent(CareerSave& save, CareerCommon::CareerEvents& events,
                      const std::string& playerName) {
  auto it =
      std::find_if(save.freeAgents.begin(), save.freeAgents.end(),
                   [&playerName](const PlayerCareerState& p) { return p.name == playerName; });
  if (it == save.freeAgents.end())
    return;
  if (save.wageBudget < it->wage) {
    events.AddEvent("financial", "Failed to sign " + playerName + " due to wage budget limits", -1,
                    false);
    return;
  }
  save.wageBudget -= it->wage;
  save.finance.wageBudget = save.wageBudget;
  save.roster.push_back(*it);
  save.freeAgents.erase(it);
  events.AddEvent("recruitment", "Signed free agent " + playerName, 2, false);
  events.ModifyBoardConfidence(1);
}

void ReleasePlayer(CareerSave& save, CareerCommon::CareerEvents& events,
                   const std::string& playerName) {
  auto it = std::find_if(
      save.roster.begin(), save.roster.end(),
      [&playerName](const PlayerCareerState& player) { return player.name == playerName; });
  if (it == save.roster.end())
    return;

  // Severance pay: ~25 weeks of wages
  long long severance = it->wage * 25;
  if (save.finances.netWorth < severance) {
    events.AddEvent("financial", "Cannot afford severance pay (" + std::to_string(severance) + ") to release " + playerName, 0, false);
    return;
  }

  save.finances.netWorth -= severance;
  save.wageBudget += it->wage;
  save.finance.wageBudget = save.wageBudget;
  PlayerCareerState released = *it;
  released.transferStatus = TransferStatus::NONE;
  save.freeAgents.push_back(released);
  save.roster.erase(it);
  events.AddEvent("squad", "Released player " + playerName + " (Severance: " + std::to_string(severance) + ")", -1, false);
  events.ModifyBoardConfidence(-1);
}

void SeedFreeAgents(CareerSave& save) {
  if (!save.freeAgents.empty())
    return;

  bool loadedFromDb = false;
  sqlite3* db;
  if (sqlite3_open("databases/default/database.sqlite", &db) == SQLITE_OK) {
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, "SELECT firstname, lastname, role, age, base_stat FROM players WHERE base_stat <= 70 ORDER BY RANDOM() LIMIT 10", -1, &stmt, nullptr) == SQLITE_OK) {
      while (sqlite3_step(stmt) == SQLITE_ROW) {
        PlayerCareerState fa;
        const char* firstRaw = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char* lastRaw = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* roleRaw = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        
        std::string first = firstRaw ? firstRaw : "";
        std::string last = lastRaw ? lastRaw : "";
        fa.name = first.empty() ? last : (first + " " + last);
        fa.position = roleRaw ? roleRaw : "CM";
        fa.preferredPosition = fa.position;
        fa.age = sqlite3_column_int(stmt, 3);
        fa.ovr = sqlite3_column_int(stmt, 4);
        
        fa.pot = std::min(90, fa.ovr + CareerCommon::RandomInt(2, 8));
        fa.value = ComputeMarketValue(fa.ovr, fa.pot, fa.age);
        fa.wage = std::max(1500LL, fa.value / 1400LL);
        fa.transferStatus = TransferStatus::NONE;
        fa.morale = 75;
        fa.fitness = 90;
        fa.matchForm = 50;
        
        save.freeAgents.push_back(fa);
        loadedFromDb = true;
      }
      sqlite3_finalize(stmt);
    }
    sqlite3_close(db);
  }

  if (!loadedFromDb) {
    // Fallback if DB query fails
    static const std::vector<std::string> lastNames = {"Smith", "Silva", "Muller", "Rossi", "Garcia"};
    for (int i = 0; i < 5; ++i) {
      PlayerCareerState fa;
      fa.name = "FreeAgent " + lastNames[i];
      fa.position = "CM";
      fa.preferredPosition = "CM";
      fa.age = 22 + i * 2;
      fa.ovr = 60 + i * 2;
      fa.pot = fa.ovr + 5;
      fa.value = ComputeMarketValue(fa.ovr, fa.pot, fa.age);
      fa.wage = 5000;
      fa.transferStatus = TransferStatus::NONE;
      save.freeAgents.push_back(fa);
    }
  }
}

long long ComputeMarketValue(int overallRating, int potentialRating, int age) {
  long long ageModifier = 120;
  if (age >= 30)
    ageModifier = 85;
  else if (age <= 21)
    ageModifier = 135;
  long long potentialModifier = 100 + std::max(0, potentialRating - overallRating);
  long long baseValue = static_cast<long long>(overallRating) * overallRating * 4000;
  return std::max(50000LL, (baseValue * ageModifier * potentialModifier) / 12000LL);
}

void PopulateTransferMarket(std::vector<TransferTarget>& targets) {
  if (!targets.empty())
    return;

  targets.clear();
  bool loadedFromDb = false;
  sqlite3* db;
  if (sqlite3_open("databases/default/database.sqlite", &db) == SQLITE_OK) {
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, "SELECT firstname, lastname, role, age, base_stat, team_id FROM players ORDER BY RANDOM() LIMIT 20", -1, &stmt, nullptr) == SQLITE_OK) {
      while (sqlite3_step(stmt) == SQLITE_ROW) {
        TransferTarget target;
        const char* firstRaw = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char* lastRaw = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* roleRaw = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        
        std::string first = firstRaw ? firstRaw : "";
        std::string last = lastRaw ? lastRaw : "";
        target.name = first.empty() ? last : (first + " " + last);
        target.preferredPosition = roleRaw ? roleRaw : "CM";
        target.age = sqlite3_column_int(stmt, 3);
        target.overallRating = sqlite3_column_int(stmt, 4);
        target.teamID = sqlite3_column_int(stmt, 5);
        
        target.potentialRating = std::max(target.overallRating, target.overallRating + RandomInt(1, 10));
        target.value = ComputeMarketValue(target.overallRating, target.potentialRating, target.age);
        target.askingPrice = target.value + target.value * RandomInt(10, 30) / 100;
        target.wage = std::max(1500LL, target.value / 1400LL);
        target.isListed = true;
        targets.push_back(target);
        loadedFromDb = true;
      }
      sqlite3_finalize(stmt);
    }
    sqlite3_close(db);
  }

  if (!loadedFromDb) {
    // Fallback if DB query fails
    static const std::vector<std::string> lastNames = {"Silva", "Rossi", "Meyer", "Costa", "Lopez"};
    static const std::vector<std::string> positions = {"GK", "CB", "CM", "ST", "AM"};
    for (int i = 0; i < 15; ++i) {
      TransferTarget target;
      target.name = "Target " + lastNames[i % 5] + " " + std::to_string(i);
      target.preferredPosition = positions[i % 5];
      target.age = RandomInt(18, 31);
      target.overallRating = RandomInt(62, 84);
      target.potentialRating = std::max(target.overallRating, target.overallRating + RandomInt(1, 10));
      target.value = ComputeMarketValue(target.overallRating, target.potentialRating, target.age);
      target.askingPrice = target.value + target.value * RandomInt(10, 30) / 100;
      target.wage = std::max(1500LL, target.value / 1400LL);
      target.teamID = 1000 + i;
      target.isListed = true;
      targets.push_back(target);
    }
  }
}

TransferBid PlaceBid(CareerSave& save, CareerCommon::CareerEvents& events,
                     std::vector<TransferTarget>& targets, std::vector<TransferBid>& bids,
                     const std::string& playerName, long long bidAmount, int offeredWage,
                     int contractYears) {
  TransferBid bid;
  bid.playerName = playerName;
  bid.bidAmount = bidAmount;
  bid.offeredWage = offeredWage;
  bid.contractYears = contractYears;
  bid.agentFee = std::max(25000LL, bidAmount / 20);
  bid.status = BidStatus::REJECTED;

  auto targetIt = std::find_if(
      targets.begin(), targets.end(),
      [&playerName](const TransferTarget& target) { return target.name == playerName; });
  if (targetIt == targets.end())
    return bid;

  long long totalCost = bidAmount + bid.agentFee;
  if (totalCost > save.transferBudget || offeredWage > save.wageBudget) {
    events.AddEvent("transfer", "Bid rejected for " + playerName + " due to budget limits", -1,
                    false);
    return bid;
  }

  bid.status = BidStatus::PENDING;
  bids.push_back(bid);
  events.AddEvent("transfer", "Placed bid for " + playerName, 0, false);
  return bid;
}

void WithdrawBid(std::vector<TransferBid>& bids, const std::string& playerName) {
  auto it = std::find_if(bids.begin(), bids.end(), [&playerName](const TransferBid& bid) {
    return bid.playerName == playerName;
  });
  if (it == bids.end())
    return;
  it->status = BidStatus::WITHDRAWN;
}

bool ImprovePendingBid(TransferBid& bid, long long transferBudget) {
  if (bid.status != BidStatus::PENDING)
    return false;

  const long long increase = std::max(50000LL, bid.bidAmount / 10);
  const long long proposedBid = bid.bidAmount + increase;
  const long long proposedAgentFee = std::max(25000LL, proposedBid / 20);
  if (proposedBid + proposedAgentFee > transferBudget)
    return false;

  bid.bidAmount = proposedBid;
  bid.agentFee = proposedAgentFee;
  bid.negotiationRounds++;
  return true;
}

void ProcessPendingBids(CareerSave& save, CareerCommon::CareerEvents& events,
                        std::vector<TransferTarget>& targets, std::vector<TransferBid>& bids) {
  for (auto& bid : bids) {
    if (bid.status != BidStatus::PENDING)
      continue;

    auto targetIt = std::find_if(
        targets.begin(), targets.end(),
        [&bid](const TransferTarget& target) { return target.name == bid.playerName; });
    if (targetIt == targets.end()) {
      bid.status = BidStatus::REJECTED;
      continue;
    }

    long long requiredPrice = targetIt->askingPrice;
    // Negotiation rounds progressively soften the asking price (5% per round,
    // capped at 15%) rather than switching on a flat discount at round two.
    if (bid.negotiationRounds >= 1) {
      const int discount = std::min(15, 5 * bid.negotiationRounds);
      requiredPrice = requiredPrice * (100 - discount) / 100;
    }

    if (bid.bidAmount >= requiredPrice && bid.offeredWage >= targetIt->wage * 9 / 10) {
      bid.status = BidStatus::ACCEPTED;
      events.AddEvent("transfer", "Bid accepted for " + bid.playerName, 1, false);
    } else {
      bid.status = BidStatus::REJECTED;
      events.AddEvent("transfer", "Bid rejected for " + bid.playerName, -1, false);
    }
  }
}

std::string GetBidStatusString(BidStatus status) {
  switch (status) {
    case BidStatus::PENDING:
      return TR("career_bid_pending");
    case BidStatus::ACCEPTED:
      return TR("career_bid_accepted");
    case BidStatus::REJECTED:
      return TR("career_bid_rejected_status");
    case BidStatus::WITHDRAWN:
      return TR("career_bid_withdrawn");
  }
  return TR("career_bid_pending");
}

bool CompleteTransfer(CareerSave& save, CareerCommon::CareerEvents& events,
                      std::vector<TransferTarget>& targets, std::vector<TransferBid>& bids,
                      const std::string& playerName) {
  auto bidIt = std::find_if(bids.begin(), bids.end(), [&playerName](const TransferBid& bid) {
    return bid.playerName == playerName && bid.status == BidStatus::ACCEPTED;
  });
  if (bidIt == bids.end())
    return false;

  auto targetIt = std::find_if(
      targets.begin(), targets.end(),
      [&playerName](const TransferTarget& target) { return target.name == playerName; });
  if (targetIt == targets.end())
    return false;

  long long totalCost = bidIt->bidAmount + bidIt->agentFee;
  if (totalCost > save.transferBudget || bidIt->offeredWage > save.wageBudget)
    return false;

  PlayerCareerState player;
  player.name = targetIt->name;
  player.preferredPosition = targetIt->preferredPosition;
  player.position = targetIt->preferredPosition;
  player.ovr = targetIt->overallRating;
  player.pot = targetIt->potentialRating;
  player.age = targetIt->age;
  player.value = targetIt->value;
  player.wage = bidIt->offeredWage;
  player.contract.yearsRemaining = bidIt->contractYears;
  player.contract.transferListed = false;
  player.morale = 70;
  player.fitness = 95;
  player.matchForm = 60;

  save.transferBudget -= totalCost;
  save.wageBudget -= bidIt->offeredWage;
  save.finance.transferBudget = save.transferBudget;
  save.finance.wageBudget = save.wageBudget;
  save.finances.transferSpending += bidIt->bidAmount;
  save.roster.push_back(player);

  targets.erase(targetIt);
  bids.erase(std::remove_if(
                 bids.begin(), bids.end(),
                 [&playerName](const TransferBid& bid) { return bid.playerName == playerName; }),
             bids.end());

  events.AddEvent("transfer", "Completed transfer for " + playerName, 2, true);
  events.ModifyBoardConfidence(1);
  return true;
}

}  // namespace CareerTransfers
}  // namespace blunted
