#include "career_finance.hpp"

#include <algorithm>

#include "career_common.hpp"
#include "career_sim.hpp"
#include "utils/localization.hpp"

namespace blunted {
namespace CareerFinance {

namespace {
using CareerCommon::ClampInt;
}  // namespace

void ModifyBudget(CareerSave& save, long long transferDelta, long long wageDelta) {
  save.transferBudget += transferDelta;
  save.wageBudget += wageDelta;
  save.finance.transferBudget = save.transferBudget;
  save.finance.wageBudget = save.wageBudget;
}

void InitializeOwnerData(CareerSave& save) {
  if (save.stadium.name.empty())
    save.stadium.name = save.name + " Stadium";
  if (save.stadium.availableUpgrades.empty()) {
    save.stadium.availableUpgrades.push_back(
        {"Expand North Stand", "Adds a new upper tier.", 12000000, 2, 2, 8000, 1500000});
    save.stadium.availableUpgrades.push_back(
        {"Hospitality Suites", "Improves VIP match-day revenue.", 6500000, 1, 1, 0, 2200000});
    save.stadium.availableUpgrades.push_back({"Training Complex",
                                              "Improves overall squad fitness and growth.", 9000000,
                                              2, 2, 0, 1000000});
    save.stadium.availableUpgrades.push_back({"Youth Academy",
                                              "Massively boosts development of young players.", 15000000,
                                              3, 3, 0, 1200000});
  }

  if (save.staff.empty()) {
    save.staff.push_back(StaffMember("Avery Cole", "Assistant Coach", 68, 850000, 3));
    save.staff.push_back(StaffMember("Nina Petrov", "Head Scout", 72, 950000, 3));
    save.staff.push_back(StaffMember("Marcus Reed", "Physio", 70, 780000, 2));
  }

  long long playerWages = 0;
  for (const auto& player : save.roster)
    playerWages += player.wage;
  long long staffWages = 0;
  for (const auto& member : save.staff)
    staffWages += member.salary;

  save.finances.playerWages = playerWages;
  save.finances.staffWages = staffWages;
  save.finances.matchDayIncome = save.stadium.matchDayRevenue;
  save.finances.sponsorIncome = 0;
  save.finances.stadiumCosts = save.stadium.maintenanceCost;
  save.finances.totalRevenue = save.finances.matchDayIncome + save.finances.sponsorIncome +
                               save.finances.merchandiseIncome + save.finances.tvRevenue +
                               save.finances.transferIncome;
  save.finances.totalExpenses = save.finances.playerWages + save.finances.staffWages +
                                save.finances.stadiumCosts + save.finances.transferSpending;
}

void SetTicketPrice(CareerSave& save, int price) {
  save.finances.ticketPrice = ClampInt(price, 10, 200);
  int delta = save.finances.ticketPrice - 40;
  save.fanBase = ClampInt(save.fanBase - (delta / 8), 10, 100);
  save.stadium.fanSatisfaction = ClampInt(save.stadium.fanSatisfaction - (delta / 4), 0, 100);
}

void RepairStadium(CareerSave& save, int amount) {
  int repairAmount = std::max(1, amount);
  long long repairCost = 50000LL * std::max(1, repairAmount / 10);
  if (repairCost > save.finances.netWorth)
    return;

  save.finances.netWorth -= repairCost;
  save.stadium.condition = ClampInt(save.stadium.condition + repairAmount, 0, 100);
  save.stadium.fanSatisfaction = ClampInt(save.stadium.fanSatisfaction + repairAmount / 2, 0, 100);
}

void UpgradeStadium(CareerSave& save, CareerCommon::CareerEvents& events, int upgradeIndex) {
  if (upgradeIndex < 0 || upgradeIndex >= static_cast<int>(save.stadium.availableUpgrades.size()))
    return;

  const StadiumUpgrade upgrade = save.stadium.availableUpgrades[upgradeIndex];
  if (upgrade.cost > save.finances.netWorth)
    return;

  save.finances.netWorth -= upgrade.cost;
  save.finances.stadiumCosts += upgrade.cost / std::max(1, upgrade.buildTimeSeasons);
  save.stadium.upgrades.push_back(upgrade);
  save.stadium.availableUpgrades.erase(save.stadium.availableUpgrades.begin() + upgradeIndex);
  events.AddEvent("stadium", "Started stadium upgrade: " + upgrade.name, 1, false);
}

void RenameStadium(CareerSave& save, const std::string& newName) {
  if (newName.empty())
    return;
  save.stadium.name = newName;
}

void InvestInFanBase(CareerSave& save, long long amount) {
  if (amount <= 0 || amount > save.finances.netWorth)
    return;
  save.finances.netWorth -= amount;
  save.fanBase = ClampInt(save.fanBase + static_cast<int>(amount / 1000000LL) * 2, 0, 100);
  save.stadium.fanSatisfaction = ClampInt(save.stadium.fanSatisfaction + 5, 0, 100);
}

void InvestInPrestige(CareerSave& save, long long amount) {
  if (amount <= 0 || amount > save.finances.netWorth)
    return;
  save.finances.netWorth -= amount;
  save.clubPrestige = ClampInt(save.clubPrestige + static_cast<int>(amount / 1000000LL), 0, 100);
  save.reputation = ClampInt(save.reputation + static_cast<int>(amount / 1500000LL), 0, 100);
  save.club.reputation = save.reputation;
}

void ProcessSeasonFinances(CareerSave& save) {
  InitializeOwnerData(save);
  
  // Dynamic Attendance & Matchday Revenue
  // High prestige reduces the penalty of high ticket prices.
  double ticketRatio = 40.0 / static_cast<double>(std::max(10, save.finances.ticketPrice));
  double attendanceFactor = (save.fanBase * 0.4 + save.clubPrestige * 0.6) / 100.0;
  double prestigeResistance = save.clubPrestige / 100.0; 
  if (ticketRatio < 1.0) {
      ticketRatio = 1.0 - ((1.0 - ticketRatio) * (1.0 - prestigeResistance * 0.8));
  }
  
  int expectedAttendance = static_cast<int>(save.stadium.capacity * attendanceFactor * ticketRatio);
  expectedAttendance = std::max(5000, std::min(save.stadium.capacity, expectedAttendance));
  
  long long seasonMatchRevenue = static_cast<long long>(expectedAttendance) * save.finances.ticketPrice * 19LL;

  // TV Revenue based on estimated league position
  int finishPos = CareerSim::EstimateLeaguePosition(save.seasonWins, save.seasonDraws, save.seasonLosses);
  // 1st place gets ~20M, 20th place gets ~5M
  long long tvRevenue = 5000000LL + std::max(0, 20 - finishPos) * 750000LL;

  long long sponsorRevenue = 0;
  for (const auto& sponsor : save.activeSponsors)
    sponsorRevenue += sponsor.annualRevenue;
  long long merchandiseRevenue = static_cast<long long>(save.fanBase) * 120000LL; // Boosted slightly

  save.finances.matchDayIncome = seasonMatchRevenue;
  save.finances.tvRevenue = tvRevenue;
  save.finances.sponsorIncome = sponsorRevenue;
  save.finances.merchandiseIncome = merchandiseRevenue;
  save.finances.stadiumCosts = save.stadium.maintenanceCost;
  
  save.finances.totalRevenue = seasonMatchRevenue + sponsorRevenue + merchandiseRevenue +
                               tvRevenue + save.finances.transferIncome;
  save.finances.totalExpenses = save.finances.playerWages + save.finances.staffWages +
                                save.finances.stadiumCosts + save.finances.transferSpending;
                                
  long long profit = GetSeasonProfit(save);
  save.finances.netWorth = std::max(0LL, save.finances.netWorth + profit);
  save.transferBudget = std::max(0LL, save.transferBudget + profit / 2);
  save.finance.transferBudget = save.transferBudget;
}

long long GetSeasonProfit(const CareerSave& save) {
  return save.finances.totalRevenue - save.finances.totalExpenses;
}

std::string GetFinancialHealthString(const CareerSave& save) {
  long long profit = GetSeasonProfit(save);
  if (save.finances.netWorth >= 150000000 && profit >= 0)
    return TR("career_fin_elite");
  if (save.finances.netWorth >= 75000000 && profit >= -5000000)
    return TR("career_fin_stable");
  if (save.finances.netWorth >= 25000000)
    return TR("career_fin_tight");
  return TR("career_fin_critical");
}

}  // namespace CareerFinance
}  // namespace blunted
